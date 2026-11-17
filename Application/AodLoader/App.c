
#include <Uefi.h>
#include <PiDxe.h> 
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/HandleParsingLib.h>
#include <Library/PrintLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiPackageList.h>
#include <Guid/ZeroGuid.h>

#define MAX_FORWARD_SCAN MAX_UINT8


static VOID
PrintConsoleDump(
  IN UINT64 Size,
  IN UINT8 *Dump,
  IN BOOLEAN isUnformattedAscii
)
{
  if (isUnformattedAscii)
  {
    for (UINT64 i = 0; i < Size * 2; i++)
    {
      if (i % 0x20 == 0)
      {
        Print(u"%a", "\n\r");
      }
      if (i % 2 == 0)
      {
        Print(u"%a", " ");
      }
      Print(u"%c", Dump[i]);
    }
    Print(u"%a", "\n\r");

    return;
  }

  for (UINT64 i = 0; i < Size; i++)
  {
    if (i % 0x10 == 0)
    {
      Print(u"%a", "\n\r");
    }
    Print(u"%02x ", Dump[i]);
  }
  Print(u"%a", "\n\r");
}

static CHAR16 *
FindLoadedImageBuffer(
  IN EFI_LOADED_IMAGE_PROTOCOL *LoadedImage,
  IN UINT8 SectionType,
  IN UINTN SectionInstance,
  OUT UINTN *Size
)
{
  EFI_STATUS Status;
  EFI_GUID *NameGuid = NULL;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv2 = NULL;
  VOID *Buffer = NULL;
  UINTN BufferSize = 0;
  UINT32 AuthenticationStatus;
  //-----------------------------------------------------

  if ((LoadedImage == NULL) || (LoadedImage->FilePath == NULL))
  {
    return NULL;
  }

  NameGuid = EfiGetNameGuidFromFwVolDevicePathNode((MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)LoadedImage->FilePath);

  if (NameGuid == NULL)
  {
    return NULL;
  }

  //
  //Get the Protocol of the device handle that this image was loaded from.
  //
  Status = gBS->HandleProtocol(LoadedImage->DeviceHandle, &gEfiFirmwareVolume2ProtocolGuid, (VOID **)&Fv2);

  //
  //FirmwareVolume2Protocol is PI, and is not required to be available.
  //
  if (EFI_ERROR(Status))
  {
    return NULL;
  }

  //
  //Read the user interface section of the image.
  //
  Status = Fv2->ReadSection(Fv2, NameGuid, SectionType, SectionInstance, &Buffer, &BufferSize, &AuthenticationStatus);

  if (EFI_ERROR(Status))
  {
    return NULL;
  }

  *Size = BufferSize;

  //
  //ReadSection returns just the section data, without any section header. For
  //a user interface section, the only data is the file name.
  //
  return Buffer;
}

