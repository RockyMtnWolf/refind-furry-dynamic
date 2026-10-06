param([string]$Distribution='Ubuntu-24.04',[string]$Output='qemu-4k.png',[string]$Key,[switch]$Quit)
$root=Split-Path $PSScriptRoot -Parent
$linuxPath=(& wsl -d $Distribution -- wslpath -a ($root.Replace('\','/'))).Trim()
$argsList=@('python3',"$linuxPath/scripts/capture.py",'--output',$Output)
if($Key){$argsList+=@('--key',$Key)}
if($Quit){$argsList+='--quit'}
& wsl -d $Distribution -- @argsList
if($LASTEXITCODE -ne 0){throw 'QMP capture/control failed.'}
