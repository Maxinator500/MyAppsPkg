#pragma once

extern "C"
{
  #include <Uefi.h>
  #include <PiDxe.h> 
  #include <Library/UefiLib.h>
  #include <Library/UefiApplicationEntryPoint.h>
  #include <Library/UefiBootServicesTableLib.h>
  #include <Protocol/LoadedImage.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/PrintLib.h>
  #include <IndustryStandard/SmBios.h>
  #include <Protocol/Smbios.h>
}

EFI_BOOT_SERVICES    *_gBS = NULL;
EFI_RUNTIME_SERVICES *_gRS = NULL;

const UINT8 _FVHSIG[] = { 0x5F, 0x46, 0x56, 0x48 };

static VOID
DisplayUpdateProgressSREP(
  IN UINTN Completion
)
{
  UINTN Index;
  UINTN CurrentAttribute = (UINTN)gST->ConOut->Mode->Attribute;;

  //
  // Check range
  //
  if (Completion > 100) {
    return;
  }

  //
  // Print progress percentage
  //
  AsciiPrint("\r %3d%% ", Completion);

  //
  // Set progress bar color
  //
  gST->ConOut->SetAttribute (
                 gST->ConOut,
                 EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK)
                 );

  //
  // Print completed portion of progress bar
  //
  for (Index = 0; Index < Completion / 2; Index++){
    AsciiPrint("%c", BLOCKELEMENT_FULL_BLOCK);
  }

  //
  // Restore text color
  //
  gST->ConOut->SetAttribute(gST->ConOut, CurrentAttribute);

  //
  // Print remaining portion of progress bar
  //
  for ( ; Index < 50; Index++) {
    AsciiPrint("%c", BLOCKELEMENT_LIGHT_SHADE);
  }

  return;
}