static EFI_STATUS
LocateAodFromFV(
  OUT UINT8 **Buffer,
  OUT UINTN *BufferSize
)
{
  EFI_STATUS Status;
  EFI_HANDLE *Handles = NULL;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *FvInstance = NULL;
  UINTN HandleBufferSize, VolIndex = 0;
  //-----------------------------------------------------

  Status = gBS->LocateHandleBuffer(
    ByProtocol,
    &gEfiFirmwareVolume2ProtocolGuid,
    NULL,
    &HandleBufferSize,
    &Handles
  );
  if (EFI_ERROR(Status))
  {
    Print(L"LocateHandleBuffer error\n\r");

    return Status;
  }

  for (UINTN Index = 0; Index < HandleBufferSize; Index++)
  {
    Status = gBS->HandleProtocol(
      Handles[Index],
      &gEfiFirmwareVolume2ProtocolGuid,
      (VOID **)&FvInstance
    );

    if (EFI_ERROR(Status)) {
      Print(L"HandleProtocol error\n\r");

      FreePool(Handles);
      return Status;
    }

    EFI_FV_FILETYPE FileType = EFI_FV_FILETYPE_ALL;
    EFI_FV_FILE_ATTRIBUTES FileAttributes;
    UINTN FileSize;
    EFI_GUID NameGuid = gZeroGuid;
    VOID *Keys = AllocateZeroPool(FvInstance->KeySize);
    if (Keys == NULL) {
      FreePool(Handles);
      return EFI_OUT_OF_RESOURCES;
    }

    while (TRUE)
    {
      FileType = EFI_FV_FILETYPE_ALL;
      NameGuid = gZeroGuid;
      Status = FvInstance->GetNextFile(
        FvInstance,
        Keys,
        &FileType,
        &NameGuid,
        &FileAttributes,
        &FileSize
      );
      if (EFI_ERROR(Status))
      {
        Print(L"Image not found on Firmware Volume %d\n\r", VolIndex++);
        break;
      }

      VOID *String;
      UINTN StringSize = 0;
      UINT32 AuthenticationStatus;
      String = NULL;
      FvInstance->ReadSection(
        FvInstance,
        &NameGuid,
        EFI_SECTION_USER_INTERFACE,
        0,
        &String,
        &StringSize,
        &AuthenticationStatus
      );
      if (!CompareMem((VOID *)L"Aod", String, sizeof(UINT16) * StrLen(L"Aod")) && StrStr(String, L"Dxe"))
      {
        Print(L"Image %s Found on FV %d\nGuid and Size: %g / 0x%X\n\r", String == NULL ? L"NONAME" : String, VolIndex, NameGuid, FileSize);

        Status = FvInstance->ReadSection(
          FvInstance,
          &NameGuid,
          EFI_SECTION_PE32,
          0,
          (VOID **)Buffer,
          BufferSize,
          &AuthenticationStatus
        );
        Print(L"ReadSection: %r\n\r", Status);

        if ((*BufferSize < MAX_UINT16))
        {
          FreePool(*Buffer);
          *Buffer = NULL;
          *BufferSize = 0;
          continue;
        }

        FreePool(Handles);
        FreePool(String);
        FreePool(Keys);
        return Status;
      }
      if (String != NULL) {
        FreePool(String);
      }
    }
    FreePool(Keys);
  }

  FreePool(Handles);
  return EFI_NOT_FOUND;
}

static EFI_STATUS
LoadAodFromFV(
  OUT EFI_LOADED_IMAGE_PROTOCOL **ImageInfo,
  OUT EFI_HANDLE *OutHandle
)
{
  EFI_STATUS Status;
  EFI_HANDLE NewHandle = NULL;
  UINT8 *Buffer = NULL;
  UINTN BufferSize = 0;

  LocateAodFromFV(&Buffer, &BufferSize);
  Status = gBS->LoadImage(
    FALSE,
    gImageHandle,
    (VOID *)NULL,
    Buffer,
    BufferSize,
    &NewHandle
  );
  if (Buffer != NULL) {
    FreePool(Buffer);
  }

  if (!EFI_ERROR(Status))
  {
    Status = gBS->OpenProtocol(
      NewHandle,
      &gEfiLoadedImageProtocolGuid,
      (VOID **)ImageInfo,
      gImageHandle,
      (VOID *)NULL,
      EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );

    *OutHandle = NewHandle;
  }
  else
  {
    Print(L"LoadImage error\n\r");
  }

  return Status;
};

static EFI_STATUS
FindLoadedImageFromShellIndex(
  IN UINTN HandleIndex,
  OUT EFI_LOADED_IMAGE_PROTOCOL **ImageInfo
)
{
  EFI_HANDLE TheHandle;
  //-----------------------------------------------------

  Print(L"Handle Index %X\n\r", HandleIndex);

  EFI_HANDLE *AllHandles = GetHandleListByProtocol(NULL);
  if (AllHandles == NULL)
  {
    Print(L"Failed getting handle list\n\r");
    return EFI_NOT_FOUND;
  }

  TheHandle = ConvertHandleIndexToHandle(HandleIndex);
  if (TheHandle != NULL)
  {
    Print(L"\n\r%X: 0x%X\n\r", HandleIndex, TheHandle);
    {
      return gBS->HandleProtocol(TheHandle, &gEfiLoadedImageProtocolGuid, (VOID **)ImageInfo);
    }
  }

  return EFI_NOT_FOUND;
}

