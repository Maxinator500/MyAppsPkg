#include "AmiSmiNVWriter.hpp"

/**
    Description of the VARIABLE_BLOCK variables.

 Fields:
 Name Type Description

 VendorGuid EFI_GUID A unique identifier for the vendor.
 VariableName UINT32 32-bit physical address of the null-terminated Unicode string that is the name of the vendor's variable. The caller must ensure natural alignment.
 Attributes UINT32 The attributes bitmask for the variable. This information is always returned to the caller. The corresponding EFI function defined this as an optional parameter. The attributes are defined in the EFI specification.
 DataSize UINT32 On input, the size in bytes of the return Data buffer. On output the size of data returned in Data.
 Data UINT32 32-bit physical address of the buffer for returning the value of the variable. No specific alignment requirements exist.

**/

typedef struct _VARIABLE_BLOCK {
  EFI_GUID VendorGuid;
  UINT32 VariableName;
  UINT32 Attributes;
  UINT32 DataSize;
  UINT32 Data;
} VARIABLE_BLOCK;

STATIC VOID
GetSmiCommandPort(
  OUT UINT32 *mSmiCommandPort
)
{
  EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE *FADT = (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE *)EfiLocateFirstAcpiTable(
    EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE
  );

  if (FADT == NULL) {
    AsciiPrint("FADT = NULL.\n");
    return;
  }

  *mSmiCommandPort = FADT->SmiCmd;
  AsciiPrint("mSmiCommandPort = 0x%X.\n", *mSmiCommandPort);
}

INTN EFIAPI
ShellAppMain(IN UINTN Argc, IN CHAR16 **Argv)
{
  if (Argc < 3) {
    AsciiPrint("Argument count is invalid: %d.\n", Argc);
    return EFI_INVALID_PARAMETER;
  };

  STATIC EFI_GUID AmiSetupVariableGuid = { 0xec87d643, 0xeba4, 0x4bb5, { 0xa1, 0xe5, 0x3f, 0x3e, 0x36, 0xb2, 0x0d, 0xa9 } };

  CHAR16 *Name = reinterpret_cast<CHAR16 *>(L"Setup");
  UINTN DataSize = 0;
  UINT32 Attributes;
  CHAR8 *SetupData = NULL;
  EFI_STATUS Status = gRT->GetVariable(
    Name,
    &AmiSetupVariableGuid,
    &Attributes,
    &DataSize,
    NULL
  );
  if (Status == EFI_BUFFER_TOO_SMALL || Status == EFI_SUCCESS) {
    SetupData = (CHAR8 *)AllocateZeroPool(DataSize);

    
    Status = gRT->GetVariable(
      Name,
      &AmiSetupVariableGuid,
      &Attributes,
      &DataSize,
      SetupData
    );
  }
  else
  {
    AsciiPrint("Failed getting %s (%g) - %r.\n", Name, AmiSetupVariableGuid, Status);
    return Status;
  }
  if (StrHexToUintn(Argv[1]) > DataSize) {
    AsciiPrint("%s is smaller than 0x%X. Operation canceled.\n", Name, StrHexToUintn(Argv[1]));
    return EFI_INVALID_PARAMETER;
  }

  SetupData[StrHexToUintn(Argv[1])] = (INT8)StrHexToUintn(Argv[2]);
  AsciiPrint("Got %s (%g) - %r.\nSetting %s[0x%X] to 0x%X...\n", Name, AmiSetupVariableGuid, Status, Name, StrHexToUintn(Argv[1]), (INT8)StrHexToUintn(Argv[2]));

  VARIABLE_BLOCK *Request = (VARIABLE_BLOCK *)AllocateZeroPool(sizeof(VARIABLE_BLOCK));
  Request->VendorGuid = AmiSetupVariableGuid;
  Request->VariableName = (UINT32)(UINTN)Name;
  Request->Attributes = Attributes;
  Request->DataSize = (UINT32)DataSize;
  Request->Data = (UINT32)(UINTN)SetupData;

  SendRequest((UINT32)(UINTN)Request);
  AsciiPrint("Constructed VARIABLE_BLOCK for request at %X.\nCalling %d subfunction.\n", Request, GetAh());

  //Execute SmiSetVar
  UINT32 mSmiCommandPort;
  GetSmiCommandPort(&mSmiCommandPort);
  if (mSmiCommandPort == 0) {
    AsciiPrint("Couldn't retrieve SMI_CMD.\n");
    return EFI_UNSUPPORTED;
  }

  IoWrite8(mSmiCommandPort, FUNCTION_NUMBER);

  //Get Status
  UINT8 Error = 0xFF;
  Error = GetAh();
  CHAR8 *Message = NULL;

  //UINT8 Error = IoRead8(mSmiCommandPort);
  switch (Error)
  {
    case NO_ERROR: Message = "Success"; break;
    case INVALID_VALUE: Message = "Invalid Value"; break;
    case BUFFER_TOO_SMALL: Message = "Buffer Too Small"; break;
    case NOT_SUPPORTED: Message = "Unsupported"; break;
    case DEVICE_ERROR: Message = "Device Error"; break;
    case NOT_FOUND :Message = "Not Found"; break;
    case ACCESS_DENIED: Message = "Access Denied"; break;

    default: Message = "Not Started"; break;
  }

  AsciiPrint("Status - 0x%X (%a). Comm Data at %X\n", Error, Message, (UINT32)(UINTN)SetupData);

  FreePool(Request);
  return Status;
}
