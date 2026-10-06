/* SPDX-License-Identifier: GPL-3.0-or-later
 * Collect firmware information during menu redraw; assets contain no values.
 */
#include "global.h"
#include "screen.h"
#include "libeg.h"
#include "dynamic_info.h"
#include "../include/refit_call_wrapper.h"

static EFI_GUID Smbios2 = {0xeb9d2d31,0x2d88,0x11d3,{0x9a,0x16,0x00,0x90,0x27,0x3f,0xc1,0x4d}};
static EFI_GUID Smbios3 = {0xf2fd1544,0x9794,0x4a2c,{0x99,0x2e,0xe5,0xbb,0xcf,0x20,0xe3,0x94}};
static UINT16 Read16(UINT8 *p) { UINT16 v; CopyMem(&v,p,2); return v; }
static UINT32 Read32(UINT8 *p) { UINT32 v; CopyMem(&v,p,4); return v; }
static UINT64 Read64(UINT8 *p) { UINT64 v; CopyMem(&v,p,8); return v; }
static BOOLEAN Checksum(UINT8 *p, UINTN n) {
    UINT8 sum=0; UINTN i; for(i=0;i<n;i++) sum+=p[i]; return sum==0;
}
/* Firmware owns these tables. Bound formatted records and string sets. */
static UINT64 SmbiosMemory(VOID) {
    UINT8 *table=NULL,*ep,*p,*end,*next; UINTN i,len=0,count=0;
    UINT64 total=0; UINT32 size;
    for(i=0;i<ST->NumberOfTableEntries;i++) {
        ep=ST->ConfigurationTable[i].VendorTable;
        if(CompareGuid(&ST->ConfigurationTable[i].VendorGuid,&Smbios3)==0 &&
           ep && CompareMem(ep,"_SM3_",5)==0 && ep[6]>=24 && Checksum(ep,ep[6])) {
            len=Read32(ep+12); table=(UINT8 *)(UINTN)Read64(ep+16); break;
        }
        if(CompareGuid(&ST->ConfigurationTable[i].VendorGuid,&Smbios2)==0 &&
           ep && CompareMem(ep,"_SM_",4)==0 && ep[5]>=31 && Checksum(ep,ep[5]) &&
           CompareMem(ep+16,"_DMI_",5)==0 && Checksum(ep+16,15)) {
            len=Read16(ep+22); table=(UINT8 *)(UINTN)Read32(ep+24);
        }
    }
    if(!table || len<4 || len>32*1024*1024) return 0;
    p=table; end=table+len;
    while(p+4<=end && count++<65536) {
        if(p[1]<4 || p[1]>(UINTN)(end-p)) return 0;
        next=p+p[1];
        while(next+1<end && (next[0] || next[1])) next++;
        if(next+1>=end) return 0;
        if(p[0]==17 && p[1]>=14) {
            size=Read16(p+12);
            if(size==0xffff || (size==0x7fff && p[1]<32)) return 0;
            if(size==0x7fff && p[1]>=32) total+=(UINT64)(Read32(p+28)&0x7fffffff)*1024*1024;
            else if(size!=0 && size!=0xffff && size!=0x7fff)
                total+=(UINT64)(size&0x7fff)*((size&0x8000)?1024:1024*1024);
        }
        if(p[0]==127) break;
        p=next+2;
    }
    return total;
}
/* Fallback measures firmware-mapped RAM, not installed DIMM capacity. */
static UINT64 MappedMemory(VOID) {
    EFI_MEMORY_DESCRIPTOR *map=NULL,*d; EFI_STATUS status;
    UINTN bytes=0,key,stride=0,i,attempt,capacity; UINT32 version; UINT64 total=0;
    status=refit_call5_wrapper(BS->GetMemoryMap,&bytes,map,&key,&stride,&version);
    if(status!=EFI_BUFFER_TOO_SMALL || !stride) return 0;
    for(attempt=0;attempt<3;attempt++) {
        capacity=bytes+stride*8; map=AllocatePool(capacity); if(!map) return 0;
        bytes=capacity;
        status=refit_call5_wrapper(BS->GetMemoryMap,&bytes,map,&key,&stride,&version);
        if(!EFI_ERROR(status) && stride>=sizeof(EFI_MEMORY_DESCRIPTOR)) {
            for(i=0;i+stride<=bytes;i+=stride) {
                d=(EFI_MEMORY_DESCRIPTOR *)((UINT8 *)map+i);
                if(d->Type>=EfiLoaderCode && d->Type<=EfiACPIMemoryNVS)
                    total+=d->NumberOfPages*4096;
            }
            FreePool(map); return total;
        }
        FreePool(map); if(status!=EFI_BUFFER_TOO_SMALL) return 0;
    }
    return 0;
}
static VOID CpuName(CHAR16 *out) {
#if defined(__x86_64__) || defined(__i386__)
    UINT32 a,b,c,d,leaf,words[12]; UINTN i,j=0; CHAR8 *name=(CHAR8 *)words;
    __asm__ volatile("cpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):"a"(0x80000000U),"c"(0));
    if(a>=0x80000004U) {
        for(leaf=0;leaf<3;leaf++) {
            __asm__ volatile("cpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):"a"(0x80000002U+leaf),"c"(0));
            words[leaf*4]=a; words[leaf*4+1]=b; words[leaf*4+2]=c; words[leaf*4+3]=d;
        }
        for(i=0;i<48 && name[i]==' ';i++);
        for(;i<48 && name[i];i++) out[j++]=(name[i]>=32 && name[i]<127)?name[i]:'?';
        while(j && out[j-1]==' ') j--;
        out[j]=0; if(j) return;
    }
#endif
    StrCpy(out,L"Unavailable (CPUID)");
}
/* Provider extension points; never invent GPU or disk model strings. */
static CONST CHAR16 *GpuInfo(VOID) { return L"pending PCI provider"; }
static CONST CHAR16 *DiskInfo(VOID) { return L"pending Block I/O provider"; }
static VOID Line(EG_IMAGE *frame, CHAR16 *text, UINTN x, UINTN y, BOOLEAN pink) {
    EG_IMAGE *mask; EG_PIXEL clear={0,0,0,0}; UINTN i,w=egComputeTextWidth(text),h=egGetFontHeight();
    if(x>=frame->Width || y>=frame->Height || y+h>frame->Height) return;
    if(w>frame->Width-x) w=frame->Width-x;
    if(!w) return;
    mask=egCreateFilledImage(w,h,TRUE,&clear); if(!mask) return;
    egRenderText(text,mask,0,0,0);
    for(i=0;i<w*h;i++) { mask->PixelData[i].a=mask->PixelData[i].b;
        mask->PixelData[i].r=pink?244:167;
        mask->PixelData[i].g=pink?114:139; mask->PixelData[i].b=pink?182:250; }
    egComposeImage(frame,mask,x,y); egFreeImage(mask);
}
VOID DrawDynamicInfo(VOID) {
    EG_IMAGE *frame; CHAR16 cpu[64],buf[200],vendor[49];
    EFI_GUID global=EFI_GLOBAL_VARIABLE; EFI_STATUS status; UINTN size=1,i;
    UINT8 secure=0; UINT64 memory; BOOLEAN smbios; UINTN x,y,step;
    frame=egCopyScreen(); if(!frame) return;
    x=frame->Width*35/1000; y=frame->Height*55/1000; step=egGetFontHeight()+frame->Height/90;
    CpuName(cpu); memory=SmbiosMemory(); smbios=memory!=0; if(!memory) memory=MappedMemory();
    for(i=0; i<48 && ST->FirmwareVendor && ST->FirmwareVendor[i]; i++)
        vendor[i]=ST->FirmwareVendor[i]>=32 && ST->FirmwareVendor[i]<127?ST->FirmwareVendor[i]:'?';
    vendor[i]=0;
    Line(frame,L"furry // firmware boot sequence",x,y,TRUE); y+=step;
    Line(frame,L"--------------------------------------------------------------",x,y,FALSE); y+=step;
    SPrint(buf,sizeof(buf),L"> cpu     :: %s",cpu); Line(frame,buf,x,y,FALSE); y+=step;
    if(memory) SPrint(buf,sizeof(buf),L"> memory  :: %ld MiB [%s]",memory/(1024*1024),smbios?L"SMBIOS":L"EFI mapped RAM");
    else StrCpy(buf,L"> memory  :: unavailable");
    Line(frame,buf,x,y,FALSE); y+=step;
    SPrint(buf,sizeof(buf),L"> firmware:: %s / rev 0x%x",vendor,ST->FirmwareRevision); Line(frame,buf,x,y,FALSE); y+=step;
    SPrint(buf,sizeof(buf),L"> uefi    :: %d.%02d",ST->Hdr.Revision>>16,ST->Hdr.Revision&0xffff); Line(frame,buf,x,y,FALSE); y+=step;
    status=refit_call5_wrapper(RT->GetVariable,L"SecureBoot",&global,NULL,&size,&secure);
    SPrint(buf,sizeof(buf),L"> secure  :: %s",EFI_ERROR(status)?L"UNKNOWN (variable unavailable)":secure?L"ON":L"OFF");
    Line(frame,buf,x,y,TRUE); y+=step;
    SPrint(buf,sizeof(buf),L"> gpu     :: %s",GpuInfo()); Line(frame,buf,x,y,FALSE); y+=step;
    SPrint(buf,sizeof(buf),L"> disk    :: %s",DiskInfo()); Line(frame,buf,x,y,FALSE); y+=step;
    Line(frame,L"> awaiting boot target ...",x,y,TRUE);
    egDrawImage(frame,0,0); egFreeImage(frame);
}

