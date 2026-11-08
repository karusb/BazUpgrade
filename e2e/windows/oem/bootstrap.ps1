$ErrorActionPreference = 'Stop'
$share = '\\host.lan\Data'
for ($attempt = 0; $attempt -lt 360; ++$attempt) {
    if (Test-Path "$share\input\ready.json" -ErrorAction SilentlyContinue) { break }
    Start-Sleep -Seconds 10
}
try {
    $configuration = Get-Content "$share\input\ready.json" -Raw | ConvertFrom-Json
    $bios = Get-CimInstance Win32_ComputerSystem
    if ($bios.Manufacturer -notmatch 'QEMU' -or $configuration.guard -ne 'bazupgrade-docker-e2e' -or $configuration.nonce -notmatch '^[a-f0-9]{32}$') {
        throw 'This harness can run only inside the designated QEMU Windows guest.'
    }
    New-Item -ItemType Directory C:\BazUpgradeE2E -Force | Out-Null
    # Resume the exact signed binaries after reboot; never overwrite them from
    # the host input directory while an installation checkpoint exists.
    if (-not (Test-Path C:\BazUpgradeE2E\checkpoint.json)) {
        Set-TimeZone -Id UTC
        Copy-Item "$share\input\*" C:\BazUpgradeE2E -Recurse -Force
    }
    & C:\BazUpgradeE2E\Guest.ps1 -Evidence "$share\evidence\$($configuration.nonce)" -Nonce $configuration.nonce
} catch {
    New-Item -ItemType Directory "$share\evidence" -Force | Out-Null
    @{ status='failed'; message=$_.Exception.Message; time=[DateTime]::UtcNow.ToString('o') } |
        ConvertTo-Json | Set-Content "$share\evidence\bootstrap-failure.json" -Encoding UTF8
}
