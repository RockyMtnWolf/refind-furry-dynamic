#include <efi.h>
#include <efilib.h>
EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *system) {
    EFI_INPUT_KEY key; UINTN event;
    InitializeLib(image,system);
    uefi_call_wrapper(system->ConOut->ClearScreen,1,system->ConOut);
    Print(L"Test ESP demonstration loader. No operating system installed.\r\nPress any key to return to rEFInd.\r\n");
    uefi_call_wrapper(system->BootServices->WaitForEvent,3,1,&system->ConIn->WaitForKey,&event);
    uefi_call_wrapper(system->ConIn->ReadKeyStroke,2,system->ConIn,&key);
    return EFI_SUCCESS;
}
