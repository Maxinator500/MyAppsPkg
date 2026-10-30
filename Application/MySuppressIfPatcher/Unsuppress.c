#include "Includes.h"

static EFI_STATUS
GetFormSetPointer(
  IN EFI_LOADED_IMAGE_PROTOCOL *PImageInfo,
  OUT UINT8 ***FormSetStart,
  OUT UINT8 *FormSetsCount
)
{
  EFI_STATUS Status = EFI_NOT_FOUND;
  UINT8 **TempPointerBuffer = AllocateZeroPool(sizeof(UINTN) * *FormSetsCount);
  *FormSetsCount = 0;
  if (TempPointerBuffer == NULL) {
    Print(L"Couldn't allocate memory for \"UINT8 **FormSetStart\"\n\r");
    return Status;
  }

  CONST UINT8 ifrHiiBinPointerHeaderUint[] = { EFI_HII_PACKAGE_FORMS, EFI_IFR_FORM_SET_OP, 0xA7 };

  for (UINTN i = 0; i < PImageInfo->ImageSize; i++)
  {
    UINTN Offset = (UINTN)PImageInfo->ImageBase + (UINTN)i;

    if (CompareMem((UINT8 *)Offset, (UINT8 *)ifrHiiBinPointerHeaderUint, sizeof(ifrHiiBinPointerHeaderUint)) == 0)
    {
      TempPointerBuffer[*FormSetsCount] = (UINT8 *)(Offset - (sizeof(UINT64) - 1));
      *FormSetsCount += 1;

      Print(L"Found Form Set %d\n\r", *FormSetsCount);
    }
  }

  if (*FormSetsCount) {
    *FormSetStart = TempPointerBuffer;
    Status = EFI_SUCCESS;
  }
  else
  {
    FreePool(TempPointerBuffer);
  }
  return Status;
}

static BOOLEAN
ProcessNestedStatement(
  IN UINT8 *Buffer,
  IN UINTN CurrentIndex,
  IN UINT8 StatementLength,
  OUT UINT8 *ConditionsLength
)
{
  if ((StatementLength == 0) || (Buffer == NULL)) return FALSE;

  /*
  typedef struct _EFI_IFR_OP_HEADER {
    UINT8    OpCode;
    UINT8    Length : 7;
    UINT8    Scope  : 1;
  } EFI_IFR_OP_HEADER;
  */

  UINT8 *ConditionsPointer = Buffer + CurrentIndex + StatementLength;
  for (UINT8 j = 0; j < MAX_UINT8; j++)
  {
    *ConditionsLength += *(ConditionsPointer + 1) & 0x0F;
    ConditionsPointer = Buffer + CurrentIndex + StatementLength + *ConditionsLength;

    if (*ConditionsPointer == EFI_IFR_END_OP)
    {
      *ConditionsLength += 0x2;
      return TRUE;
    }
  }

  Print(L"The statement' conditions are invalid or very long:\n\r");
  for (UINTN i = 0; i < 0x30; i++)
  {
    if (i % 0x10 == 0)
    {
      Print(L"\n");
    }
    Print(L"%02X ", Buffer[CurrentIndex + i]);
  }
  Print(L"\n");

  return FALSE;
}

static EFI_STATUS
SimpleSuppressIfReplace(
  IN UINT8 *Buffer,
  IN UINTN Length
)
{

  EFI_STATUS Status = EFI_NOT_FOUND;
  if (Length >= MAX_UINT32) return Status;

  UINTN Index = 0;
  UINTN StatementsPatched = 0;

  CONST UINT16 SuppressIf = 0x820A;
  UINT8 ReplacementOp[2] = { EFI_IFR_FALSE_OP, 0 };
  BOOLEAN GotLength;

  while (Index < Length - sizeof(SuppressIf)) {

    if (SuppressIf == *(UINT16 *)(Buffer + Index)) //{0A 82}
    {
      ReplacementOp[1] = 0;
      GotLength = FALSE;
      //Process nested
      UINT8 StatementLength = *(Buffer + Index + 1) & 0x0F; //0x82 -> 0x02
      if (*(Buffer + Index + StatementLength + 0x1) & EFI_IFR_FLAG_OPTIONS_ONLY) {
        GotLength = ProcessNestedStatement(Buffer, Index, StatementLength, &ReplacementOp[1]);
      }

      CopyMem((UINT8 *)Buffer + Index + sizeof(SuppressIf), ReplacementOp, (UINT8)GotLength + 0x1);
      StatementsPatched++;

      if (GotLength)
      {
        //Debug
        //Print(L"Patched nested statement of size 0x%X at 0x%X\n\r", ReplacementOp[1], (UINT8 *)Buffer + Index);

        Length -= ReplacementOp[1] + sizeof(SuppressIf);
        Index += ReplacementOp[1] + sizeof(SuppressIf);
        continue;
      }
    }

    Index++;
  }
  Print(L"\nPatched %d statements\n\r", StatementsPatched);

  if (Index >= Length - sizeof(SuppressIf))
  {
    Status = EFI_SUCCESS;
  }

  return Status;
}

