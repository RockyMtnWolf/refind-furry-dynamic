/* Production device decoders with firmware protocol fixtures. */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#define AllocateZeroPool TestAllocateZeroPool
#include "dynamic_devices.c"
REFIT_CONFIG GlobalConfig;
EFI_FILE_HANDLE SelfDir;
CHAR16 *gLogTemp;
VOID WriteToLog(CHAR16 **s,UINTN t) { (void)s;(void)t; }
VOID *AllocatePool(UINTN n) { return malloc(n); }
VOID *AllocateZeroPool(UINTN n) { return calloc(1,n); }
VOID FreePool(VOID *p) { free(p); }
EFI_STATUS egLoadFile(EFI_FILE_HANDLE dir,CHAR16 *path,UINT8 **data,UINTN *size) {
    FILE *f=fopen("furry-project/theme/gpu-names.txt","rb"); (void)dir;(void)path;
    assert(f); fseek(f,0,SEEK_END); *size=ftell(f); rewind(f);
    *data=malloc(*size); assert(fread(*data,1,*size,f)==*size); fclose(f); return EFI_SUCCESS;
}
static EFI_DISK_INFO_PROTOCOL usbInfo;
static FURRY_USB_IO usbIo;
static BOOLEAN usbAvailable=FALSE, usbWrongPath=FALSE, usbStringFails=FALSE;
static UINTN usbStringCalls;
static EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL nvme;
static EFI_NVM_EXPRESS_PASS_THRU_MODE mode;
static UINT8 nvPath[]={1,1,4,0,3,0x17,4,0,0x7f,0xff,4,0};
static UINT8 controllerPath[]={1,1,4,0,0x7f,0xff,4,0};
static UINT8 usbPath[]={3,5,4,0,0x7f,0xff,4,0};
static UINTN nvmeCalls,inquiryCalls,diskInfoCalls;
static EFI_STATUS diskStatus=EFI_SUCCESS, inquiryStatus=EFI_SUCCESS;
static UINT32 inquirySize=36;
static BOOLEAN binaryModel=FALSE,shortLabel=FALSE;
static VOID EFIAPI copy(VOID *d,VOID *s,UINTN n) { memcpy(d,s,n); }
static VOID EFIAPI set(VOID *d,UINTN n,UINT8 v) { memset(d,v,n); }
static EFI_STATUS EFIAPI inquiry(EFI_DISK_INFO_PROTOCOL *this,VOID *data,UINT32 *n) {
    (void)this; assert(*n>=36); memset(data,0,36);
    memcpy((UINT8 *)data+8,"Lexar   1TB SSD         ",24); if(shortLabel) { memset(data,0,36); memcpy(data,"RTL9210B-CG",11); }
    *n=inquirySize; if(binaryModel) ((UINT8 *)data)[8]=0xff; inquiryCalls++; return inquiryStatus;
}
static EFI_STATUS EFIAPI locate(EFI_LOCATE_SEARCH_TYPE t,EFI_GUID *g,VOID *key,UINTN *n,EFI_HANDLE **h) {
    EFI_GUID expected=EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL_GUID;
    EFI_GUID usb=FURRY_USB_IO_GUID;
    (void)t;(void)key;
    if(CompareGuid(g,&usb)==0) {
        if(!usbAvailable) return EFI_NOT_FOUND;
        *n=1; *h=malloc(sizeof(EFI_HANDLE)); (*h)[0]=(EFI_HANDLE)4; return EFI_SUCCESS;
    }
    assert(CompareGuid(g,&expected)==0);
    *n=1; *h=malloc(sizeof(EFI_HANDLE)); (*h)[0]=(EFI_HANDLE)2; return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI handle(EFI_HANDLE h,EFI_GUID *g,VOID **out) {
    EFI_GUID disk=EFI_DISK_INFO_PROTOCOL_GUID,dp=EFI_DEVICE_PATH_PROTOCOL_GUID,usb=FURRY_USB_IO_GUID;
    if(CompareGuid(g,&disk)==0) { assert(h==(EFI_HANDLE)1); diskInfoCalls++; *out=&usbInfo; return diskStatus; }
    else if(CompareGuid(g,&usb)==0) *out=&usbIo;
    else if(CompareGuid(g,&dp)==0) *out=h==(EFI_HANDLE)4?(usbWrongPath?controllerPath:usbPath):controllerPath;
    else *out=&nvme;
    return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI pass(EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL *this,UINT32 ns,EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET *packet,EFI_EVENT event) {
    (void)this;(void)event; assert(ns==0 && packet->NvmeCmd->Cdw0.Opcode==6 && packet->NvmeCmd->Cdw10==1);
    assert(packet->TransferLength==4096 && packet->CommandTimeout==10000000);
    memset(packet->TransferBuffer,0,4096);
    memset((UINT8 *)packet->TransferBuffer+24,' ',40);
    memcpy((UINT8 *)packet->TransferBuffer+24,"Samsung SSD 9100 PRO 4TB",24);
    nvmeCalls++; return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI descriptor(FURRY_USB_IO *this,FURRY_USB_DEVICE_DESCRIPTOR *desc) {
    (void)this; memset(desc,0,sizeof(*desc)); desc->Length=18; desc->DescriptorType=1;
    desc->IdVendor=0x0bda; desc->IdProduct=0x9210; desc->StrProduct=2; return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI language(FURRY_USB_IO *this,UINT16 **ids,UINT16 *n) {
    static UINT16 langs[]={0x0409}; (void)this; *ids=langs; *n=2; return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI string(FURRY_USB_IO *this,UINT16 lang,UINT8 index,CHAR16 **out) {
    (void)this; assert(lang==0x0409 && index==2); usbStringCalls++;
    if(usbStringFails) return EFI_NOT_FOUND;
    *out=malloc(sizeof(L"RTL9210B-CG")); memcpy(*out,L"RTL9210B-CG",sizeof(L"RTL9210B-CG")); return EFI_SUCCESS;
}
int main(void) {
    EFI_BOOT_SERVICES services={0}; CHAR16 text[64]; UINT8 data[40]; UINTN i;
    assert(sizeof(FURRY_USB_DEVICE_DESCRIPTOR)==18);
    assert(offsetof(FURRY_USB_IO,GetDeviceDescriptor)==6*sizeof(void (*)(void)));
    assert(offsetof(FURRY_USB_IO,GetStringDescriptor)==10*sizeof(void (*)(void)));
    BS=&services; services.CopyMem=copy; services.SetMem=set;
    services.LocateHandleBuffer=locate; services.HandleProtocol=handle;
    usbInfo.Interface=(EFI_GUID)EFI_DISK_INFO_USB_INTERFACE_GUID; usbInfo.Inquiry=inquiry;
    nvme.Mode=&mode; mode.IoAlign=8; nvme.PassThru=pass;
    memset(data,0xff,sizeof(data)); data[37]='\''; data[38]='}'; AsciiField(text,data,40,FALSE); assert(!text[0]);
    memset(data,' ',40); memcpy(data,"Samsung SSD",11); AsciiField(text,data,40,FALSE); assert(StrCmp(text,L"Samsung SSD")==0);
    for(i=0;i<40;i+=2) { UINT8 c=data[i]; data[i]=data[i+1]; data[i+1]=c; }
    AsciiField(text,data,40,TRUE); assert(StrCmp(text,L"Samsung SSD")==0);
    assert(GpuName(0x10de,0x28b0,0x10de,0x1870,text)); assert(StrCmp(text,L"NVIDIA RTX 2000 Ada")==0);
    assert(GpuName(0x10de,0x28b0,0x10de,0x1871,text)); assert(StrCmp(text,L"NVIDIA RTX 2000E Ada")==0);
    assert(GpuName(0x1002,0x13c0,0,0,text)); assert(StrCmp(text,L"AMD Radeon Graphics (Granite Ridge)")==0);
    DiskName((EFI_HANDLE)1,(EFI_DEVICE_PATH *)usbPath,text); assert(StrCmp(text,L"Lexar   1TB SSD")==0);
    assert(inquiryCalls==1 && diskInfoCalls==1);
    DiskName((EFI_HANDLE)3,(EFI_DEVICE_PATH *)nvPath,text); assert(StrCmp(text,L"Samsung SSD 9100 PRO 4TB")==0);
    assert(nvmeCalls==1 && diskInfoCalls==1); // Never decode namespace DiskInfo as a model.
    inquiryStatus=EFI_UNSUPPORTED;
    DiskName((EFI_HANDLE)1,(EFI_DEVICE_PATH *)usbPath,text); assert(StrCmp(text,L"unknown model")==0);
    inquiryStatus=EFI_SUCCESS; inquirySize=20;
    DiskName((EFI_HANDLE)1,(EFI_DEVICE_PATH *)usbPath,text); assert(StrCmp(text,L"unknown model")==0);
    inquirySize=36; binaryModel=TRUE;
    DiskName((EFI_HANDLE)1,(EFI_DEVICE_PATH *)usbPath,text); assert(StrCmp(text,L"unknown model")==0);
    binaryModel=FALSE; shortLabel=TRUE; inquirySize=11;
    DiskName((EFI_HANDLE)1,(EFI_DEVICE_PATH *)usbPath,text); assert(StrCmp(text,L"RTL9210B-CG [USB label]")==0);
    usbInfo.Interface=(EFI_GUID)EFI_DISK_INFO_SCSI_INTERFACE_GUID;
    DiskName((EFI_HANDLE)1,(EFI_DEVICE_PATH *)usbPath,text); assert(StrCmp(text,L"unknown model")==0);
    usbInfo.Interface=(EFI_GUID)EFI_DISK_INFO_USB_INTERFACE_GUID;
    shortLabel=FALSE;
    DiskName((EFI_HANDLE)1,(EFI_DEVICE_PATH *)usbPath,text); assert(StrCmp(text,L"unknown model")==0);
    diskStatus=EFI_UNSUPPORTED;
    DiskName((EFI_HANDLE)1,(EFI_DEVICE_PATH *)usbPath,text); assert(StrCmp(text,L"unknown model")==0);
    diskStatus=EFI_SUCCESS; usbInfo.Inquiry=NULL;
    DiskName((EFI_HANDLE)1,(EFI_DEVICE_PATH *)usbPath,text); assert(StrCmp(text,L"unknown model")==0);
    usbAvailable=TRUE; usbIo.GetDeviceDescriptor=descriptor;
    usbIo.GetSupportedLanguages=language; usbIo.GetStringDescriptor=string;
    DiskName((EFI_HANDLE)1,(EFI_DEVICE_PATH *)usbPath,text); assert(StrCmp(text,L"RTL9210B-CG [box]")==0);
    assert(usbStringCalls==1);
    usbWrongPath=TRUE;
    DiskName((EFI_HANDLE)1,(EFI_DEVICE_PATH *)usbPath,text); assert(StrCmp(text,L"unknown model")==0);
    assert(usbStringCalls==1); // Unrelated USB handles cannot supply the name.
    usbWrongPath=FALSE; usbStringFails=TRUE;
    DiskName((EFI_HANDLE)1,(EFI_DEVICE_PATH *)usbPath,text); assert(StrCmp(text,L"USB box [0BDA:9210]")==0);
    puts("PASS: USB descriptor fallback, correct path matching, missing string ID fallback");
    puts("PASS: USB missing protocol/function, failed Inquiry, short/binary results handled safely");
    puts("PASS: USB Inquiry, NVMe controller model, binary rejection, ATA byte order, GPU subsystem names");
}
