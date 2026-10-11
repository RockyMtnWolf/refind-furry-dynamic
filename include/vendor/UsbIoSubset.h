/* SPDX-License-Identifier: BSD-2-Clause-Patent
 * Read-only subset of EFI_USB_IO_PROTOCOL, with the original field offsets.
 * Based on MdePkg/Include/Protocol/UsbIo.h and IndustryStandard/Usb.h:
 * Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.
 * https://github.com/tianocore/edk2
 */
#pragma once
#define FURRY_USB_IO_GUID {0x2b2f68d6,0x0cd2,0x44cf,{0x8e,0x8b,0xbb,0xa2,0x0b,0x1b,0x5b,0x75}}
#pragma pack(1)
typedef struct {
    UINT8 Length,DescriptorType; UINT16 BcdUSB;
    UINT8 DeviceClass,DeviceSubClass,DeviceProtocol,MaxPacketSize0;
    UINT16 IdVendor,IdProduct,BcdDevice;
    UINT8 StrManufacturer,StrProduct,StrSerialNumber,NumConfigurations;
} FURRY_USB_DEVICE_DESCRIPTOR;
#pragma pack()
typedef struct FURRY_USB_IO FURRY_USB_IO;
struct FURRY_USB_IO {
    VOID (EFIAPI *UnusedTransfers[6])(VOID);
    EFI_STATUS (EFIAPI *GetDeviceDescriptor)(FURRY_USB_IO *,FURRY_USB_DEVICE_DESCRIPTOR *);
    VOID (EFIAPI *UnusedDescriptors[3])(VOID);
    EFI_STATUS (EFIAPI *GetStringDescriptor)(FURRY_USB_IO *,UINT16,UINT8,CHAR16 **);
    EFI_STATUS (EFIAPI *GetSupportedLanguages)(FURRY_USB_IO *,UINT16 **,UINT16 *);
    VOID (EFIAPI *UnusedReset)(VOID);
};
