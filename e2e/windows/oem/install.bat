@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$action = New-ScheduledTaskAction -Execute 'powershell.exe' -Argument '-NoProfile -ExecutionPolicy Bypass -File C:\OEM\bootstrap.ps1'; $trigger = New-ScheduledTaskTrigger -AtStartup; Register-ScheduledTask -TaskName BazUpgradeDockerE2E -Action $action -Trigger $trigger -User SYSTEM -RunLevel Highest -Force | Out-Null"
start "" /b powershell.exe -NoProfile -ExecutionPolicy Bypass -File C:\OEM\bootstrap.ps1
exit /b 0
