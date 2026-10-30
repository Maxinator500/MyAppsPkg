extern "C"
{
  #include <Uefi.h>
  #include <PiDxe.h> 
  #include <Library/UefiLib.h>
  #include <Library/UefiApplicationEntryPoint.h>
  #include <Library/UefiBootServicesTableLib.h>
  #include <Library/UefiRuntimeServicesTableLib.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/MemoryAllocationLib.h>
  #include <Library/ShellCEntryLib.h>

  #include <sal.h>
}

INTN EFIAPI
ShellAppMain(IN UINTN Argc, IN CHAR16 **Argv)
{
  if (gBS == NULL || Argc < 2) {
    AsciiPrint("Arguments count %d:\nBoot Services - %a.\n", Argc - 1, gBS == NULL ? "Not Available" : "Present");
    return EFI_INVALID_PARAMETER;
  }

  UINTN *MemOffsets = (UINTN *)AllocateZeroPool(sizeof(UINTN) * Argc);

  for (UINTN i = 0; i < Argc - 1; i++) {
    MemOffsets[i] = StrHexToUintn(Argv[i + 1]);
    if (MemOffsets[i] == 0) break;

    AsciiPrint("Offset 0x%08X = 0x%08X;\n", MemOffsets[i], ReadUnaligned32((UINTN *)(MemOffsets[i])));
  }

  FreePool(MemOffsets);
  return EFI_SUCCESS;
}
