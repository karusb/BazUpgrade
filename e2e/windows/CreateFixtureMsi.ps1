function New-FixtureMsi {
    param([string]$Path, [ValidateSet('1.0.0','2.0.0')][string]$Version, [string]$ProductCode, [string]$UpgradeCode)
    $installer = New-Object -ComObject WindowsInstaller.Installer
    $db = $installer.GetType().InvokeMember('OpenDatabase','InvokeMethod',$null,$installer,@($Path,3))
    function Sql([string]$statement) {
        $view = $db.GetType().InvokeMember('OpenView','InvokeMethod',$null,$db,@($statement))
        try { $view.GetType().InvokeMember('Execute','InvokeMethod',$null,$view,$null) | Out-Null }
        finally { $view.GetType().InvokeMember('Close','InvokeMethod',$null,$view,$null) | Out-Null; [Runtime.InteropServices.Marshal]::FinalReleaseComObject($view) | Out-Null }
    }
    try {
        Sql 'CREATE TABLE `Property` (`Property` CHAR(72) NOT NULL, `Value` CHAR(0) LOCALIZABLE PRIMARY KEY `Property`)'
        foreach ($entry in @{
            ProductCode=$ProductCode; UpgradeCode=$UpgradeCode; ProductName='BazUpgrade synthetic reboot fixture'
            ProductVersion=$Version; ProductLanguage='1033'; Manufacturer='Synthetic fixture'; ALLUSERS='1'
            SecureCustomProperties='OLDPRODUCTS'; ARPNOREPAIR='1'; ARPNOMODIFY='1'
        }.GetEnumerator()) {
            Sql ('INSERT INTO `Property` (`Property`,`Value`) VALUES (''{0}'',''{1}'')' -f $entry.Key,$entry.Value)
        }
        Sql 'CREATE TABLE `Directory` (`Directory` CHAR(72) NOT NULL, `Directory_Parent` CHAR(72), `DefaultDir` CHAR(255) NOT NULL LOCALIZABLE PRIMARY KEY `Directory`)'
        Sql 'CREATE TABLE `Media` (`DiskId` SHORT NOT NULL, `LastSequence` SHORT NOT NULL, `DiskPrompt` CHAR(64) LOCALIZABLE, `Cabinet` CHAR(255), `VolumeLabel` CHAR(32), `Source` CHAR(72) PRIMARY KEY `DiskId`)'
        Sql 'INSERT INTO `Media` (`DiskId`,`LastSequence`) VALUES (1,0)'
        Sql 'CREATE TABLE `File` (`File` CHAR(72) NOT NULL, `Component_` CHAR(72) NOT NULL, `FileName` CHAR(255) NOT NULL LOCALIZABLE, `FileSize` LONG NOT NULL, `Version` CHAR(72), `Language` CHAR(20), `Attributes` SHORT, `Sequence` LONG NOT NULL PRIMARY KEY `File`)'
        Sql 'INSERT INTO `Directory` (`Directory`,`DefaultDir`) VALUES (''TARGETDIR'',''SourceDir'')'
        Sql 'CREATE TABLE `Component` (`Component` CHAR(72) NOT NULL, `ComponentId` CHAR(38), `Directory_` CHAR(72) NOT NULL, `Attributes` SHORT NOT NULL, `Condition` CHAR(255), `KeyPath` CHAR(72) PRIMARY KEY `Component`)'
        $component = '{' + [guid]::NewGuid().ToString().ToUpperInvariant() + '}'
        Sql ('INSERT INTO `Component` (`Component`,`ComponentId`,`Directory_`,`Attributes`,`KeyPath`) VALUES (''FixtureRegistry'',''{0}'',''TARGETDIR'',260,''VersionMarker'')' -f $component)
        Sql 'CREATE TABLE `Registry` (`Registry` CHAR(72) NOT NULL, `Root` SHORT NOT NULL, `Key` CHAR(255) NOT NULL LOCALIZABLE, `Name` CHAR(255) LOCALIZABLE, `Value` CHAR(0) LOCALIZABLE, `Component_` CHAR(72) NOT NULL PRIMARY KEY `Registry`)'
        Sql 'INSERT INTO `Registry` (`Registry`,`Root`,`Key`,`Name`,`Value`,`Component_`) VALUES (''VersionMarker'',2,''Software\BazUpgradeDockerE2E'',''InstalledVersion'',''[ProductVersion]'',''FixtureRegistry'')'
        Sql 'CREATE TABLE `Feature` (`Feature` CHAR(38) NOT NULL, `Feature_Parent` CHAR(38), `Title` CHAR(64) LOCALIZABLE, `Description` CHAR(255) LOCALIZABLE, `Display` SHORT, `Level` SHORT NOT NULL, `Directory_` CHAR(72), `Attributes` SHORT NOT NULL PRIMARY KEY `Feature`)'
        Sql 'INSERT INTO `Feature` (`Feature`,`Title`,`Display`,`Level`,`Attributes`) VALUES (''Main'',''Synthetic registry fixture'',1,1,0)'
        Sql 'CREATE TABLE `FeatureComponents` (`Feature_` CHAR(38) NOT NULL, `Component_` CHAR(72) NOT NULL PRIMARY KEY `Feature_`,`Component_`)'
        Sql 'INSERT INTO `FeatureComponents` (`Feature_`,`Component_`) VALUES (''Main'',''FixtureRegistry'')'
        Sql 'CREATE TABLE `Upgrade` (`UpgradeCode` CHAR(38) NOT NULL, `VersionMin` CHAR(20), `VersionMax` CHAR(20), `Language` CHAR(255), `Attributes` SHORT NOT NULL, `Remove` CHAR(255), `ActionProperty` CHAR(72) NOT NULL PRIMARY KEY `UpgradeCode`,`VersionMin`,`VersionMax`,`Language`,`Attributes`)'
        Sql ('INSERT INTO `Upgrade` (`UpgradeCode`,`VersionMax`,`Attributes`,`ActionProperty`) VALUES (''{0}'',''{1}'',513,''OLDPRODUCTS'')' -f $UpgradeCode,$Version)
        Sql 'CREATE TABLE `InstallExecuteSequence` (`Action` CHAR(72) NOT NULL, `Condition` CHAR(255), `Sequence` SHORT PRIMARY KEY `Action`)'
        $sequence = [ordered]@{
            FindRelatedProducts=25; CostInitialize=800; FileCost=900; CostFinalize=1000
            InstallValidate=1400; RemoveExistingProducts=1450; InstallInitialize=1500
            ProcessComponents=1600; UnpublishFeatures=1800; RemoveRegistryValues=2600
            WriteRegistryValues=5000; RegisterUser=6000; RegisterProduct=6100
            PublishFeatures=6300; PublishProduct=6400; ScheduleReboot=6600; InstallFinalize=6700
        }
        foreach ($action in $sequence.GetEnumerator()) {
            $condition = if ($action.Key -eq 'ScheduleReboot') { 'NOT (REMOVE~="ALL")' } else { '' }
            Sql ('INSERT INTO `InstallExecuteSequence` (`Action`,`Condition`,`Sequence`) VALUES (''{0}'',''{1}'',{2})' -f $action.Key,$condition,$action.Value)
        }
        $summary = $db.GetType().InvokeMember('SummaryInformation','GetProperty',$null,$db,@(20))
        foreach ($item in @{1=1252;2='Synthetic E2E installation';3='BazUpgrade Docker test';4='Synthetic fixture';7='x64;1033';9=('{' + [guid]::NewGuid().ToString().ToUpperInvariant() + '}');14=200;15=2;18='BazUpgrade E2E';19=2}.GetEnumerator()) {
            $summary.GetType().InvokeMember('Property','SetProperty',$null,$summary,@([int]$item.Key,$item.Value)) | Out-Null
        }
        $summary.GetType().InvokeMember('Persist','InvokeMethod',$null,$summary,$null) | Out-Null
        [Runtime.InteropServices.Marshal]::FinalReleaseComObject($summary) | Out-Null
        $db.GetType().InvokeMember('Commit','InvokeMethod',$null,$db,$null) | Out-Null
    } finally {
        [Runtime.InteropServices.Marshal]::FinalReleaseComObject($db) | Out-Null
        [Runtime.InteropServices.Marshal]::FinalReleaseComObject($installer) | Out-Null
    }
}
