/* Verify submenu initialization survives replacing its background image. */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "menu.c"
REFIT_CONFIG GlobalConfig;
UINTN UGAWidth=1920, UGAHeight=1080, ConHeight=25;
BOOLEAN AllowGraphicsMode=TRUE;
EG_PIXEL MenuBackgroundPixel={10,20,30,0};
CHAR16 *gLogTemp;
VOID *AllocatePool(UINTN n) { return malloc(n); }
CHAR16 *TestStrDuplicate(const CHAR16 *s) { UINTN n=(StrLen(s)+1)*sizeof(CHAR16); CHAR16 *p=malloc(n); assert(p); memcpy(p,s,n); return p; }
VOID FreePool(VOID *p) { free(p); }
VOID MyFreePool(VOID *p) { free(p); }
VOID WriteToLog(CHAR16 **s, UINTN t) { (void)s; (void)t; }
UINTN egGetFontCellWidth(VOID) { return 8; }
UINTN egGetFontHeight(VOID) { return 16; }
UINTN egComputeTextWidth(CHAR16 *s) { return StrLen(s)*8; }
VOID egMeasureText(CHAR16 *s,UINTN *w,UINTN *h) { if(w) *w=StrLen(s)*8; if(h) *h=16; }
VOID egRenderText(CHAR16 *s,EG_IMAGE *i,UINTN x,UINTN y,UINT8 b) { (void)s;(void)i;(void)x;(void)y;(void)b; }
VOID egDrawImage(EG_IMAGE *i,UINTN x,UINTN y) {
    (void)x; (void)y; assert(i && i->PixelData[0].r==30); // Original color survives clear.
}
VOID egDrawImageWithTransparency(EG_IMAGE *i,EG_IMAGE *badge,UINTN x,UINTN y,UINTN w,UINTN h) { (void)i;(void)badge;(void)x;(void)y;(void)w;(void)h; }
VOID BltImageAlpha(EG_IMAGE *i,UINTN x,UINTN y,EG_PIXEL *p) { (void)i;(void)x;(void)y;(void)p; }
VOID SwitchToGraphicsAndClear(VOID) {
    EG_PIXEL replacement={100,100,100,0};
    egFreeImage(GlobalConfig.ScreenBackground);
    GlobalConfig.ScreenBackground=egCreateFilledImage(8,8,FALSE,&replacement);
    assert(GlobalConfig.ScreenBackground);
}
int main(void) {
    SCROLL_STATE state;
    REFIT_MENU_ENTRY entry={.Title=L"Return",.Row=1};
    REFIT_MENU_ENTRY *entries[]={&entry};
    REFIT_MENU_SCREEN screen={.Title=L"About",.EntryCount=1,.Entries=entries};
    GlobalConfig.ScreenBackground=egCreateFilledImage(8,8,FALSE,&MenuBackgroundPixel);
    GraphicsMenuStyle(&screen,&state,MENU_FUNCTION_INIT,NULL);
    egFreeImage(GlobalConfig.ScreenBackground);
    {
        REFIT_MENU_ENTRY ubuntu={.Title=L"Ubuntu",.Row=0};
        REFIT_MENU_ENTRY windows={.Title=L"Windows",.Row=0};
        REFIT_MENU_ENTRY microsoft={.Title=L"Microsoft EFI boot",.Row=0};
        REFIT_MENU_ENTRY *loaders[]={&ubuntu,&windows,&microsoft};
        REFIT_MENU_SCREEN menu={.EntryCount=3,.Entries=loaders};
        assert(FindMenuShortcutEntry(&menu,L"Windows,Microsoft")==1);
        menu.EntryCount=1;
        assert(FindMenuShortcutEntry(&menu,L"Windows,Microsoft")==-1); // Keep initial first-entry selection.
        menu.EntryCount=2; loaders[1]=&microsoft;
        assert(FindMenuShortcutEntry(&menu,L"Windows,Microsoft")==1);
        puts("PASS: Windows priority, Microsoft alternate, no-match first-entry fallback");
    }
    puts("PASS: submenu background replacement does not access freed pixels");
    return 0;
}
