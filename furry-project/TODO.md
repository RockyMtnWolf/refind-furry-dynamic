# Roadmap and Validation Checklist

## Implemented and Verified

- [x] Live CPU information from CPUID, with SMBIOS Type 4 fallback.
- [x] SMBIOS Type 17 memory capacity/type/speed/slots and EFI memory-map fallback.
- [x] Firmware vendor/revision, UEFI version, and Secure Boot ON/OFF/UNKNOWN.
- [x] PCI Display Controller/GOP enumeration and exact GPU vendor/device mapping.
- [x] Disk model, capacity, and transport through Block I/O, Device Path, Disk Info, and read-only NVMe Identify.
- [x] Adaptive 720p/1080p/4K theme, text truncation, and font-cache behavior after rescanning.
- [x] ASan/UBSan tests of the production SMBIOS parser, including malformed records and fallback cases.
- [x] QEMU/OVMF menu validation and isolated Secure Boot signed/unsigned child comparison.
- [x] Actual Ubuntu Live boot with UEFI runtime and SecureBoot enabled.
- [x] Actual Microsoft EFI boot manager to Windows WinPE; PEFirmwareType equals 2.
- [x] VMware basic menu, NVMe startup, firmware/GPU/disk information, and 1080p layout.
- [x] Windows default selection, visible five-second countdown, and automatic demo EFI launch in VMware.
- [x] Ubuntu installer package staging, repeatability, configuration, and invalid EFI rejection.

## Current Physical-Machine Validation — Completed (Ubuntu, 2026-10-11–12)

- [x] Current monitor: maximum GOP mode (3840×2160), smaller panel text and larger OS icons.
- [x] CPUID/SMBIOS RAM, NVIDIA RTX 2000 Ada / AMD graphics and separate NVMe/SATA disks; USB enclosure descriptor fallback.
- [x] Ubuntu MOK enrollment and signed rEFInd installation with Secure Boot enabled.
- [x] Signed updates with the existing key, EFI/BootOrder backups, original Ubuntu/fallback files preserved and recovery entry available.
- [x] Repeated Ubuntu boot, normal menu refresh and working About submenu on current firmware.

## Remaining Physical Coverage

- [ ] Secure Boot disabled.
- [ ] Other monitors/firmware and loader-return/rescan scenarios.
- [ ] Second Windows system disk discovery and full Windows desktop boot with Secure Boot enabled.
- [ ] Bare-metal five-second automatic boot after the final configuration change.
- [ ] Actual restoration from backup (backup integrity and the recovery entry are verified).

Use [HARDWARE-TEST.md](HARDWARE-TEST.md) for the detailed physical-machine checklist.

## Additional VM Coverage — TODO

- [ ] VMware Ubuntu MOK enrollment and full dual-system-disk OS startup.
- [ ] Hyper-V validation with an account authorized to manage virtual machines.
- [ ] Completed Windows installation and desktop startup; current Windows evidence covers WinPE only.

## Publication

- [x] English README with actual screenshots and explicit validation scope.
- [x] Public GitHub repository with the furry-dynamic development branch.

Detailed test evidence: [Phase 1](evidence/VERIFICATION.md) and [Phase 2](evidence/phase2/VERIFICATION.md).
