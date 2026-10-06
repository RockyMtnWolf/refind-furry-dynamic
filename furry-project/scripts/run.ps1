param([int]$MemoryMiB=2048,[string]$Distribution='Ubuntu-24.04',[switch]$Window)
$root=Split-Path $PSScriptRoot -Parent
$linuxPath=(& wsl -d $Distribution -- wslpath -a ($root.Replace('\','/'))).Trim()
$backend=if($Window){'gtk'}else{'none'}
& wsl -d $Distribution -- env "RAM=$MemoryMiB" "DISPLAY_BACKEND=$backend" bash "$linuxPath/scripts/run.sh"
