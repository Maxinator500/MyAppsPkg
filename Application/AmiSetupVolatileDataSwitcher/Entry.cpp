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

STATIC EFI_GUID SetupVolatileDataGuid = { 0xec87d643,0xeba4,0x4bb5,{0xa1,0xe5,0x3f,0x3e,0x36,0xb2,0x0d,0xa9} };

INTN EFIAPI
ShellAppMain(IN UINTN Argc, IN CHAR16 **Argv)
{
  //Debug
  //Argc = 2;
  UINTN Offset = StrHexToUintn(Argv[1]);

  if (gRT == NULL || gRT->SetVariable == NULL || Argc < 2) return EFI_INVALID_PARAMETER;

  CHAR16 *Name = reinterpret_cast<CHAR16 *>(L"SetupVolatileData");
  //CHAR16 *Name = reinterpret_cast<CHAR16 *>(L"fjSetupHiddenAdminPage"); //{0x77d054d6,0x53c6,0x4215,{0xb9,0xc3,0x66,0x63,0x7f,0xca,0xb7,0xce}}
  //CHAR16 *Name = reinterpret_cast<CHAR16 *>(L"SystemAccess"); //{0xe770bb69,0xbcb4,0x4d04,{0x9e,0x97,0x23,0xff,0x94,0x56,0xfe,0xac}}

  UINTN DataSize = 0;
  UINT32 Attributes;
  CHAR8 *Data = NULL;

  EFI_STATUS Status = gRT->GetVariable(
    Name,
    &SetupVolatileDataGuid,
    &Attributes,
    &DataSize,
    NULL
  );
  if (Offset > DataSize) Status = EFI_INVALID_PARAMETER;

  if (Status == EFI_BUFFER_TOO_SMALL || Status == EFI_SUCCESS) {
    Data = (CHAR8 *)AllocateZeroPool(DataSize);

    gRT->GetVariable(
      Name,
      &SetupVolatileDataGuid,
      &Attributes,
      &DataSize,
      Data
    );
    Status = gRT->SetVariable(
      Name,
      &SetupVolatileDataGuid,
      0,
      0,
      NULL
    );

    if (EFI_ERROR(Status)) {
      AsciiPrint("Couldn't delete (invalidate) %s, switch aborted.\n", Name);
      return Status;
    }
  }
  else
  {
    if (DataSize) {
      Status = gRT->SetVariable(
        Name,
        &SetupVolatileDataGuid,
        0,
        0,
        NULL
      );
      if (EFI_ERROR(Status)) {
        AsciiPrint("Couldn't delete (invalidate) %s, switch aborted.\n", Name);
        return Status;
      }
    }

    if (!EFI_ERROR(Status)) {
      AsciiPrint("%s' size 0x%X is smaller than 0x%X,\ngoing to create var bigger than original.\n", Name, DataSize, Offset);
    }
    else
    {
      AsciiPrint("%s doesn't exist,\ngoing to create a new var with DataSize 0x%X nevertheless.\n", Name, Offset);
    }

    DataSize = Offset;
    Data = (CHAR8 *)AllocateZeroPool(DataSize);
  }

  Status = gRT->GetVariable(
    Name,
    &SetupVolatileDataGuid,
    &Attributes,
    &DataSize,
    Data
  );

  Data[Offset] = ((BOOLEAN)(Data[Offset] != 0)) ^ 1;

  //Debug
  //Data[Offset] = 0x55;
  AsciiPrint("%s GetVariable after delete status - %r.\n", Name, Status);

  Status = gRT->SetVariable(
    Name,
    &SetupVolatileDataGuid,
    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    DataSize,
    Data
  );

  if (!EFI_ERROR(Status)) {
    AsciiPrint("Set %s[0x%X] = %X - %r.\n", Name, Offset, Data[Offset], Status);
  }
  else
  {
    AsciiPrint("Failed setting %s:\nSetVariable - %r.\n", Name, Status);
  }

  FreePool(Data);
  return Status;
}
