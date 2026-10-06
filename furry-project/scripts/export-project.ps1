param([Parameter(Mandatory=$true)][string]$Destination)
$root=Split-Path $PSScriptRoot -Parent
$source=Join-Path $root 'refind'
if(!(Test-Path (Join-Path $source 'refind'))){$source=Split-Path $root -Parent}
if(Test-Path $Destination){throw 'Choose a new destination directory.'}
New-Item -ItemType Directory -Path $Destination | Out-Null
foreach($item in @('README.md','CHANGES.md','TODO.md','HARDWARE-TEST.md','refind.conf','scripts','theme','theme-reference','assets')){
    Copy-Item -LiteralPath (Join-Path $root $item) -Destination $Destination -Recurse
}
git clone --no-hardlinks $source (Join-Path $Destination 'refind')
if($LASTEXITCODE -ne 0){throw 'Source clone failed.'}
git -C (Join-Path $Destination 'refind') remote rename origin local-source
git -C (Join-Path $Destination 'refind') remote add upstream https://git.code.sf.net/p/refind/code
Write-Host "Exported project to $Destination"
