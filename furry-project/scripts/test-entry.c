/* Call the actual production entry with the ABI used by GNU-EFI's CRT. */
#include <assert.h>
#include <stdio.h>
#include "global.h"
#include "../include/refit_call_wrapper.h"
extern EFI_STATUS REFIT_ENTRYAPI efi_main(EFI_HANDLE,EFI_SYSTEM_TABLE *);
static EFI_SYSTEM_TABLE system_table;
static EFI_BOOT_SERVICES boot_services;
static EFI_RUNTIME_SERVICES runtime_services;
VOID __wrap_InitializeLib(EFI_HANDLE image, EFI_SYSTEM_TABLE *table) {
    assert(image==(EFI_HANDLE)0x1234 && table==&system_table);
    gST=table; ST=table; BS=table->BootServices; RT=table->RuntimeServices;
}
EFI_STATUS __wrap_InitRefitLib(EFI_HANDLE image) {
    assert(image==(EFI_HANDLE)0x1234);
    assert(gST==&system_table && gBS==&boot_services && gRT==&runtime_services);
    return EFI_ABORTED; // Stop before scanning disks or firmware graphics.
}
int main(void) {
    system_table.BootServices=&boot_services;
    system_table.RuntimeServices=&runtime_services;
    ST=&system_table; BS=&boot_services; RT=&runtime_services;
    assert(efi_main((EFI_HANDLE)0x1234,&system_table)==EFI_ABORTED);
    puts("PASS: production efi_main receives GNU-EFI CRT image/system-table arguments with the native ABI");
    return 0;
}