static CHAR8 *
ApplySimpleMask(
  IN UINT8 *SourceBuffer, //Accepts UINT8 byte buffer only
  IN CHAR8 *MaskBuffer,
  IN UINTN MaskLengthBytes,
  IN CHAR8 Metacharacter
)
{
  if ((SourceBuffer == NULL) || (MaskBuffer == NULL)) {
    return NULL;
  }

  CHAR8 *tmp = AllocatePool(0x3);
  CHAR8 *StringifiedHexDump = AllocateZeroPool(MAX_UINT8);
  CHAR8 *Result = AllocateZeroPool(MaskLengthBytes * sizeof(UINT16));
  if (tmp == NULL || StringifiedHexDump == NULL || Result == NULL) {
    Print(L"Out of resources\n\r");

    return NULL;
  }

  for (UINT16 i = 0; i < MaskLengthBytes; i++) //Get string from UINT buffer
  {
    AsciiSPrint(tmp, AsciiStrSize("XX"), "%02x", SourceBuffer[i]);
    AsciiStrCatS(StringifiedHexDump, sizeof(StringifiedHexDump) + AsciiStrSize('\0'), tmp);
  }

  for (UINTN i = 0; i < MaskLengthBytes * sizeof(UINT16); i++)
  {
    if (MaskBuffer[i] == Metacharacter) //e.g. 0x2E is (CHAR8)'.'
    {
      Result[i] = StringifiedHexDump[i];
      continue;
    }

    Result[i] = MaskBuffer[i];
  }

  if (StringifiedHexDump != NULL) {
    FreePool(tmp);
    FreePool(StringifiedHexDump);
  }

  return Result;
}

static BOOLEAN
isInterfaceInitBeforeSig(
  IN UINT8 *OffsetP,
  IN UINT8 HowFarBack
)
{
  STATIC CHAR8 *RegexPatternBuffer = "4.......48"; UINT8 RegexPatternLength = (UINT8)AsciiStrLen(RegexPatternBuffer) / 2;
  UINT8 *Location = OffsetP - HowFarBack;
  UINT8 *HexPattern = AllocateZeroPool(RegexPatternLength);

  CHAR8 *ProcessedPattern = ApplySimpleMask(Location, RegexPatternBuffer, RegexPatternLength, '.');

  RETURN_STATUS RetStatus = AsciiStrHexToBytes(ProcessedPattern, RegexPatternLength * sizeof(UINT16), HexPattern, RegexPatternLength);
  if (RETURN_ERROR(RetStatus))
  {
    Print(L"ProcessedPattern: %a,\nisInterfaceInitBeforeSig error: %r\n\r", ProcessedPattern, RetStatus);

    FreePool(ProcessedPattern);
    return FALSE;
  }

  FreePool(ProcessedPattern);
  return (BOOLEAN)!CompareMem(Location, (UINT8 *)HexPattern, RegexPatternLength);
}

static VOID
SetAtAllocatePool(
  IN UINT8 *OffsetP,
  IN OUT UINT8 *HowFarBack
)
{
  STATIC CHAR8 *RegexPatternBuffer = "B9..000000E8"; UINT8 RegexPatternLength = (UINT8)AsciiStrLen(RegexPatternBuffer) / 2;
  UINT8 *Location = OffsetP - *HowFarBack;
  UINT8 *HexPattern = AllocateZeroPool(RegexPatternLength);
  UINT8 ScanLen = MAX_FORWARD_SCAN / 2;

  ASUS:
  for (; OffsetP - Location < ScanLen; Location--) {

    CHAR8 *ProcessedPattern = ApplySimpleMask(Location, RegexPatternBuffer, RegexPatternLength, '.');

    RETURN_STATUS RetStatus = AsciiStrHexToBytes(ProcessedPattern, RegexPatternLength * sizeof(UINT16), HexPattern, RegexPatternLength);
    if (RETURN_ERROR(RetStatus))
    {
      Print(L"ProcessedPattern: %a,\nSetAtAllocatePool error: %r\n\r", ProcessedPattern, RetStatus);

      FreePool(ProcessedPattern);
      return;
    }
    FreePool(ProcessedPattern);

    if (CompareMem(Location, (UINT8 *)HexPattern, RegexPatternLength) == 0) {
      Print(L"Found PrivateData = AllocatePool(), setting it as jmp location\n\r");

      *HowFarBack = (UINT8)(UINTN)(OffsetP - Location);
      return;
    }
  }

  if (ScanLen < MAX_FORWARD_SCAN) {
    ScanLen = MAX_FORWARD_SCAN;
    RegexPatternBuffer = "BA..0.000041";
    goto ASUS;
  }
}

