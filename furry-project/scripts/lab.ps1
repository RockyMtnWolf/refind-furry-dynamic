param(
 [ValidateSet('prepare','on','off','reject','ubuntu','windows','quit','capture','test','fixtures')][string]$Action='on',
 [string]$Distribution='Ubuntu-24.04',
 [int]$Width=3840,[int]$Height=2160,[int]$MemoryMiB=4096,
 [string]$Output='phase2/latest.png',
 [string]$IsoPath,
 [ValidateSet('kvm','tcg')][string]$Accelerator='kvm'
)
$root=Split-Path $PSScriptRoot -Parent
$linuxRoot=(& wsl -d $Distribution -- wslpath -a ($root.Replace('\','/'))).Trim()
if($Action -in @('quit','capture')) {
 $argsList=@('-d',$Distribution,'--','python3',"$linuxRoot/scripts/capture.py",'--port','4450')
 if($Action -eq 'quit'){$argsList+='--quit'}else{$argsList+=@('--output',$Output)}
 & wsl @argsList
} elseif($Action -eq 'prepare') {
 & wsl -d $Distribution -u root -- bash "$linuxRoot/scripts/prepare-secure-lab.sh"
} elseif($Action -eq 'test') {
 & wsl -d $Distribution -- bash "$linuxRoot/scripts/test.sh"
} elseif($Action -eq 'fixtures') {
 & wsl -d $Distribution -- python3 "$linuxRoot/scripts/make-smbios-fixtures.py"
} else {
 $cpu=if($Accelerator -eq 'kvm'){'host'}else{'max'}
 $envArgs=@("MODE=$Action","WIDTH=$Width","HEIGHT=$Height","RAM=$MemoryMiB","ACCEL=$Accelerator","CPU=$cpu")
 if($IsoPath){
  $linuxIso=(& wsl -d $Distribution -- wslpath -a ($IsoPath.Replace('\','/'))).Trim()
  $envArgs+="ISO=$linuxIso"
 }
 & wsl -d $Distribution -u root -- env @envArgs bash "$linuxRoot/scripts/run-lab.sh"
}
if($LASTEXITCODE -ne 0){throw "Lab action failed: $Action"}
