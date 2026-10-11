/* Exercise real maximum-mode selection with a mock GOP protocol. */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "../../libeg/screen.c"
REFIT_CONFIG GlobalConfig;
CHAR16 *gLogTemp;
static EFI_GRAPHICS_OUTPUT_MODE_INFORMATION modes[4];
static EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE mode;
static EFI_GRAPHICS_OUTPUT_PROTOCOL gop;
VOID *AllocatePool(UINTN n) { return malloc(n); }
VOID FreePool(VOID *p) { free(p); }
VOID WriteToLog(CHAR16 **s, UINTN t) { (void)s; (void)t; }
VOID SwitchToText(BOOLEAN cursor) { (void)cursor; assert(0); }
VOID SwitchToGraphics(VOID) { assert(0); }
VOID PauseForKey(VOID) { assert(0); }
static EFI_STATUS EFIAPI query(EFI_GRAPHICS_OUTPUT_PROTOCOL *self,UINT32 n,UINTN *size,EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **info) {
    assert(self==&gop); if(n>=4) return EFI_INVALID_PARAMETER;
    *size=sizeof(**info); *info=malloc(*size); assert(*info); **info=modes[n]; return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI set(EFI_GRAPHICS_OUTPUT_PROTOCOL *self,UINT32 n) {
    assert(self==&gop && n<4); mode.Mode=n; mode.Info=&modes[n]; return EFI_SUCCESS;
}
int main(void) {
    modes[0].HorizontalResolution=1280; modes[0].VerticalResolution=1024;
    modes[1].HorizontalResolution=1920; modes[1].VerticalResolution=1080;
    modes[2].HorizontalResolution=2560; modes[2].VerticalResolution=1080;
    modes[3].HorizontalResolution=3840; modes[3].VerticalResolution=2160;
    mode.MaxMode=4; mode.Mode=0; mode.Info=&modes[0];
    gop.Mode=&mode; gop.QueryMode=query; gop.SetMode=set; GraphicsOutput=&gop;
    egSetMaxResolution(); assert(mode.Mode==3 && egScreenWidth==3840 && egScreenHeight==2160);
    egSetMaxResolution(); assert(mode.Mode==3); // Already selected.
    mode.MaxMode=3; mode.Mode=0; mode.Info=&modes[0];
    egSetMaxResolution(); assert(mode.Mode==2 && egScreenWidth==2560 && egScreenHeight==1080);
    GraphicsOutput=NULL; egSetMaxResolution(); // No GOP: leave mode intact.
    puts("PASS: maximum GOP selection, equal-height modes, repeated selection, missing GOP, UINTN mode writes");
    return 0;
}
