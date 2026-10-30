#include "ImageHandlesGather.c"
#include "PackageListsGather.c"
#include "Unsuppress.c"

int _fltused = 1;

static VOID
FireAptioNamedEventSignal()
{
  EFI_GUID AMITSE_SETUP_ENTER_GUID = { 0x71202EEE, 0x5F53, 0x40d9, { 0xAB, 0x3D, 0x9E, 0x0C, 0x26, 0xD9, 0x66, 0x57 } };
  EFI_HANDLE Handle = NULL;

  gBS->InstallProtocolInterface(
    &Handle,
    &AMITSE_SETUP_ENTER_GUID,
    EFI_NATIVE_INTERFACE,
    NULL
  );

  gBS->UninstallProtocolInterface(
    Handle,
    &AMITSE_SETUP_ENTER_GUID,
    NULL
  );
}

static EFI_LOADED_IMAGE_PROTOCOL *
GetFirstHandleFileGuid(
  IN EFI_GUID *First,
  ...
)
{
  VA_LIST Marker;
  EFI_GUID *Current;
  EFI_LOADED_IMAGE_PROTOCOL *Found;

  if (First == NULL) {
    return NULL;
  }

  Found = NULL;
  if (!EFI_ERROR(FindLoadedImageFromGUID(*First, &Found))) {
    return Found;
  }

  VA_START(Marker, First);
  while ((Current = VA_ARG(Marker, EFI_GUID *)) != NULL) {

    Found = NULL;
    if (!EFI_ERROR(FindLoadedImageFromGUID(*Current, &Found))) {
      break;
    }
  }
  VA_END(Marker);

  return Found;
}

EFI_STATUS EFIAPI
Main(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {

  //seek EFI_IFR_SUPPRESS_IF_OP in HiiDb and Handles
  EFI_HII_PACKAGE_LIST_HEADER **PackageLists = NULL;
  EFI_LOADED_IMAGE_PROTOCOL **ImageInfo = NULL;

  EFI_HII_HANDLE *HiiHandles;
  UINTN PackageListsBufferSize;

  //Space for HiiHandles, PackageLists is allocated!
  EFI_STATUS Status = GetAllPackageLists(&HiiHandles, &PackageLists, &PackageListsBufferSize);
  if (EFI_ERROR(Status))
  {
    Print(L"GetAllPackageLists status: %r\n", Status);
    return EFI_UNSUPPORTED;
  }
  Print(L"HiiDb exported and saved.\n\r");
  UINTN CountFromBufSize = PackageListsBufferSize / sizeof(EFI_HII_PACKAGE_LIST_HEADER);


  Print(L"--------------------------------------\n\r");
  Status = GetLoadedImagesFromHiiHandles(HiiHandles, CountFromBufSize, &ImageInfo);
  if (EFI_ERROR(Status))
  {
    Print(L"GetLoadedImagesFromHiiHandles status: %r\n", Status);
    return EFI_UNSUPPORTED;
  }

  Print(L"Images Handles saved.\n\r");

  Print(L"--------------------------------------\n\r");
  BOOLEAN isSetupProcessed = FALSE; //Want to know if res are published at the DXE stage and theres something to patch

  ProcessHandlesImageInfoAndPatch(ImageInfo, CountFromBufSize, &isSetupProcessed);

  if (!isSetupProcessed) {
    Print(L"Warning! Couldn't locate main Setup DXE! Trying again...\n\r");
    gBS->Stall(3000000);

    //May force res to publish
    FireAptioNamedEventSignal();
    ImageInfo[0] = GetFirstHandleFileGuid(&InsydePackageListGuid, &AptioPackageListGuid); //Guids are hardcoded in includes

    if (ImageInfo[0] != NULL) {
      ProcessHandlesImageInfoAndPatch(ImageInfo, 1, &isSetupProcessed);
    }
    if (!isSetupProcessed)
    {
      Print(L"Error! Couldn't locate main Setup DXE twice!\nOnly the HiiDb' PackagesLists will be patched\n\r");
    }
    else
    {
      Print(L"Success! Main Setup DXE found and patched!\n\r");
    }
  }

  Print(L"--------------------------------------\n\r");
  Status = ProcessPackagesListsAndPatch(PackageLists, CountFromBufSize);
  if (EFI_ERROR(Status))
  {
    Print(L"ProcessPackagesListsAndPatch status: %r\n\r", Status);
  }

  //Package Lists require update using patched export...
  for (UINTN Index = 0; Index < CountFromBufSize; Index++) {
    if (PackageLists[Index] == NULL) continue;

    Status = gHiiDatabase->UpdatePackageList(
      gHiiDatabase,
      HiiHandles[Index],
      PackageLists[Index]
    );
    if (EFI_ERROR(Status)) {
      Print(L"UpdatePackageList status: %r\nCouldn't update PackageListGuid %g\n\r", Status, PackageLists[Index]->PackageListGuid);
    }
  }

  if (HiiHandles != NULL) {
    FreePool(HiiHandles);
  }
  if (ImageInfo != NULL) {
    FreePool(ImageInfo);
  }
  if (PackageLists != NULL) {
    FreePool(PackageLists);
  }

  return Status;
}
