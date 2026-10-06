/* SPDX-License-Identifier: GPL-3.0-or-later
 * Read-only PCI, Block I/O, Disk Info and NVMe Identify providers.
 * Protocol declarations in include/vendor retain their EDK II BSD license.
 */
#include "global.h"
#include "libeg.h"
#include "dynamic_devices.h"
#include "../include/refit_call_wrapper.h"
#ifdef __MAKEWITH_GNUEFI
#include <efipciio.h>
typedef EFI_DEVICE_PATH EFI_DEVICE_PATH_PROTOCOL;
#endif
#include "../include/vendor/DiskInfo.h"
#include "../include/vendor/NvmExpressPassthru.h"

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
    UINTN i,j=0; UINT8 c;
    for(i=0;i<len && j<63;i++) {
        c=in[swap?(i^1):i]; out[j++]=(c>=32 && c<127)?c:'?';
    }
    while(j && (out[j-1]==' ' || out[j-1]=='?')) j--;
    out[j]=0;
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
static BOOLEAN GpuName(UINT16 vendor,UINT16 device,CHAR16 *name) {
    UINT8 *data=NULL; UINTN size=0,i=0,j,k;
    if(EFI_ERROR(egLoadFile(SelfDir,L"themes\\finn-term\\gpu-names.txt",&data,&size))) return FALSE;
    if(size>65536) { FreePool(data); return FALSE; }
    while(i+10<size) {
        j=i; while(j<size && data[j]!='\n') j++;
        if(j-i>10 && data[i+4]==':' && data[i+9]=='|' &&
           Hex4(data+i)==vendor && Hex4(data+i+5)==device) {
            k=0; i+=10;
            while(i<j && data[i]!='\r' && k<63) {
                name[k++]=data[i]>=32 && data[i]<127?data[i]:'?'; i++;
            }
            name[k]=0; FreePool(data); return k!=0;
        }
        i=j+1;
    }
    FreePool(data); return FALSE;
}
VOID DynamicGpu(CHAR16 *text,UINTN bytes) {
    EFI_GUID guid=EFI_PCI_IO_PROTOCOL_GUID,gop=EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_HANDLE *handles=NULL; EFI_PCI_IO_PROTOCOL *pci; VOID *graphics;
    UINTN count=0,i,total=0; UINT8 config[16]; UINT16 vendor=0,device=0;
    BOOLEAN chosen=FALSE,active=FALSE; CHAR16 name[64],extra[20];
    SPrint(text,bytes,L"unavailable (PCI I/O)");
    if(EFI_ERROR(refit_call5_wrapper(BS->LocateHandleBuffer,ByProtocol,&guid,NULL,&count,&handles))) return;
    for(i=0;i<count;i++) {
        if(EFI_ERROR(refit_call3_wrapper(BS->HandleProtocol,handles[i],&guid,(VOID **)&pci))) continue;
        if(EFI_ERROR(refit_call5_wrapper(pci->Pci.Read,pci,EfiPciIoWidthUint8,0,16,config)) || config[11]!=3) continue;
        total++;
        if(!chosen || (!active && !EFI_ERROR(refit_call3_wrapper(BS->HandleProtocol,handles[i],&gop,&graphics)))) {
            vendor=config[0]|((UINT16)config[1]<<8); device=config[2]|((UINT16)config[3]<<8);
            active=!EFI_ERROR(refit_call3_wrapper(BS->HandleProtocol,handles[i],&gop,&graphics)); chosen=TRUE;
        }
    }
    FreePool(handles); if(!chosen) return;
    if(!GpuName(vendor,device,name)) {
        SPrint(name,sizeof(name),L"%s display controller",vendor==0x10de?L"NVIDIA":vendor==0x1002?L"AMD":vendor==0x8086?L"Intel":L"PCI");
    }
    extra[0]=0; if(total>1) SPrint(extra,sizeof(extra),L" +%d",total-1);
    SPrint(text,bytes,L"%s [%04x:%04x]%s",name,vendor,device,extra);
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
static VOID DiskName(EFI_HANDLE handle,EFI_DEVICE_PATH *path,CHAR16 *name) {
    EFI_GUID guid=EFI_DISK_INFO_PROTOCOL_GUID,nv=EFI_DISK_INFO_NVME_INTERFACE_GUID;
    EFI_GUID ata=EFI_DISK_INFO_IDE_INTERFACE_GUID,ahci=EFI_DISK_INFO_AHCI_INTERFACE_GUID;
    EFI_GUID scsi=EFI_DISK_INFO_SCSI_INTERFACE_GUID; EFI_DISK_INFO_PROTOCOL *info; UINT8 data[4096]; UINT32 size;
    name[0]=0;
    if(!EFI_ERROR(refit_call3_wrapper(BS->HandleProtocol,handle,&guid,(VOID **)&info))) {
        size=sizeof(data);
        if(info->Identify && !EFI_ERROR(refit_call3_wrapper(info->Identify,info,data,&size))) {
            if(CompareGuid(&info->Interface,&nv)==0 && size>=64) AsciiField(name,data+24,40,FALSE);
            else if((CompareGuid(&info->Interface,&ata)==0 || CompareGuid(&info->Interface,&ahci)==0) && size>=94)
                AsciiField(name,data+54,40,TRUE);
        }
        if(!name[0] && CompareGuid(&info->Interface,&scsi)==0 && info->Inquiry) {
            size=sizeof(data);
            if(!EFI_ERROR(refit_call3_wrapper(info->Inquiry,info,data,&size)) && size>=32) AsciiField(name,data+8,24,FALSE);
        }
    }
    if(!name[0] && CompareMem(Transport(path),L"NVMe",8)==0) NvmeName(path,name);
    if(!name[0]) StrCpy(name,L"model unavailable");
}
VOID DynamicDisk(CHAR16 *text,UINTN bytes) {
    EFI_GUID guid=EFI_BLOCK_IO_PROTOCOL_GUID,dp=EFI_DEVICE_PATH_PROTOCOL_GUID;
    EFI_HANDLE *handles=NULL,selected=NULL; EFI_BLOCK_IO_PROTOCOL *io;
    EFI_DEVICE_PATH *path,*chosenPath=NULL,*boot=NULL; UINTN count=0,i,total=0,rank,best=0;
    UINT64 capacity=0; CHAR16 name[64],size[32],extra[20];
    SPrint(text,bytes,L"unavailable (Block I/O)");
    refit_call3_wrapper(BS->HandleProtocol,SelfLoadedImage->DeviceHandle,&dp,(VOID **)&boot);
    if(EFI_ERROR(refit_call5_wrapper(BS->LocateHandleBuffer,ByProtocol,&guid,NULL,&count,&handles))) return;
    for(i=0;i<count;i++) {
        if(EFI_ERROR(refit_call3_wrapper(BS->HandleProtocol,handles[i],&guid,(VOID **)&io)) || !io->Media) continue;
        if(io->Media->LogicalPartition || !io->Media->MediaPresent || !io->Media->BlockSize) continue;
        path=NULL; refit_call3_wrapper(BS->HandleProtocol,handles[i],&dp,(VOID **)&path);
        total++; rank=io->Media->RemovableMedia?1:2; if(ParentPath(path,boot)) rank+=4;
        if(rank>best) {
            selected=handles[i]; chosenPath=path; best=rank; capacity=0;
            if(io->Media->LastBlock<~(UINT64)0 && io->Media->LastBlock+1<=~(UINT64)0/io->Media->BlockSize)
                capacity=(io->Media->LastBlock+1)*io->Media->BlockSize;
        }
    }
    FreePool(handles); if(!selected) return;
    DiskName(selected,chosenPath,name);
    if(capacity>=1000000000) SPrint(size,sizeof(size),L"%ld.%d GB",capacity/1000000000,(UINTN)(capacity%1000000000/100000000));
    else SPrint(size,sizeof(size),L"%ld MiB",capacity/(1024*1024));
    extra[0]=0; if(total>1) SPrint(extra,sizeof(extra),L" +%d disks",total-1);
    SPrint(text,bytes,L"%s / %s [%s]%s",name,size,Transport(chosenPath),extra);
}
