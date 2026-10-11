/* Run the actual font renderer against small destinations under ASan. */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define egPrepareEmbeddedImage TestPrepareEmbeddedImage
#include "text.c"
#undef egPrepareEmbeddedImage
#include "dynamic_info.c"
REFIT_CONFIG GlobalConfig;
CHAR16 *gLogTemp;
VOID WriteToLog(CHAR16 **s, UINTN t) { (void)s; (void)t; }
static UINTN gpuCalls, diskCalls;
VOID DynamicGpus(DYNAMIC_DEVICE_LINE emit, VOID *context) { assert(GlobalConfig.DynamicDeviceProbe); gpuCalls++; assert(emit(1,L"Mock GPU 1",context)); emit(2,L"Mock GPU 2",context); }
VOID DynamicDisks(DYNAMIC_DEVICE_LINE emit, VOID *context) { assert(GlobalConfig.DynamicDeviceProbe); diskCalls++; assert(emit(1,L"Mock SSD 1",context)); emit(2,L"Mock SSD 2",context); }
EG_IMAGE *egCopyScreen(VOID) { EG_PIXEL black={0,0,0,0}; return egCreateFilledImage(1920,1080,FALSE,&black); }
VOID egDrawImage(EG_IMAGE *i, UINTN x, UINTN y) { (void)x;(void)y; assert(i && i->Width==1920); }

VOID *AllocatePool(UINTN n) { return malloc(n); }
VOID FreePool(VOID *p) { free(p); }
VOID MyFreePool(VOID *p) { free(p); }
VOID egGetScreenSize(UINTN *w, UINTN *h) { *w=1920; *h=1080; }
EG_IMAGE *TestPrepareEmbeddedImage(EG_EMBEDDED_IMAGE *i, BOOLEAN alpha) { (void)i; (void)alpha; return NULL; }
static VOID EFIAPI copy(VOID *dest, VOID *src, UINTN n) { memcpy(dest,src,n); }
static EFI_STATUS EFIAPI memory_map(UINTN *n, EFI_MEMORY_DESCRIPTOR *m, UINTN *k, UINTN *stride, UINT32 *v) {
    (void)n;(void)m;(void)k;(void)stride;(void)v; return EFI_UNSUPPORTED;
}
static EFI_STATUS EFIAPI variable(CHAR16 *name,EFI_GUID *guid,UINT32 *attrs,UINTN *n,VOID *data) {
    (void)name;(void)guid;(void)attrs; assert(*n==1); *(UINT8 *)data=1; return EFI_SUCCESS;
}
int main(void) {
    EFI_BOOT_SERVICES services={0}; services.CopyMem=copy; BS=&services;
    EG_PIXEL white={255,255,255,255}, black={0,0,0,0};
    EG_IMAGE *target;
    BaseFontImage=egCreateFilledImage(8*FONT_NUM_CHARS,16,TRUE,&white);
    assert(BaseFontImage);
    target=egCreateFilledImage(8,4,FALSE,&black); assert(target);
    egRenderText(L"A",target,0,2,255); // Only two rows may be rendered.
    assert(target->PixelData[0].r==0 && target->PixelData[16].r==255);
    egRenderText(L"A",target,9,0,255); // Out-of-range X must not underflow.
    egRenderText(L"A",target,0,5,255); // Out-of-range Y must not write.
    egRenderText(L"AA",target,0,0,255); // Horizontal clipping.
    egFreeImage(target);
    target=egCreateFilledImage(100,40,FALSE,&black); assert(target);
    Line(target,L"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",5,2,FALSE);
    assert(target->PixelData[14*100+5].r==0); // 16px atlas becomes a 12px panel row.
    Line(target,L"A",99,0,FALSE); // Panel edge must not underflow.
    Line(target,L"A",5,35,FALSE); // Scaled row must fit vertically.
    egFreeImage(target);
    {
        EFI_SYSTEM_TABLE system={0}; EFI_RUNTIME_SERVICES runtime={0};
        EFI_CONFIGURATION_TABLE invalid={0};
        invalid.VendorGuid=Smbios3; invalid.VendorTable=(VOID *)1;
        system.NumberOfTableEntries=1; system.ConfigurationTable=&invalid;
        system.FirmwareVendor=L"Mock UEFI"; ST=&system;
        services.GetMemoryMap=memory_map; runtime.GetVariable=variable; RT=&runtime;
        assert(!GlobalConfig.DynamicSmbios && !GlobalConfig.DynamicDeviceProbe);
        DrawDynamicInfo(); // Must not dereference the invalid SMBIOS or probe devices.
        assert(gpuCalls==0 && diskCalls==0);
        GlobalConfig.DynamicDeviceProbe=TRUE;
        DrawDynamicInfo();
        assert(gpuCalls==1 && diskCalls==1);
    }
    egFreeImage(BaseFontImage); egFreeImage(DarkFontImage); egFreeImage(LightFontImage);
    puts("PASS: compatibility overlay ignores raw SMBIOS/device probes and handles unavailable memory map");
    puts("PASS: actual font renderer clips vertical/horizontal bounds and rejects off-image origins");
    return 0;
}
