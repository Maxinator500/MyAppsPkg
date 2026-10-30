
#include <Uefi.h>
#include <PiDxe.h> 
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiPackageList.h>
#include <Guid/ZeroGuid.h>

//Debug
//#include <Library/HandleParsingLib.h>

#define HII_ACCESS_FIELD_COUNT (sizeof(EFI_HII_CONFIG_ACCESS_PROTOCOL) / sizeof(VOID *))
CONST UINT8 DevicePathHeaderUint[] = { 0x00, 0x00, 0x00, 0x00, HARDWARE_DEVICE_PATH, HW_VENDOR_DP };
CONST UINT8 Edk2ifrSetupStringsPointerUint[] = { EFI_HII_PACKAGE_STRINGS, 0x34, 0x00, 0x00, 0x00, 0x34 };
CONST UINT8 Edk2ifrHiiBinPointerHeaderUint[] = { EFI_HII_PACKAGE_FORMS, EFI_IFR_FORM_SET_OP, 0xA7 };

static EFI_STATUS
GetSetupPointers(
  IN EFI_LOADED_IMAGE_PROTOCOL *ImageInfo,
  OUT VENDOR_DEVICE_PATH **DevicePathPointer,
  OUT UINTN **ifrSetupStringsPointer,
  OUT UINTN **ifrHiiBinPointer
)
{
  UINTN Offset;

  for (UINTN i = 0; i < ImageInfo->ImageSize; i += 1)
  {
    Offset = (UINTN)ImageInfo->ImageBase + (UINTN)i;

    if (CompareMem((UINT8 *)Offset, (UINT8 *)DevicePathHeaderUint, sizeof(DevicePathHeaderUint)) == 0
      && !(*DevicePathPointer > NULL))
    {
      *DevicePathPointer = (VENDOR_DEVICE_PATH *)(Offset + sizeof(DevicePathHeaderUint) - 2);
      continue;
    }
    if (CompareMem((UINT8 *)Offset, (UINT8 *)Edk2ifrSetupStringsPointerUint, sizeof(Edk2ifrSetupStringsPointerUint)) == 0
      && !(*ifrSetupStringsPointer > NULL))
    {
      *ifrSetupStringsPointer = (UINTN *)(Offset - (sizeof(UINT64) - 1));
      continue;
    }
    if (CompareMem((UINT8 *)Offset, (UINT8 *)Edk2ifrHiiBinPointerHeaderUint, sizeof(Edk2ifrHiiBinPointerHeaderUint)) == 0
      && !(*ifrHiiBinPointer > NULL))
    {
      *ifrHiiBinPointer = (UINTN *)(Offset - (sizeof(UINT64) - 1));
    }
  }

  if (*DevicePathPointer && *ifrSetupStringsPointer && *ifrHiiBinPointer)
  {
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

static BOOLEAN
DoesFvFileExist(
  IN EFI_GUID GUID16
)
{
  EFI_STATUS Status;
  EFI_HANDLE *HandleBuffer = NULL;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *FvInstance = NULL;
  VOID *String;
  UINTN BufferSize;
  UINT8 Index = 0;
  UINT8 j = 0;
  //-----------------------------------------------------

  //
  // Locate protocol.
  //
  Status = gBS->LocateHandleBuffer(
    ByProtocol,
    &gEfiFirmwareVolume2ProtocolGuid,
    NULL,
    &BufferSize,
    &HandleBuffer);
  if (EFI_ERROR(Status))
  {
    return FALSE;
  }

  //Debug
  //AsciiPrint("\nFound %d Instances\n\r", BufferSize);

  for (Index = 0; Index < BufferSize; Index++)
  {

    //
    // Get the protocol on this handle
    // This should not fail because of LocateHandleBuffer
    //

    Status = gBS->HandleProtocol(
      HandleBuffer[Index],
      &gEfiFirmwareVolume2ProtocolGuid,
      (VOID **)&FvInstance);

    EFI_FV_FILETYPE FileType = EFI_FV_FILETYPE_ALL;
    EFI_FV_FILE_ATTRIBUTES FileAttributes;
    UINTN FileSize;
    EFI_GUID NameGuid = gZeroGuid;
    VOID *Keys = AllocateZeroPool(FvInstance->KeySize);

    while (j < BufferSize)
    {
      FileType = EFI_FV_FILETYPE_ALL;
      ZeroMem(&NameGuid, sizeof(EFI_GUID));
      Status = FvInstance->GetNextFile(FvInstance, Keys, &FileType, &NameGuid, &FileAttributes, &FileSize);
      if (EFI_ERROR(Status))
      {
        //Prin(L"\nBreaking cause: %r\n\r", Status);

        j++;
        break;
      }

      UINTN StringSize = 0;
      UINT32 AuthenticationStatus;
      String = NULL;
      Status = FvInstance->ReadSection(FvInstance, &NameGuid, EFI_SECTION_USER_INTERFACE, 0, &String, &StringSize, &AuthenticationStatus);

      //Debug
      //Print(L"\nCurrent GUID: %g\n\r", NameGuid); //Current processing guid per While iteration

      if (CompareGuid(&GUID16, &NameGuid))
      {

        //Debug
        //Print(L"\nFound Guid: %g, FileSize: %d, Name: %s, Type: %d\n\r", NameGuid, FileSize, String, FileType);

        FreePool(String);
        FreePool(Keys);
        return TRUE;
      }
      FreePool(String);
    }

    if (Keys != NULL) {
      FreePool(Keys);
    }
  }
  return FALSE;
}

static EFI_HII_PACKAGE_LIST_HEADER *
GetHandlePackageListAndGuid(
  IN CONST EFI_HII_HANDLE HiiHandle,
  OUT EFI_GUID *FileGuid OPTIONAL
)
{
  EFI_STATUS Status;
  EFI_HII_PACKAGE_LIST_HEADER *PackageList = NULL;
  UINTN BufferSize = 0;
  //-----------------------------------------------------

  Status = gHiiDatabase->ExportPackageLists(gHiiDatabase, HiiHandle, &BufferSize, PackageList);
  if (EFI_ERROR(Status))
  {
    PackageList = AllocateZeroPool(BufferSize);
    if (PackageList == NULL) {
      return NULL;
    }

    //Try again after allocation
    Status = gHiiDatabase->ExportPackageLists(gHiiDatabase, HiiHandle, &BufferSize, PackageList);
  }

  //Debug
  //Print(L"HII handle %X ExportPackageLists BufferSize: %X, PackageListGuid: %g, status: %r\n\r", HiiHandle, BufferSize, PackageList->PackageListGuid, Status);

  if (EFI_ERROR(Status))
  {
    if (PackageList != NULL) {
      FreePool(PackageList);
    }
    return NULL;
  }
  else
  {
    if (FileGuid != NULL) {
      EFI_HANDLE DriverHandle;
      UINT8 *DevicePathInfo = AllocateZeroPool(sizeof(UINTN));

      gHiiDatabase->GetPackageListHandle(
        gHiiDatabase,
        HiiHandle,
        &DriverHandle
      );
      if (!EFI_ERROR(Status))
      {

        //Handles got from gBS->OpenProtocol or gBS->HandleProtocol point to HII CONFIG ACCESS, but we need parent
        Status = gBS->HandleProtocol(
          DriverHandle,
          &gEfiDevicePathProtocolGuid,
          (VOID **)DevicePathInfo
        );
        if (EFI_ERROR(Status)) {
          Print(L"\nHandleProtocol status %r\n\n\r", Status);
        }
        else
        {
          VOID *Node = DevicePathInfo;
          *FileGuid = gZeroGuid;
          while (!IsDevicePathEnd(Node)) {
            if (DevicePathType(Node) == MEDIA_DEVICE_PATH &&
              DevicePathSubType(Node) == MEDIA_PIWG_FW_FILE_DP) {
              *FileGuid = ((MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)Node)->FvFileName;
              break;
            }
            Node = NextDevicePathNode(Node);
          }
        }
      }
    }

    return PackageList;
  }
}

static VOID
GetAndDecodeX86LEA(
  IN UINT8 *Start,
  IN CONST INT16 Direction,
  IN UINT8 Count,
  OUT UINT64 ***Services
)
{
#define LEA_SIZE 0x7
#define IS_POSITIVE(NUM) ((NUM) > 0)

  if (IS_POSITIVE(Direction)) { return; }

  CONST UINT8 LeaRaxPattern[] = { 0x48, 0x8D, 0x05 };
  UINT64 **Temp = AllocatePool(sizeof(VOID *) * Count);

  for (INTN i = 0; Count && (i != Direction); i += -1)
  {
    if (CompareMem(Start + i, LeaRaxPattern, sizeof(LeaRaxPattern)) == 0)
    {
      Print(L"\nFound at %X\r\n", Start + i);

      INT32 ServiceEffectiveAddress = *(INT32 *)(Start + i + sizeof(LeaRaxPattern));
      Print(L"ServiceEffectiveAddress %X\r\n", ServiceEffectiveAddress);

      Count--;
      Temp[Count] = (UINTN *)(Start + i + LEA_SIZE + ServiceEffectiveAddress);
      Print(L"Got offset %X\r\n", Temp[Count]);
    }
  }

  if (Count) { FreePool(Temp); Print(L"Error\r\n"); }
  else
  {
    *Services = Temp;
  }

  return;
}

static EFI_HII_CONFIG_ACCESS_PROTOCOL *
FindHiiConfigAccess(
  IN EFI_LOADED_IMAGE_PROTOCOL *ImageInfo,
  IN EFI_HII_CONFIG_ACCESS_PROTOCOL *ConfigAccess
)
{
  CONST UINT8 InstallProtocolInterface[] = { 0xFF, 0x90, 0x80, 0x00, 0x00, 0x00 };
  CONST UINT8 InstallMultipleProtocolInterfaces[] = { 0xFF, 0x90, 0x48, 0x01, 0x00, 0x00 };

  UINT8 *Offset = NULL;
  UINT8 *Call1 = NULL, *Call2 = NULL;
  UINT8 Call1Count = 0, Call2Count = 0;

  for (UINTN i = 0; i < ImageInfo->ImageSize - sizeof(InstallProtocolInterface); i++)
  {
    Offset = (UINT8 *)ImageInfo->ImageBase + i;

    if (CompareMem(Offset, (UINT8 *)InstallProtocolInterface, sizeof(InstallProtocolInterface)) == 0)
    {
      Call1Count++;
      Call1 = Offset;
      continue;
    }
    if (CompareMem(Offset, (UINT8 *)InstallMultipleProtocolInterfaces, sizeof(InstallProtocolInterface)) == 0)
    {
      Print(L"MultipleProtocolInterfaces at %X\r\n", Offset);
      Call2Count++;
      Call2 = Offset;
    }
  }

  if (Call1Count != 1) { Call1 = NULL; }
  if (Call2Count == 1) { Call1 = Call2; }
  if (Call1 == NULL) { Print(L"Ret Call1 == NULL\r\n"); return NULL; }

  GetAndDecodeX86LEA(Call1, (INT16)-256, HII_ACCESS_FIELD_COUNT, (UINT64 ***)&(&ConfigAccess));

  return ConfigAccess;
}

static EFI_HII_HANDLE
HiiConstructAddPackagesSREP(
  IN EFI_LOADED_IMAGE_PROTOCOL *ImageInfo,
  IN VENDOR_DEVICE_PATH *DevicePath,
  IN UINT8 *UniBin,
  IN UINT8 *VfrBin
)
{
  CONST EFI_HII_PACKAGE_HEADER mEndOfPackageList = {
    sizeof(EFI_HII_PACKAGE_HEADER),
    EFI_HII_PACKAGE_END
  };

  EFI_STATUS Status;
  EFI_HII_PACKAGE_LIST_HEADER *PackageListHeader = NULL;
  UINT32 DataLength = 0;
  UINT8 *Data = NULL;
  //-----------------------------------------------------

  //
  //Calculate the length of the packages
  //
  //Get PrependedArrSize and substract UINT32 from the value
  DataLength += (ReadUnaligned32((UINT32 *)UniBin) - sizeof(UINT32));
  DataLength += (ReadUnaligned32((UINT32 *)VfrBin) - sizeof(UINT32));

  //
  // If there are no packages or all the packages
  // are empty, then return a NULL HII Handle
  //
  if (DataLength == 0) {
    //Debug
    //Print(L"There are no packages or all the packages are empty\n\r");
    return NULL;
  }

  //
  //Add the length of the Package List Header and the terminating Package Header
  //
  DataLength += sizeof(EFI_HII_PACKAGE_LIST_HEADER) + sizeof(EFI_HII_PACKAGE_HEADER);

  //
  //Allocate the storage for the entire Package List
  //
  PackageListHeader = AllocateZeroPool(DataLength);

  //
  //If the Package List can not be allocated, then return a NULL HII Handle
  //
  if (PackageListHeader == NULL) {
    //Debug
    //Print(L"The Package List can not be allocated\n");
    return NULL;
  }

  //
  //Fill in the GUID and Length of the Package List Header
  //
  CopyGuid(&PackageListHeader->PackageListGuid, &DevicePath->Guid);
  PackageListHeader->PackageLength = DataLength;

  //
  //Initialize a pointer to the beginning of the Package List data
  //
  Data = (UINT8 *)(PackageListHeader + 1);

  //
  //Copy the data from EACH package
  //
  //Uni
  DataLength = (ReadUnaligned32((UINT32 *)UniBin) - sizeof(UINT32));
  CopyMem(Data, (UINT32 *)UniBin + 1, DataLength);
  //Update pointer
  Data += DataLength;

  //Vfr
  DataLength = (ReadUnaligned32((UINT32 *)VfrBin) - sizeof(UINT32));
  CopyMem(Data, (UINT32 *)VfrBin + 1, DataLength); //Pointer math (UINT32 + 1 = sizeof(UINT32) + sizeof(UINT32))
  //Update pointer
  Data += DataLength;

  //Debug
  //AsciiPrint("HiiAddPackages copied %X bytes of Uni, Vfr and Uni must be: 0x%X / 0x%X\n", ReadUnaligned16((UINT16 *)(PackageListHeader + 1)), ReadUnaligned32((UINT32 *)VfrBin), ReadUnaligned32((UINT32 *)UniBin));

  //
  //Append a package of type EFI_HII_PACKAGE_END to mark the end of the package list
  //
  CopyMem(Data, &mEndOfPackageList, sizeof(mEndOfPackageList));

  //
  //Tie newly created Package List to Config Access
  //
  EFI_HANDLE NewHandle = NULL;
  EFI_HII_HANDLE NewHiiHandle = NULL;
  EFI_HII_CONFIG_ACCESS_PROTOCOL *ConfigAccess = NULL;

  //Debug
  ConfigAccess = FindHiiConfigAccess(ImageInfo, ConfigAccess);

  if (ConfigAccess == NULL) {
    Print(L"NULL\n");

    Status = EFI_NOT_FOUND;
  }
  else
  {
    //Check for ConfigRouting presence
    Status = gBS->LocateProtocol(&gEfiHiiConfigRoutingProtocolGuid, NULL, NULL);
    if (Status == EFI_NOT_FOUND) {
      return NULL;
    }

    Print(L"Protocol %X\r\n", ConfigAccess);
    //EFI_HANDLE TheHandle = ConvertHandleIndexToHandle(0x1C6);
    //Status = gBS->HandleProtocol(TheHandle, &gEfiHiiConfigAccessProtocolGuid, (VOID **)&ConfigAccess);

    Print(L"Protocol %X HiiConfigRouting %r\r\n", ConfigAccess, Status);
    Status = gBS->InstallProtocolInterface( //Success, except I do "load Aod.efi" in shell. Then EFI_ALREADY_STARTED.
      &NewHandle,
      &gEfiDevicePathProtocolGuid,
      EFI_NATIVE_INTERFACE,
      DevicePath
    );
    if (Status == EFI_ALREADY_STARTED) {
      Print(L"%g Device Path\nprotocol instance already present in the database,\nloaded forms may not have a callback induced\n\r", DevicePath->Guid);
    }

    Status = gBS->InstallProtocolInterface( //Always success.
      &NewHandle,
      &gEfiHiiConfigAccessProtocolGuid,
      EFI_NATIVE_INTERFACE,
      ConfigAccess
    );
  }
  Print(L"Config Access install status: %r\n", Status);
  //Debug end

  //
  //Register the package list with the HII Database
  //
  Status = gHiiDatabase->NewPackageList(
    IN gHiiDatabase,
    IN PackageListHeader, //PackageListHeader consists of Header + Data
    IN NewHandle,
    OUT & NewHiiHandle
  );
  if (EFI_ERROR(Status)) {
    Print(L"The Package List is null or Handle is null\n");

    gBS->UninstallMultipleProtocolInterfaces(
      &NewHandle,
      &gEfiHiiConfigAccessProtocolGuid,
      ConfigAccess,
      NULL
    );
  }

  //
  //Free the allocated package list
  //
  FreePool(PackageListHeader);

  //
  //Return the new HII Handle
  //
  return NewHiiHandle;
}

static EFI_STATUS
AddFirstHiiPackageFromFile(
  IN EFI_LOADED_IMAGE_PROTOCOL *ImageInfo
)
{

  ///
  /// HII specific Vendor Device Path definition.
  ///
  ///
  /*
  typedef struct {
    VENDOR_DEVICE_PATH                VendorDevicePath;
    EFI_DEVICE_PATH_PROTOCOL          End;
  } HII_VENDOR_DEVICE_PATH;


  EFI_GUID PackageListGuid = {
    0x12345678, 0xabcd, 0xef01, {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x00}
  };

  HII_VENDOR_DEVICE_PATH mHiiVendorDevicePath0 = {
    {
      { //DevicePathHeaderUint (see utility.h)
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8)(sizeof(VENDOR_DEVICE_PATH)),
          (UINT8)((sizeof(VENDOR_DEVICE_PATH)) >> 8)
        }
      },//DevicePathHeaderUint end
      PackageListGuid
    },
    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      {
        (UINT8)(END_DEVICE_PATH_LENGTH),
        (UINT8)((END_DEVICE_PATH_LENGTH) >> 8)
      }
    }
  };
  */

  EFI_STATUS Status;
  EFI_GUID PackageListGuid = gZeroGuid;
  EFI_HII_HANDLE HiiHandle = NULL;
  //-----------------------------------------------------

  //
  //Locate HII database, retrieve all HII packages to HiiHandleBuffer
  //
  Status = gBS->LocateProtocol(&gEfiHiiDatabaseProtocolGuid, NULL, (VOID **)&gHiiDatabase);
  if (EFI_ERROR(Status)) {

    Print(L"Unable to locate HII Database!\n\r");
    return EFI_UNSUPPORTED;
  }

  //
  //Get Pointers part
  //
  VENDOR_DEVICE_PATH *DevicePathPointer = NULL;
  UINTN *ifrSetupStringsPointer = NULL;
  UINTN *ifrHiiBinPointer = NULL;

  if (ImageInfo->ImageSize > 0) {
    GetSetupPointers(
      ImageInfo,
      &DevicePathPointer,
      &ifrSetupStringsPointer,
      &ifrHiiBinPointer
    );
  }
  else
  {
    Status = EFI_NOT_STARTED;
  }

  if ((Status != EFI_NOT_STARTED) && DevicePathPointer) {
    //
    //Save PackageListGuid from image
    //
    PackageListGuid = DevicePathPointer->Guid;
  }
  else
  {
    //
    //GetSetupPointers failed, return status
    //
    Print(L"Failed getting Device Path GUID and HII packages offsets in the image, ImageBase: 0x%X\n\r", ImageInfo->ImageBase);

    return Status;
  }

  //
  //Publish resources part
  //
  // 
  //mHiiVendorDevicePath0 must be a pointer to dev. path in EFI setup binary. Dev. path is DevicePathHeader + Guid

  //Debug
  //Print(L"4 bytes print for Vfr 0x%X and Uni 0x%X\n\r", *ifrHiiBinPointer, *ifrSetupStringsPointer);

  HiiHandle = HiiConstructAddPackagesSREP(
    ImageInfo,
    DevicePathPointer,
    (UINT8 *)ifrHiiBinPointer,
    (UINT8 *)ifrSetupStringsPointer
  );

  //Debug
  //Print(L"HiiAddPackages done. Handle - %X\n", HiiHandle);


  //
  //HiiHandle checks part
  //
  EFI_HII_HANDLE *HiiHandleBuffer = NULL;
  EFI_HII_PACKAGE_LIST_HEADER *PackageList = NULL;
  UINTN BufferSize;

  gHiiDatabase->ListPackageLists(gHiiDatabase, EFI_HII_PACKAGE_TYPE_ALL, NULL, &BufferSize, NULL);
  HiiHandleBuffer = AllocateZeroPool(BufferSize);
  if (HiiHandleBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //Debug
  //Print(L"HiiHandle instances: %X\n\r", BufferSize / sizeof(EFI_HII_HANDLE));

  Status = gHiiDatabase->ListPackageLists(gHiiDatabase, EFI_HII_PACKAGE_TYPE_ALL, NULL, &BufferSize, HiiHandleBuffer);

  if (EFI_ERROR(Status))
  {
    Print(L"Failed to list HII handles to confirm resources load.\nAttempting again...\n\r");
  }
  else
  {
    //
    //Try retrieving package list in the HII database by guid
    //
    Status = EFI_NOT_FOUND;

    for (UINTN i = 0; i < BufferSize / sizeof(EFI_HII_HANDLE); i++) {
      PackageList = GetHandlePackageListAndGuid(HiiHandleBuffer[i], NULL);

      if (CompareGuid(&PackageListGuid, &PackageList->PackageListGuid)) {
        HiiHandle = HiiHandleBuffer[i];

        Print(L"Search via GetHandlePackageList done. HII Handle - %X\n\r", HiiHandle);
        Status = EFI_SUCCESS;
        break;
      }
    }
  }

  EFI_HANDLE RetrievedHandleFromHii;
  if (HiiHandle != NULL && Status != EFI_SUCCESS) {

    Status = gHiiDatabase->GetPackageListHandle(gHiiDatabase, HiiHandle, &RetrievedHandleFromHii); //Find what handle did the last function create and save to RetrievedHandleFromHii
    if (EFI_ERROR(Status)) {
      Print(L"GetPackageListHandle error\n\r");
    }
    else
    {
      Print(L"Back search via GetPackageListHandle done. EFI Handle - %X\n\r", RetrievedHandleFromHii);

      //Get the HiiPackageList from the Image's Handle
      Status = gBS->HandleProtocol(RetrievedHandleFromHii, &gEfiHiiPackageListProtocolGuid, (VOID **)&PackageList);
      if (EFI_ERROR(Status)) {
        //Debug
        //Print(L"HandleProtocol error, getting HII Package List via GetHandlePackageList...\n\r");
        PackageList = GetHandlePackageListAndGuid(HiiHandle, NULL);
      }

      Status = EFI_SUCCESS;
    }
  }

  //
  //Result print part
  //
  if (HiiHandle == NULL || Status != EFI_SUCCESS)
  {
    Print(L"Couldn't confirm the presence of new HII Handle, the handle is probably null\n\r");
  }
  else
  {
    Print(L"Registered HII Handle %X with Package List:\n%g of size 0x%X at 0x%X\n\r",
      HiiHandle,
      GetHandlePackageListAndGuid(HiiHandle, NULL)->PackageListGuid,
      GetHandlePackageListAndGuid(HiiHandle, NULL)->PackageLength,
      GetHandlePackageListAndGuid(HiiHandle, NULL)
    );
  }

  if (HiiHandleBuffer != NULL) {
    FreePool(HiiHandleBuffer);
  }

  return Status;
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
FindLoadedAod(
  OUT EFI_LOADED_IMAGE_PROTOCOL **ImageInfo
)
{
  EFI_STATUS Status;
  UINTN BufferSize = 0;
  EFI_HANDLE *Handles = NULL;
  //-----------------------------------------------------

  Status = gBS->LocateHandle(ByProtocol, &gEfiLoadedImageProtocolGuid, NULL, &BufferSize, NULL);
  Handles = AllocateZeroPool(BufferSize);
  if (Handles == NULL)
  {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->LocateHandle(ByProtocol, &gEfiLoadedImageProtocolGuid, NULL, &BufferSize, Handles);

  for (UINTN i = 0; i < BufferSize / sizeof(EFI_HANDLE); i++)
  {
    Status = gBS->HandleProtocol(Handles[i], &gEfiLoadedImageProtocolGuid, (VOID **)ImageInfo); //Process every found handle
    if (Status == EFI_SUCCESS)
    {
      CHAR16 *String = FindLoadedImageBuffer(*ImageInfo, EFI_SECTION_USER_INTERFACE, 0, (VOID *)NULL);

      if (String != NULL)
      {
        if (StrStr(L"Aod", String) && StrStr(L"Aod", String))  //Aod driver often has 2 different names, AodDxe and AodSetupDxe
        {

          Print(L"Found Aod at 0x%X\n\r", String, (*ImageInfo)->ImageBase);

          Status = EFI_SUCCESS;
          break;
        }
      }
    }

    Status = EFI_NOT_FOUND;
  }

  if (Handles != NULL) {
    FreePool(Handles);
  }

  return Status;
}

/*Debug
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
*/

EFI_STATUS EFIAPI
Main(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
  EFI_STATUS Status;
  EFI_LOADED_IMAGE_PROTOCOL *ImageInfo = NULL;

  //Debug
  //FindLoadedImageFromShellIndex(0x1C2, &ImageInfo);

  Status = FindLoadedAod(&ImageInfo);
  if (!EFI_ERROR(Status)) {
    Status = AddFirstHiiPackageFromFile(ImageInfo);
  }

  Print(L"%r\n", Status);
  return Status;
}
