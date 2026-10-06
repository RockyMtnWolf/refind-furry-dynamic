param([string]$Distribution='Ubuntu-24.04')
& wsl -d $Distribution -u root -- sh -c 'apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install --no-install-recommends -y build-essential gnu-efi ovmf qemu-system-x86 qemu-system-gui python3-pil'
if($LASTEXITCODE -ne 0){throw 'Dependency installation failed.'}
