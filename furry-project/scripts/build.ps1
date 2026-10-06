param([string]$Distribution='Ubuntu-24.04')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$linuxPath=(& wsl -d $Distribution -- wslpath -a ($root.Replace('\','/'))).Trim()
& wsl -d $Distribution -- bash "$linuxPath/scripts/build.sh"
if($LASTEXITCODE -ne 0){throw 'Build failed; see evidence/build.log'}