static VOID
ProcessHandlesImageInfoAndPatch(
  IN EFI_LOADED_IMAGE_PROTOCOL **ImageInfo,
  IN UINTN HandlesCount,
  OUT BOOLEAN *isSetupProcessed
)
{
  EFI_STATUS Status = EFI_NOT_FOUND;
  EFI_GUID *FilePath = NULL;
  UINT8 FormSetsCount = 10; //10 will be the limit
  UINT8 **FormSetStart = NULL;
  UINTN FirstFormSetToEnd = 0;

  for (UINTN i = 0; i < HandlesCount; i++) {

    if (ImageInfo[i] == NULL)
    {
      Print(L"Empty ImageInfo instance, proceeding with next\n\n\r");
      continue;
    }

    Status = GetFormSetPointer(ImageInfo[i], &FormSetStart, &FormSetsCount); //Allocates
    if (EFI_ERROR(Status))
    {
      Print(L"No FormSet found, GetFormSetPointer status: %r\n\n\r", Status);

      if (FormSetStart != NULL) {
        FreePool(FormSetStart);
      }
      continue;
    }

    FilePath = EfiGetNameGuidFromFwVolDevicePathNode((MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)ImageInfo[i]->FilePath);
    if (CompareGuid(FilePath, &AptioPackageListGuid) ||
        CompareGuid(FilePath, &InsydePackageListGuid))
    {
      *isSetupProcessed = TRUE;
    }

    FirstFormSetToEnd = (UINTN)ImageInfo[i]->ImageSize - ((UINTN)ImageInfo[i]->ImageBase - (UINTN)FormSetStart[i]);
    Print(L"Patching Image %g,\nstarting FormSet offset: 0x%X, count: %d, length: around 0x%X\n\r", FilePath, FormSetStart[i], FormSetsCount, FirstFormSetToEnd);

    for (UINT8 j = 0; j < FormSetsCount; j++) {
      Status = SimpleSuppressIfReplace((UINT8 *)FormSetStart[j], *(UINT32 *)(FormSetStart[j])); //To process only the top formset - *(UINT32 *)FormSetStart
      if (EFI_ERROR(Status))
      {
        Print(L"SimpleSuppressIfReplace status: %r\n\r", Status);
        continue;
      }
    }
    if (FormSetStart != NULL) {
      FreePool(FormSetStart);
    }
  }

  return;
}

static EFI_STATUS
ProcessPackagesListsAndPatch(
  IN EFI_HII_PACKAGE_LIST_HEADER **PackageLists,
  IN UINTN HandlesCount
)
{
  EFI_STATUS Status;

  for (UINTN i = 0; i < HandlesCount; i++) {

    if (PackageLists[i] == NULL)
    {
      Print(L"Empty PackageList instance, proceeding with next\n\r");
      continue;
    }

    Print(L"Patching PackageList %g,\nstart offset: 0x%X, size: 0x%X\n\r", PackageLists[i]->PackageListGuid, (UINT8 *)(PackageLists[i] + 1), (UINTN)PackageLists[i]->PackageLength - sizeof(EFI_HII_PACKAGE_LIST_HEADER));
    Status = SimpleSuppressIfReplace((UINT8 *)(PackageLists[i] + 1), (UINTN)PackageLists[i]->PackageLength - sizeof(EFI_HII_PACKAGE_LIST_HEADER));
    if (EFI_ERROR(Status))
    {
      Print(L"SimpleSuppressIfReplace status: %r\n\r", Status);
      continue;
    }
  }

  return Status;
}
