/* SPDX-License-Identifier: GPL-3.0-or-later
 * Read-only PCI, Block I/O, Disk Info and NVMe Identify providers.
 * Protocol declarations in include/vendor retain their EDK II BSD license.
 */
#include "global.h"
#include "libeg.h"
#include "dynamic_devices.h"
#include "log.h"
#include "../include/refit_call_wrapper.h"
#ifdef __MAKEWITH_GNUEFI
#include <efipciio.h>
typedef EFI_DEVICE_PATH EFI_DEVICE_PATH_PROTOCOL;
#endif
#include "../include/vendor/DiskInfo.h"
#include "../include/vendor/NvmExpressPassthru.h"
#include "../include/vendor/UsbIoSubset.h"

static UINTN PathLength(EFI_DEVICE_PATH *p) {
    UINTN n=0,l;
    if(!p) return 0;
    while(n<4096) {
        l=p->Length[0]+((UINTN)p->Length[1]<<8);
        if(l<4 || n+l>4096) return 0;
        if(p->Type==0x7f) return n;
        n+=l; p=(EFI_DEVICE_PATH *)((UINT8 *)p+l);
    }
    return 0;
}
static BOOLEAN ParentPath(EFI_DEVICE_PATH *parent, EFI_DEVICE_PATH *child) {
    UINTN n=PathLength(parent),m=PathLength(child);
    return n && m>=n && CompareMem(parent,child,n)==0;
}
static CHAR16 *Transport(EFI_DEVICE_PATH *p) {
    UINTN n=0,l;
    while(p && n<4096) {
        l=p->Length[0]+((UINTN)p->Length[1]<<8);
        if(l<4 || n+l>4096 || p->Type==0x7f) break;
        if(p->Type==3) {
            if(p->SubType==0x17) return L"NVMe";
            if(p->SubType==0x12) return L"SATA";
            if(p->SubType==5 || p->SubType==0x10) return L"USB";
            if(p->SubType==2) return L"SCSI";
            if(p->SubType==1) return L"ATA";
        }
        n+=l; p=(EFI_DEVICE_PATH *)((UINT8 *)p+l);
    }
    return L"Block I/O";
}
static VOID AsciiField(CHAR16 *out, UINT8 *in, UINTN len, BOOLEAN swap) {
    UINTN i,j=0; UINT8 c; BOOLEAN meaningful=FALSE;
    out[0]=0;
    if(swap && (len&1)) return;
    while(len && (in[swap?((len-1)^1):len-1]==' ' || in[swap?((len-1)^1):len-1]==0)) len--;
    for(i=0;i<len && j<63;i++) {
        c=in[swap?(i^1):i];
        if(c<32 || c>=127) { out[0]=0; return; }
        if((c>='0' && c<='9') || (c>='A' && c<='Z') || (c>='a' && c<='z')) meaningful=TRUE;
        out[j++]=c;
    }
    out[j]=0;
    if(!meaningful) out[0]=0;
}
static UINTN Hex4(UINT8 *p) {
    UINTN i,v=0,c;
    for(i=0;i<4;i++) {
        c=p[i]; if(c>='0' && c<='9') c-='0';
        else if(c>='a' && c<='f') c=c-'a'+10;
        else if(c>='A' && c<='F') c=c-'A'+10;
        else return 0x10000;
        v=(v<<4)|c;
    }
    return v;
}
/* Updateable, bounded ASCII database. Exact PCI IDs only. */
static BOOLEAN GpuName(UINT16 vendor,UINT16 device,UINT16 subvendor,UINT16 subdevice,CHAR16 *name) {
    UINT8 *data=NULL; UINTN size=0,i=0,j,k,start; BOOLEAN exact=FALSE,found=FALSE;
    if(EFI_ERROR(egLoadFile(SelfDir,L"themes\\finn-term\\gpu-names.txt",&data,&size))) return FALSE;
    if(size>65536) { FreePool(data); return FALSE; }
    while(i+10<size) {
        j=i; while(j<size && data[j]!='\n') j++;
        start=0;
        if(j-i>10 && data[i+4]==':' && Hex4(data+i)==vendor && Hex4(data+i+5)==device) {
            if(data[i+9]=='|' && !exact) start=i+10;
            else if(j-i>20 && data[i+9]==':' && data[i+14]==':' && data[i+19]=='|' &&
                    Hex4(data+i+10)==subvendor && Hex4(data+i+15)==subdevice) {
                start=i+20; exact=TRUE;
            }
            if(start) {
                k=0; while(start<j && data[start]!='\r' && k<63) {
                    name[k++]=data[start]>=32 && data[start]<127?data[start]:'?'; start++;
                }
                name[k]=0; found=k!=0;
            }
        }
        i=j+1;
    }
    FreePool(data); return found;
}
VOID DynamicGpus(DYNAMIC_DEVICE_LINE emit, VOID *context) {
    EFI_GUID guid=EFI_PCI_IO_PROTOCOL_GUID;
    EFI_HANDLE *handles=NULL; EFI_PCI_IO_PROTOCOL *pci;
    UINTN count=0,i,total=0; UINT8 config[48]; UINT16 vendor,device,subvendor,subdevice;
    CHAR16 name[64];
    if(EFI_ERROR(refit_call5_wrapper(BS->LocateHandleBuffer,ByProtocol,&guid,NULL,&count,&handles))) {
        emit(1,L"n/a",context); return;
    }
    for(i=0;i<count;i++) {
        if(EFI_ERROR(refit_call3_wrapper(BS->HandleProtocol,handles[i],&guid,(VOID **)&pci))) continue;
        if(EFI_ERROR(refit_call5_wrapper(pci->Pci.Read,pci,EfiPciIoWidthUint8,0,sizeof(config),config)) || config[11]!=3) continue;
        vendor=(config[0]|((UINT16)config[1]<<8)); device=(config[2]|((UINT16)config[3]<<8));
        subvendor=0; subdevice=0;
        if((config[14]&0x7f)==0) { subvendor=(config[44]|((UINT16)config[45]<<8)); subdevice=(config[46]|((UINT16)config[47]<<8)); }
        if(!GpuName(vendor,device,subvendor,subdevice,name))
            SPrint(name,sizeof(name),L"%s GPU [%04x:%04x]",vendor==0x10de?L"NVIDIA":vendor==0x1002?L"AMD":vendor==0x8086?L"Intel":L"PCI",vendor,device);
        LOG(1, LOG_LINE_NORMAL, L"GPU probe: %04x:%04x / %04x:%04x = %s",vendor,device,subvendor,subdevice,name);
        if(!emit(++total,name,context)) break;
    }
    FreePool(handles); if(!total) emit(1,L"n/a",context);
}
/* Identify Controller opcode 0x06/CNS=1 only; no data writes or admin changes. */
static BOOLEAN NvmeName(EFI_DEVICE_PATH *disk,CHAR16 *name) {
    EFI_GUID guid=EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL_GUID,dp=EFI_DEVICE_PATH_PROTOCOL_GUID;
    EFI_HANDLE *handles=NULL; UINTN count=0,i,align; EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL *nvme;
    EFI_DEVICE_PATH *path; EFI_NVM_EXPRESS_COMMAND cmd; EFI_NVM_EXPRESS_COMPLETION completion;
    EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET packet; UINT8 *allocation,*buffer; EFI_STATUS status; BOOLEAN found=FALSE;
    if(EFI_ERROR(refit_call5_wrapper(BS->LocateHandleBuffer,ByProtocol,&guid,NULL,&count,&handles))) return FALSE;
    for(i=0;i<count && !found;i++) {
        if(EFI_ERROR(refit_call3_wrapper(BS->HandleProtocol,handles[i],&dp,(VOID **)&path)) || !ParentPath(path,disk)) continue;
        if(EFI_ERROR(refit_call3_wrapper(BS->HandleProtocol,handles[i],&guid,(VOID **)&nvme)) || !nvme->Mode) continue;
        align=nvme->Mode->IoAlign; if(align<1) align=1;
        if(align>65536 || (align&(align-1))) continue;
        allocation=AllocateZeroPool(4096+align); if(!allocation) break;
        buffer=(UINT8 *)(((UINTN)allocation+align-1)&~(align-1));
        SetMem(&cmd,sizeof(cmd),0); SetMem(&completion,sizeof(completion),0); SetMem(&packet,sizeof(packet),0);
        cmd.Cdw0.Opcode=6; cmd.Flags=CDW10_VALID; cmd.Cdw10=1;
        packet.CommandTimeout=10000000; packet.TransferBuffer=buffer; packet.TransferLength=4096;
        packet.QueueType=NVME_ADMIN_QUEUE; packet.NvmeCmd=&cmd; packet.NvmeCompletion=&completion;
        status=refit_call4_wrapper(nvme->PassThru,nvme,0,&packet,NULL);
        if(!EFI_ERROR(status) && packet.TransferLength>=64) { AsciiField(name,buffer+24,40,FALSE); found=name[0]!=0; }
        FreePool(allocation);
    }
    FreePool(handles); return found;
}
/* Match the nearest USB ancestor, so a hub or another enclosure cannot win. */
static BOOLEAN UsbBoxName(EFI_DEVICE_PATH *disk, CHAR16 *name) {
    EFI_GUID guid=FURRY_USB_IO_GUID,dp=EFI_DEVICE_PATH_PROTOCOL_GUID;
    EFI_HANDLE *handles=NULL; FURRY_USB_IO *usb=NULL,*candidate;
    EFI_DEVICE_PATH *path; UINTN count=0,i,best=0,n,j,attempt;
    FURRY_USB_DEVICE_DESCRIPTOR desc; EFI_STATUS status;
    UINT16 *languages=NULL,bytes=0,language=0x0409; CHAR16 *product=NULL;
    status=refit_call5_wrapper(BS->LocateHandleBuffer,ByProtocol,&guid,NULL,&count,&handles);
    LOG(1, LOG_LINE_NORMAL, L"Disk probe: USB IO locate = %r, handles=%d",status,count);
    if(EFI_ERROR(status)) return FALSE;
    for(i=0;i<count;i++) {
        if(EFI_ERROR(refit_call3_wrapper(BS->HandleProtocol,handles[i],&dp,(VOID **)&path)) || !ParentPath(path,disk)) continue;
        n=PathLength(path); if(n<=best) continue;
        if(EFI_ERROR(refit_call3_wrapper(BS->HandleProtocol,handles[i],&guid,(VOID **)&candidate)) || !candidate) continue;
        best=n; usb=candidate;
    }
    FreePool(handles);
    LOG(1, LOG_LINE_NORMAL, L"Disk probe: USB IO matching ancestor=%d",usb!=NULL);
    if(!usb || !usb->GetDeviceDescriptor) return FALSE;
    SetMem(&desc,sizeof(desc),0);
    status=refit_call2_wrapper(usb->GetDeviceDescriptor,usb,&desc);
    LOG(1, LOG_LINE_NORMAL, L"Disk probe: USB descriptor = %r, VID:PID=%04x:%04x, product index=%d",status,desc.IdVendor,desc.IdProduct,desc.StrProduct);
    if(EFI_ERROR(status) || desc.Length<sizeof(desc) || desc.DescriptorType!=1) return FALSE;
    if(usb->GetSupportedLanguages) {
        status=refit_call3_wrapper(usb->GetSupportedLanguages,usb,&languages,&bytes);
        LOG(1, LOG_LINE_NORMAL, L"Disk probe: USB languages = %r, bytes=%d",status,bytes);
        if(!EFI_ERROR(status) && languages && bytes>=2 && !(bytes&1)) language=languages[0];
        /* Language storage belongs to the USB driver: do not free it. */
    }
    if(desc.StrProduct && usb->GetStringDescriptor) {
        for(attempt=0;attempt<2;attempt++) {
            if(attempt) { if(language==0x0409) break; language=0x0409; }
            product=NULL;
            status=refit_call4_wrapper(usb->GetStringDescriptor,usb,language,desc.StrProduct,&product);
            LOG(1, LOG_LINE_NORMAL, L"Disk probe: USB product string lang=%04x = %r",language,status);
            if(!EFI_ERROR(status) && product) {
                for(j=0;j<48 && product[j];j++) {
                    if(product[j]<32 || product[j]>=127) { j=0; break; }
                    name[j]=product[j];
                }
                name[j]=0;
                while(j && name[j-1]==' ') name[--j]=0;
                if(j) { FreePool(product); StrCat(name,L" [box]");
                    LOG(1, LOG_LINE_NORMAL, L"Disk probe: USB box result = %s",name); return TRUE; }
            }
            if(product) FreePool(product);
        }
    }
    SPrint(name,64*sizeof(CHAR16),L"USB box [%04x:%04x]",desc.IdVendor,desc.IdProduct);
    LOG(1, LOG_LINE_NORMAL, L"Disk probe: USB box ID fallback = %s",name);
    return TRUE;
}
/* Log only the bounded model field, never serial numbers or disk contents. */
static VOID ModelBytes(UINT8 *data, UINTN length) {
    CHAR16 hex[128],part[4]; UINTN i;
    hex[0]=0;
    for(i=0;i<length && i<40;i++) {
        SPrint(part,sizeof(part),L"%02x",data[i]); StrCat(hex,part); StrCat(hex,L" ");
    }
    LOG(1, LOG_LINE_NORMAL, L"Disk probe: model field hex = %s",hex);
}
static VOID DiskName(EFI_HANDLE handle,EFI_DEVICE_PATH *path,CHAR16 *name) {
    EFI_GUID guid=EFI_DISK_INFO_PROTOCOL_GUID,usb=EFI_DISK_INFO_USB_INTERFACE_GUID;
    EFI_GUID ata=EFI_DISK_INFO_IDE_INTERFACE_GUID,ahci=EFI_DISK_INFO_AHCI_INTERFACE_GUID;
    EFI_GUID scsi=EFI_DISK_INFO_SCSI_INTERFACE_GUID; EFI_DISK_INFO_PROTOCOL *info=NULL;
    UINT8 data[4096]; UINT32 size; EFI_STATUS status; BOOLEAN ataFormat,inquiryFormat;
    name[0]=0;
    LOG(1, LOG_LINE_NORMAL, L"Disk probe: model lookup, transport=%s",Transport(path));
    /* NVMe DiskInfo contains namespace data, not controller model text. */
    if(CompareMem(Transport(path),L"NVMe",8)==0) {
        LOG(1, LOG_LINE_NORMAL, L"Disk probe: using NVMe Identify Controller");
        NvmeName(path,name);
    } else {
        status=refit_call3_wrapper(BS->HandleProtocol,handle,&guid,(VOID **)&info);
        LOG(1, LOG_LINE_NORMAL, L"Disk probe: HandleProtocol(DiskInfo) = %r",status);
        if(!EFI_ERROR(status) && info) {
            ataFormat=CompareGuid(&info->Interface,&ata)==0 || CompareGuid(&info->Interface,&ahci)==0;
            inquiryFormat=CompareGuid(&info->Interface,&scsi)==0 || CompareGuid(&info->Interface,&usb)==0;
            LOG(1, LOG_LINE_NORMAL, L"Disk probe: interface=%g, ATA=%d, USB/SCSI=%d, Identify=%d, Inquiry=%d",
                &info->Interface,ataFormat,inquiryFormat,info->Identify!=NULL,info->Inquiry!=NULL);
            if(ataFormat && info->Identify) {
                size=sizeof(data); SetMem(data,sizeof(data),0);
                LOG(1, LOG_LINE_NORMAL, L"Disk probe: calling Identify");
                status=refit_call3_wrapper(info->Identify,info,data,&size);
                LOG(1, LOG_LINE_NORMAL, L"Disk probe: Identify = %r, bytes=%d",status,size);
                if(!EFI_ERROR(status) && size>=94 && size<=sizeof(data)) {
                    ModelBytes(data+54,40); AsciiField(name,data+54,40,TRUE);
                    LOG(1, LOG_LINE_NORMAL, L"Disk probe: Identify model accepted=%d",name[0]!=0);
                }
            }
            if(!name[0] && inquiryFormat && info->Inquiry) {
                size=sizeof(data); SetMem(data,sizeof(data),0);
                LOG(1, LOG_LINE_NORMAL, L"Disk probe: calling Inquiry");
                status=refit_call3_wrapper(info->Inquiry,info,data,&size);
                LOG(1, LOG_LINE_NORMAL, L"Disk probe: Inquiry = %r, bytes=%d",status,size);
                if(!EFI_ERROR(status) && size>=32 && size<=sizeof(data)) {
                    ModelBytes(data+8,24); AsciiField(name,data+8,24,FALSE);
                    LOG(1, LOG_LINE_NORMAL, L"Disk probe: Inquiry model accepted=%d",name[0]!=0);
                } else if(!EFI_ERROR(status) && size>0 && size<32) {
                    LOG(1, LOG_LINE_NORMAL, L"Disk probe: short Inquiry payload (%d bytes)",size);
                    ModelBytes(data,size);
                    /* A USB firmware label may be returned instead of SCSI data.
                     * Reject binary/truncated SCSI headers; label its provenance. */
                    if(CompareGuid(&info->Interface,&usb)==0) {
                        AsciiField(name,data,size,FALSE);
                        if(name[0]) StrCat(name,L" [USB label]");
                    }
                    LOG(1, LOG_LINE_NORMAL, L"Disk probe: short USB label accepted=%d",name[0]!=0);
                }
            } else if(!name[0]) {
                LOG(1, LOG_LINE_NORMAL, L"Disk probe: Inquiry skipped (unsupported format or missing function)");
            }
        } else if(!EFI_ERROR(status)) {
            LOG(1, LOG_LINE_NORMAL, L"Disk probe: DiskInfo returned a NULL interface");
        }
    }
    if(!name[0] && CompareMem(Transport(path),L"USB",6)==0) UsbBoxName(path,name);
    if(!name[0]) StrCpy(name,L"unknown model");
    LOG(1, LOG_LINE_NORMAL, L"Disk probe: model result = %s",name);
}
/* Enumerate whole media once; partitions are not additional disks. */
VOID DynamicDisks(DYNAMIC_DEVICE_LINE emit, VOID *context) {
    EFI_GUID guid=EFI_BLOCK_IO_PROTOCOL_GUID,dp=EFI_DEVICE_PATH_PROTOCOL_GUID;
    EFI_HANDLE *handles=NULL; EFI_BLOCK_IO_PROTOCOL *io;
    EFI_DEVICE_PATH *path; UINTN count=0,i,total=0;
    UINT64 capacity; CHAR16 name[64],size[32],text[128];
    if(EFI_ERROR(refit_call5_wrapper(BS->LocateHandleBuffer,ByProtocol,&guid,NULL,&count,&handles))) {
        emit(1,L"n/a",context); return;
    }
    for(i=0;i<count;i++) {
        if(EFI_ERROR(refit_call3_wrapper(BS->HandleProtocol,handles[i],&guid,(VOID **)&io)) || !io->Media) continue;
        if(io->Media->LogicalPartition || !io->Media->MediaPresent || !io->Media->BlockSize) continue;
        path=NULL; refit_call3_wrapper(BS->HandleProtocol,handles[i],&dp,(VOID **)&path);
        capacity=0;
        if(io->Media->LastBlock<~(UINT64)0 && io->Media->LastBlock+1<=~(UINT64)0/io->Media->BlockSize)
            capacity=(io->Media->LastBlock+1)*io->Media->BlockSize;
        LOG(1, LOG_LINE_NORMAL, L"Disk probe: disk%d, transport=%s, capacity=%ld bytes, removable=%d",total+1,Transport(path),capacity,io->Media->RemovableMedia);
        if(GlobalConfig.DynamicDeviceProbe) DiskName(handles[i],path,name);
        else StrCpy(name,L"model probe disabled");
        if(capacity>=1000000000) SPrint(size,sizeof(size),L"%ld.%d GB",capacity/1000000000,(UINTN)(capacity%1000000000/100000000));
        else SPrint(size,sizeof(size),L"%ld MiB",capacity/(1024*1024));
        SPrint(text,sizeof(text),L"%s / %s / %s",name,size,Transport(path));
        if(!emit(++total,text,context)) break;
    }
    FreePool(handles);
    if(!total) emit(1,L"n/a",context);
}
