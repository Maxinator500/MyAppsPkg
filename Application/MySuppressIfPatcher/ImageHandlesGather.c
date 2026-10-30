#include "Includes.h"

static EFI_STATUS
FindLoadedImageFromGUID(
  IN EFI_GUID FileGuid,
  OUT EFI_LOADED_IMAGE_PROTOCOL **PImageInfo
)
{
  EFI_STATUS Status;
  EFI_LOADED_IMAGE_PROTOCOL *ImageInfo = NULL;
  EFI_HANDLE *Handles = NULL;
  UINTN BufferSize = 0;
  //-----------------------------------------------------

  gBS->LocateHandle(ByProtocol, &gEfiLoadedImageProtocolGuid, NULL, &BufferSize, NULL);
  Handles = AllocateZeroPool(BufferSize);
  if (Handles == NULL) { return EFI_OUT_OF_RESOURCES; }

  Status = gBS->LocateHandle(ByProtocol, &gEfiLoadedImageProtocolGuid, NULL, &BufferSize, Handles);
  if (EFI_ERROR(Status))
  {
    Print(L"FindLoadedImageFromGUID error\n\r");
    return Status;
  }

  for (UINTN i = 0; i < BufferSize / sizeof(EFI_HANDLE); i++)
  {
    Status = gBS->HandleProtocol(Handles[i], &gEfiLoadedImageProtocolGuid, (VOID **)&ImageInfo); //Process every found handle

    if ((Status == EFI_SUCCESS) && (ImageInfo->ImageSize != 0))
    {
      EFI_GUID NameGuid = *EfiGetNameGuidFromFwVolDevicePathNode((MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)ImageInfo->FilePath);
      if (CompareGuid(&FileGuid, &NameGuid) == 1)
      {
        Print(L"Found %g\n\r", NameGuid);
        Status = EFI_SUCCESS;

        break;
      }
    }

    ImageInfo = NULL;
    Status = EFI_NOT_FOUND;
  }

  if (Handles != NULL) {
    FreePool(Handles);
  }

  *PImageInfo = ImageInfo;
  return Status;
}

static EFI_STATUS GetLoadedImagesFromHiiHandles(
  IN EFI_HII_HANDLE *HiiHandles,
  IN UINTN HandlesCount,
  OUT EFI_LOADED_IMAGE_PROTOCOL ***ImageInfo
)
{
  EFI_STATUS Status = gBS->LocateProtocol(&gEfiHiiDatabaseProtocolGuid, NULL, (VOID **)&gHiiDatabase);
  if (EFI_ERROR(Status)) {
    Print(L"Locate HiiDb status %r\n\r", Status);
    return EFI_UNSUPPORTED;
  }

  EFI_LOADED_IMAGE_PROTOCOL **TempImageInfo = AllocateZeroPool(HandlesCount * sizeof(UINTN));
  UINT8 **DevicePathInfo = AllocateZeroPool(HandlesCount * sizeof(UINTN));
  if (TempImageInfo == NULL) { return EFI_OUT_OF_RESOURCES; }

  EFI_HANDLE DriverHandle;
  for (UINTN i = 0; i < HandlesCount; i++) {
    Status = gHiiDatabase->GetPackageListHandle(
      gHiiDatabase,
      HiiHandles[i],
      &DriverHandle
    );
    if (EFI_ERROR(Status)) {
      Print(L"GetPackageListHandle status %r\n\n\r", Status);
      break;
    }

    /*Handles got from gBS->OpenProtocol or gBS->HandleProtocol point to HII CONFIG ACCESS, but we need parent
    Status = gBS->OpenProtocol(
      DriverHandle,
      &gEfiDevicePathProtocolGuid,
      (VOID **)&DevicePathInfo[i],
      gImageHandle,
      NULL,
      EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );
    */
    Status = gBS->HandleProtocol(
      DriverHandle,
      &gEfiDevicePathProtocolGuid,
      (VOID **)&DevicePathInfo[i]
    );
    if (EFI_ERROR(Status)) {
      Print(L"\nHandleProtocol status %r\n\n\r", Status);
    }


    EFI_GUID FvFileName;
    //FvFileName = *(EFI_GUID *)(DevicePathInfo[i] + sizeof(EFI_DEVICE_PATH_PROTOCOL) + sizeof(UINT32) + sizeof(EFI_GUID));
    //First comes FW_VOL_DEVICE_PATH, then FW_VOL_FILEPATH_DEVICE_PATH, then VENDOR_DEVICE_PATH
    //Print(GetProtocolInformationDump(DriverHandle, &gEfiDevicePathProtocolGuid, TRUE));

    //Better impl to get FvFileName?
    VOID *Node = DevicePathInfo[i];
    FvFileName = gZeroGuid;
    while (!IsDevicePathEnd(Node)) {
      if (DevicePathType(Node) == MEDIA_DEVICE_PATH &&
        DevicePathSubType(Node) == MEDIA_PIWG_FW_FILE_DP) {
        FvFileName = ((MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)Node)->FvFileName;
        break;
      }
      Node = NextDevicePathNode(Node);
    }

    if (CompareGuid(&FvFileName, &gZeroGuid)) {
      Print(L"HII handle %X doesn't have parent Driver handle, nothing to patch\n\r", HiiHandles[i]);
      continue;
    }

    Print(L"HII handle %X has parent Driver handle %X\nwith GUID: %g\n\r", HiiHandles[i], DriverHandle, FvFileName);

    Status = FindLoadedImageFromGUID(FvFileName, &TempImageInfo[i]);
    if (EFI_ERROR(Status)) {
      Print(L"\nFindLoadedImageFromGUID status %r\n\n\r", Status);
    }

    Print(L"LOADED IMAGE INTERFACE at: 0x%X, PE base: 0x%X\n\n\r", TempImageInfo[i], TempImageInfo[i]->ImageBase);
  }

  if (DevicePathInfo != NULL) { FreePool(DevicePathInfo); }

  *ImageInfo = TempImageInfo;
  return EFI_SUCCESS;
}