static EFI_STATUS
FindAodSignature(
  IN EFI_LOADED_IMAGE_PROTOCOL *ImageInfo,
  OUT UINT8 **InstallP
)
{
  if (ImageInfo->ImageSize < MAX_UINT16) {
    Print(L"Image is invalid\r\n");
    return EFI_INVALID_PARAMETER;
  }

  #define MAX_UINT4 0xF
  CONST UINT8 InstallMultipleProtocolInterfaces[] = { 0xFF, 0x90, 0x48, 0x01, 0x00, 0x00 };
  CONST UINT32 AODD = 0x44444F41;

  EFI_STATUS Status = EFI_NOT_FOUND;
  UINT8 *Offset = NULL;
  UINT8 *Call = NULL;
  UINT8 CompareLen = sizeof(InstallMultipleProtocolInterfaces);

  //For InstallMultipleProtocolInterfaces
  for (UINTN i = 0; i < ImageInfo->ImageSize - CompareLen; i++)
  {
    Offset = (UINT8 *)ImageInfo->ImageBase + i;

    if (CompareMem(Offset, (UINT8 *)InstallMultipleProtocolInterfaces, CompareLen) == 0)
    {
      Print(L"InstallMultipleProtocolInterfaces at %X\r\n", Offset);
      Call = Offset;
      break;
    }
  }
  Offset -= sizeof(AODD);

  //For Aod siganture
  while (Offset >= Call - MAX_FORWARD_SCAN)
  {
    if (*(UINT32 *)Offset == AODD)
    {
      Print(L"Aod signature at %X\r\n", Offset);
      Call = Offset;

      break;
    }
    Offset--;
  }

  Print(L"Last offset 100h dump\r\n");
  PrintConsoleDump(0x100, Offset - MAX_FORWARD_SCAN + CompareLen, FALSE);

  if ((Call == NULL) || (Call != Offset)) {
    Print(L"InstallMultipleProtocolInterfaces call == NULL\nor Aod signature not found\r\n");
    return Status;
  }

  //Attempt to cover interface initialization
  UINT8 HowFarBack = 0x7;
  BOOLEAN Result = isInterfaceInitBeforeSig(Call, HowFarBack);
  SetAtAllocatePool(Call, &HowFarBack);

  if (!Result) {
    Print(L"ConfigAccess protocol init not found,\nAod PrivateData may not be preserved, expect Invalid Parameter\n\r");
  }

  if (HowFarBack < MAX_UINT4)
  {
    Print(L"No suitable place found\n, can't patch and exec Aod\r\n");
  }
  else
  {
    Print(L"Redirection place found\r\n");

    Call -= HowFarBack;
    Status = EFI_SUCCESS;
  }

  *InstallP = Call;
  return Status;
}

