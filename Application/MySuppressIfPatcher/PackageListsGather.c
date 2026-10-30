#include "Includes.h"

static EFI_STATUS
GetHiiFormsPackagesByGuid(
  IN EFI_GUID *PackageGuid OPTIONAL,
  OUT UINTN *BufferSize,
  OUT EFI_HII_HANDLE **HiiHandles
)
{
  EFI_HII_HANDLE *HiiHandleBuffer = NULL;
  UINTN HiiHandleBufferSize = 0;

  EFI_STATUS Status = gBS->LocateProtocol(&gEfiHiiDatabaseProtocolGuid, NULL, (VOID **)&gHiiDatabase);
  if (EFI_ERROR(Status)) {
    Print(L"Locate HiiDb status %r\n\r", Status);
    return EFI_UNSUPPORTED;
  }

  Status = gHiiDatabase->ListPackageLists(gHiiDatabase, EFI_HII_PACKAGE_FORMS, PackageGuid, &HiiHandleBufferSize, NULL);
  Print(L"HII handle instances: %d\n\r", HiiHandleBufferSize / sizeof(EFI_HII_HANDLE));

  if (EFI_ERROR(Status) && Status != EFI_BUFFER_TOO_SMALL)
  {
    Print(L"%r\n\r", Status);
  }
  else
  {
    HiiHandleBuffer = AllocateZeroPool(HiiHandleBufferSize);
    if (HiiHandleBuffer == NULL) {

      return EFI_OUT_OF_RESOURCES;
    }

    Status = gHiiDatabase->ListPackageLists(gHiiDatabase, EFI_HII_PACKAGE_FORMS, PackageGuid, &HiiHandleBufferSize, HiiHandleBuffer);
  }

  *BufferSize = HiiHandleBufferSize;
  *HiiHandles = HiiHandleBuffer;

  return Status;
}

static EFI_HII_PACKAGE_LIST_HEADER *
GetHandlePackageList(
  IN CONST EFI_HII_HANDLE HiiHandle
)
{
  EFI_HII_PACKAGE_LIST_HEADER *PackageList = NULL;
  UINTN BufferSize = 0;
  //-----------------------------------------------------

  EFI_STATUS Status = gHiiDatabase->ExportPackageLists(gHiiDatabase, HiiHandle, &BufferSize, PackageList);
  if (EFI_ERROR(Status))
  {
    PackageList = AllocateZeroPool(BufferSize);
    if (PackageList == NULL) {
      return NULL;
    }

    Status = gHiiDatabase->ExportPackageLists(gHiiDatabase, HiiHandle, &BufferSize, PackageList);
  }

  Print(L"HII handle %X\nFORMS PackageList BufferSize: 0x%X, Guid: %g, Export Status: %r\n\r", HiiHandle, BufferSize, PackageList->PackageListGuid, Status);

  if (EFI_ERROR(Status))
  {
    if (PackageList != NULL) {
      FreePool(PackageList);
    }
    return NULL;
  }
  else
  {
    return PackageList;
  }
}

static EFI_STATUS
GetAllPackageLists(
  OUT EFI_HII_HANDLE **HiiHandles,
  OUT EFI_HII_PACKAGE_LIST_HEADER ***PackageLists,
  OUT UINTN *PackageListsBufferSize
)
{
  EFI_STATUS Status;
  EFI_HII_HANDLE *HiiHandleBuffer = NULL;
  EFI_HII_PACKAGE_LIST_HEADER **TempPackageListBuffer = NULL;
  UINTN HiiHandleBufferSize = 0;
  //-----------------------------------------------------

  Status = GetHiiFormsPackagesByGuid(NULL, &HiiHandleBufferSize, &HiiHandleBuffer);
  if (EFI_ERROR(Status))
  {
    return Status;
  }

  *PackageListsBufferSize = (HiiHandleBufferSize / sizeof(EFI_HII_HANDLE)) * sizeof(EFI_HII_PACKAGE_LIST_HEADER);
  TempPackageListBuffer = AllocateZeroPool(*PackageListsBufferSize);
  if (TempPackageListBuffer == NULL) { return EFI_OUT_OF_RESOURCES; }

  for (UINTN i = 0; i < HiiHandleBufferSize / sizeof(EFI_HII_HANDLE); i++)
  {
    TempPackageListBuffer[i] = GetHandlePackageList(HiiHandleBuffer[i]);

    Print(L"PACKAGE LIST at: 0x%X, IFR base: 0x%X\n\n\r", TempPackageListBuffer[i], TempPackageListBuffer[i] + 1);
  }

  if (TempPackageListBuffer[0] != 0) {
    *HiiHandles = HiiHandleBuffer;
    *PackageLists = TempPackageListBuffer;
  }
  else
  {
    if (TempPackageListBuffer != NULL) {
      FreePool(TempPackageListBuffer);
    }
    Status = EFI_NOT_FOUND;
  }

  return Status;
}
