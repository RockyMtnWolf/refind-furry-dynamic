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

## Final Windows Physical-Machine Validation — Completed

- [x] Final Windows physical-machine boot test: user confirmed success on 2026-10-12 after applying the five-second Windows-first configuration.

Use [HARDWARE-TEST.md](HARDWARE-TEST.md) for the physical-machine procedure. All TODO items in the current project scope are complete. The final Windows result is based on user confirmation; no new Windows screenshot or diagnostic capture is included. VMware Ubuntu MOK / full dual-disk boot and Hyper-V are outside the current validation scope. Other firmware/monitors, Secure Boot-off behavior and an actual backup restoration are not claimed as tested and are not remaining TODO items.

## Publication

- [x] English README with actual screenshots and explicit validation scope.
- [x] Public GitHub repository with the furry-dynamic development branch.

Detailed test evidence: [Phase 1](evidence/VERIFICATION.md) and [Phase 2](evidence/phase2/VERIFICATION.md).