static EFI_STATUS
LocateFirstJmpAndSetToAod(
  IN UINT8 *SetJmpTo
)
{
#define INT3_OPCODE 0xCC
#define JE_SHORT_OPCODE 0x74
#define JNE_SHORT_OPCODE 0x75
#define JMP_NEAR_OPCODE 0xE9
#define MAX_BACKWARD_SCAN 0x200

  EFI_STATUS Status;
  UINTN BackOffset;
  UINT8 ForwardOffset;
  UINT8 *Int3Address;
  UINT8 *PatchAddress;
  INT64 RelativeOffset;

  Status = EFI_NOT_FOUND;

  Print(L"Searching for INT3 before 0x%X...\n\r", SetJmpTo);

  //
  //Search nearest INT3 before SetJmpTo.
  //
  for (BackOffset = 0; BackOffset < MAX_BACKWARD_SCAN; BackOffset++)
  {
    if (*(SetJmpTo - BackOffset) == INT3_OPCODE) {
      Int3Address = SetJmpTo - BackOffset;

      Print(L"Found INT3 at 0x%X, code scan starts at 0x%X\n\r", Int3Address, Int3Address + 1);

      Status = EFI_SUCCESS;
      break;
    }
  }

  if (EFI_ERROR(Status)) {
    Print(L"INT3 marker was not found\n\r");
    return Status;
  }

  //
  //Search first short jump:
  //
  //74 xx == JE/JZ rel8
  //75 xx == JNE/JNZ rel8
  //
  for (ForwardOffset = 0; ForwardOffset < MAX_FORWARD_SCAN; ForwardOffset++)
  {
    PatchAddress = Int3Address + ForwardOffset;

    if ((*PatchAddress != JE_SHORT_OPCODE) && (*PatchAddress != JNE_SHORT_OPCODE))
    {
      continue;
    }

    //
    //JMP rel32 is: E9 xx xx xx xx
    //
    //Offset counts from the end of insctruction,
    //so do PatchAddress + 5.
    //
    RelativeOffset = (INT64)(INTN)SetJmpTo - (INT64)(INTN)(PatchAddress + 5);

    Print(L"Found conditional jump at 0x%X\nPatching it to JMP 0x%X\n\r", PatchAddress, SetJmpTo);

    *PatchAddress = JMP_NEAR_OPCODE;

    //
    //Rewrite 4 next bytes to turn it into
    //E9 <rel32>
    //
    *(INT32 *)(PatchAddress + 1) = (INT32)RelativeOffset;

    Print(L"JMP rel32 = 0x%08X\n\r",(UINT32)RelativeOffset);

    return EFI_SUCCESS;
  }

  Print(L"Conditional jump was not found\n\r");
  return EFI_NOT_FOUND;
}

static EFI_STATUS
PatchAndExecAod(
  IN EFI_LOADED_IMAGE_PROTOCOL *AodImageInfo,
  IN EFI_HANDLE AodHandle
)
{
  UINT8 *AodSigP = NULL;

  EFI_STATUS Status = FindAodSignature(AodImageInfo, &AodSigP);
  if (!EFI_ERROR(Status))
  {
    Status = LocateFirstJmpAndSetToAod(AodSigP);
  }

  if (!EFI_ERROR(Status))
  {
    UINTN ExitDataSize;
    Status = gBS->StartImage(AodHandle, &ExitDataSize, (CHAR16 **)NULL);
    Print(L"Aod returned: %r\n", Status);
  }

  return Status;
}

INTN EFIAPI
ShellAppMain(IN UINTN Argc, IN CHAR16 **Argv)
{
  #define DEBUG_ACTIVE 0
  EFI_STATUS Status;
  EFI_LOADED_IMAGE_PROTOCOL *ImageInfo = NULL;
  EFI_HANDLE AodHandle;

  if (Argc > 1 && DEBUG_ACTIVE)
  {
    if (Argv[1] != NULL) {
      UINTN ShellHandleIndex = StrHexToUintn(Argv[1]);
      if (ShellHandleIndex)
      {
        Status = FindLoadedImageFromShellIndex(ShellHandleIndex, &ImageInfo);
      }
    }
  }
  else
  {
    Status = LoadAodFromFV(&ImageInfo, &AodHandle);
  }
  Print(L"Load status: %r\n\r", Status);


  if (!EFI_ERROR(Status)) {
    Status = PatchAndExecAod(ImageInfo, AodHandle);
  }
  if (Status == EFI_ALREADY_STARTED) {
    Print(L"\nA Device Path with Aod package GUID already exists in the system\n\r");
  }

  return Status;
}
