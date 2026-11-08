param([Parameter(Mandatory=$true)][string]$Evidence, [Parameter(Mandatory=$true)][string]$Nonce)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$bios = Get-CimInstance Win32_ComputerSystem
if ($bios.Manufacturer -notmatch 'QEMU' -or $env:COMPUTERNAME -ne 'BazUpgradeE2E' -or $Nonce -notmatch '^[a-f0-9]{32}$') {
    throw 'Guest identity guard failed; this test must never run on the host.'
}
Set-Location C:\BazUpgradeE2E
New-Item -ItemType Directory $Evidence -Force | Out-Null
Start-Transcript -Path "$Evidence\guest-transcript.log" -Append | Out-Null
function SaveJson($Path, $Value) {
    [IO.File]::WriteAllText($Path, ($Value | ConvertTo-Json -Depth 40), (New-Object Text.UTF8Encoding($false)))
}
function Tool([string]$Executable, [string[]]$Arguments) {
    $log = Join-Path $PWD ('tool-' + [guid]::NewGuid().ToString('N') + '.stderr.log')
    $savedPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $result = & $Executable @Arguments 2> $log
        $code = $LASTEXITCODE
    } finally { $ErrorActionPreference = $savedPreference }
    Copy-Item -LiteralPath $log -Destination $Evidence
    if ($code -ne 0) { throw "Tool failed ($code): $Executable $Arguments ; $(Get-Content -LiteralPath $log -Raw)" }
    return ($result -join [Environment]::NewLine) | ConvertFrom-Json
}
function Assert($Condition, [string]$Message) { if (-not $Condition) { throw $Message } }
function Marker { (Get-ItemProperty -LiteralPath HKLM:\Software\BazUpgradeDockerE2E).InstalledVersion }
function BootTime { (Get-CimInstance Win32_OperatingSystem).LastBootUpTime.ToUniversalTime().ToString('o') }
function BootRecord { (Get-WinEvent -FilterHashtable @{LogName='System';ProviderName='Microsoft-Windows-Kernel-General';Id=12} -MaxEvents 1).RecordId }
function RestartGuest($State) {
    SaveJson "$PWD\checkpoint.json" $State
    SaveJson "$Evidence\checkpoint.json" $State
    Stop-Transcript | Out-Null
    Restart-Computer -Force
    exit
}
try {
    $publisher = "$PWD\bazupgrade-publish.exe"
    $client = "$PWD\bazupgrade.exe"
    $config = "$PWD\client.json"
    if (Test-Path .\checkpoint.json) {
        $state = Get-Content .\checkpoint.json -Raw | ConvertFrom-Json
        Assert ($state.nonce -eq $Nonce) 'Unexpected prior test session'
        Assert ((BootTime) -ne $state.previousBoot) 'Container restart did not prove a Windows reboot'
        Assert ((BootRecord) -gt $state.previousBootRecord) 'No new Windows kernel boot event was observed'
        Assert ((Tool $client @('status','--config',$config)).phase -eq 'pendingReboot') 'Reboot lost pending operation'
        Assert ((Marker) -eq "$($state.sequence).0.0") 'Installed version marker changed across reboot'
        Tool $client @('reconcile','--config',$config,'--installed','true') | Out-Null
        Assert ((Tool $client @('apply','--config',$config)).status -eq 'upToDate') 'Completed operation was replayed'
        if ($state.sequence -eq 2) {
            Assert (-not (Test-Path "HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\$($state.firstProduct)")) 'Upgrade retained old MSI product'
            SaveJson "$Evidence\result.json" @{
                status='passed'; nonce=$Nonce; certificate='temporary self-signed RSA3072/SHA256, guest trust only'
                firstBoot=$state.firstBoot; secondBoot=$state.previousBoot; thirdBoot=(BootTime)
                firstBootRecord=$state.firstBootRecord; secondBootRecord=$state.previousBootRecord; thirdBootRecord=(BootRecord)
                installedVersion=(Marker); sequence=2; nativeReboots=2; installer='Windows Installer MSI'
                tools=@('create-test-certificate','sign-windows','init','publish','check','apply','status','reconcile','pack','rotate-root','refresh','restore-metadata','export-electron')
                identity=$bios.Manufacturer; hostTrustChanges=0
            }
            Unregister-ScheduledTask -TaskName BazUpgradeDockerE2E -Confirm:$false
            Stop-Transcript | Out-Null
            exit
        }
    } else {
        SaveJson "$Evidence\input-binaries.json" @{
            nonce=$Nonce
            binaries=@(foreach ($name in @('bazupgrade.exe','bazupgrade-publish.exe','bazupgrade-sdk.dll')) {
                @{ name=$name; sha256=(Get-FileHash -LiteralPath (Join-Path $PWD $name) -Algorithm SHA256).Hash }
            })
        }
        $firstProduct = '{' + [guid]::NewGuid().ToString().ToUpperInvariant() + '}'
        $secondProduct = '{' + [guid]::NewGuid().ToString().ToUpperInvariant() + '}'
        $upgrade = '{' + [guid]::NewGuid().ToString().ToUpperInvariant() + '}'
        $certificate = Tool $publisher @('create-test-certificate','--output',"$PWD\test-signing.cer")
        Assert $certificate.testOnly 'Certificate must be test-only'
        Import-Certificate -FilePath .\test-signing.cer -CertStoreLocation Cert:\LocalMachine\Root | Out-Null
        Import-Certificate -FilePath .\test-signing.cer -CertStoreLocation Cert:\LocalMachine\TrustedPeople | Out-Null
        . .\CreateFixtureMsi.ps1
        New-FixtureMsi "$PWD\fixture-1.msi" '1.0.0' $firstProduct $upgrade
        New-FixtureMsi "$PWD\fixture-2.msi" '2.0.0' $secondProduct $upgrade
        foreach ($artifact in @('fixture-1.msi','fixture-2.msi','bazupgrade.exe','bazupgrade-sdk.dll')) {
            Tool $publisher @('sign-windows','--artifact',"$PWD\$artifact",'--certificate-thumbprint',$certificate.thumbprint,'--sign-tool',"$PWD\signtool.exe") | Out-Null
            Assert ((Get-AuthenticodeSignature "$PWD\$artifact").Status -eq 'Valid') "Untrusted signature: $artifact"
        }
        Copy-Item $publisher .\publisher-signed.exe
        Tool $publisher @('sign-windows','--artifact',"$PWD\publisher-signed.exe",'--certificate-thumbprint',$certificate.thumbprint,'--sign-tool',"$PWD\signtool.exe") | Out-Null
        Move-Item .\publisher-signed.exe $publisher -Force
        Tool $publisher @('init','--repo',"$PWD\repository",'--keys',"$PWD\private-signing") | Out-Null
        Copy-Item .\repository\metadata\1.root.json .\trusted-root.json
        SaveJson $config @{
            schemaVersion=1; repository='./repository'; trustedRoot='./trusted-root.json'; appId='synthetic'
            channel='stable'; os='windows'; arch='x64'; packageType='msi'; target='fixture.msi'
            stateDirectory='./client-state'; installDirectory='./client-install'
        }
        $state = [pscustomobject]@{ nonce=$Nonce; sequence=0; firstBoot=(BootTime); previousBoot=''; firstBootRecord=(BootRecord); previousBootRecord=0; firstProduct=$firstProduct; secondProduct=$secondProduct; thumbprint=$certificate.thumbprint }
        New-Item -ItemType Directory .\portable | Out-Null
        [IO.File]::WriteAllText("$PWD\portable\hello.txt",'synthetic fixture')
        Tool $publisher @('pack','--directory',"$PWD\portable",'--output',"$PWD\portable.zip") | Out-Null
        Tool $publisher @('rotate-root','--repo',"$PWD\repository",'--keys',"$PWD\private-signing") | Out-Null
        Tool $publisher @('refresh','--repo',"$PWD\repository",'--keys',"$PWD\private-signing") | Out-Null
        Tool $publisher @('restore-metadata','--repo',"$PWD\repository",'--keys',"$PWD\private-signing") | Out-Null
    }
    $next = $state.sequence + 1
    SaveJson "$PWD\release.json" @{
        schemaVersion=1; appId='synthetic'; channel='stable'; os='windows'; arch='x64'; packageType='msi'
        appVersion="$next.0.0"; releaseSequence=$next; minUpdaterMajor=2
    }
    Tool $publisher @('publish','--repo',"$PWD\repository",'--keys',"$PWD\private-signing",'--artifact',"$PWD\fixture-$next.msi",'--target','fixture.msi','--descriptor',"$PWD\release.json") | Out-Null
    Tool $publisher @('export-electron','--repo',"$PWD\repository",'--keys',"$PWD\private-signing",'--target','fixture.msi','--base-url','https://updates.example.invalid/','--output',"$PWD\latest.yml",'--unsigned-native-feed','true') | Out-Null
    Assert ((Tool $client @('check','--config',$config)).releaseSequence -eq $next) 'Wrong TUF release'
    $result = Tool $client @('apply','--config',$config)
    Assert ($result.status -eq 'pendingReboot') 'Real MSI did not request reboot (expected exit 3010)'
    Assert ((Marker) -eq "$next.0.0") 'MSI did not install the version marker'
    $state.sequence = $next
    $state.previousBoot = BootTime
    $state.previousBootRecord = BootRecord
    RestartGuest $state
} catch {
    SaveJson "$Evidence\failure.json" @{ status='failed'; message=$_.Exception.Message; stack=$_.ScriptStackTrace; nonce=$Nonce; boot=(BootTime) }
    Stop-Transcript | Out-Null
    throw
}
