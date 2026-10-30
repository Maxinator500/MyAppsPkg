extern "C"
{
  #include <Uefi.h>
  #include <PiDxe.h> 
  #include <Library/UefiLib.h>
  #include <Library/UefiApplicationEntryPoint.h>
  #include <Library/UefiRuntimeServicesTableLib.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/ShellCEntryLib.h>
  #include <Guid/GlobalVariable.h>

  #include <sal.h>
}

INTN EFIAPI
ShellAppMain(IN UINTN Argc, IN CHAR16 **Argv)
{
  EFI_STATUS Status;

  if (Argc > 1)
  {
    if (StrCmp(Argv[1], reinterpret_cast<CHAR16 *>(L"OsIndications")) == 0)
    {
	    UINT64 		OsIndications = EFI_OS_INDICATIONS_BOOT_TO_FW_UI;
	    UINTN 		DataSize = sizeof(UINT64);
	    UINT32    OsIndicationsAttributes = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;

      Status = gRT->GetVariable(reinterpret_cast<CHAR16 *>(L"OsIndications"),
	      &gEfiGlobalVariableGuid,
	      &OsIndicationsAttributes,
	      &DataSize,
	      (VOID *)&OsIndications
      );

      if(!EFI_ERROR(Status) && DataSize)
      {
	      OsIndications = OsIndications | EFI_OS_INDICATIONS_BOOT_TO_FW_UI;
      }
      else
      {
        DataSize = sizeof(UINT64);
      }

      Status = gRT->SetVariable(reinterpret_cast<CHAR16 *>(L"OsIndications"),
        &gEfiGlobalVariableGuid,
        OsIndicationsAttributes,
        DataSize,
        (VOID *)&OsIndications
      );
    }
    else
    {
      AsciiPrint("Unknown argument: %S\n", Argv[1]);
      Status = EFI_INVALID_PARAMETER;
    }
  }

  gRT->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);;

  return Status;
}
