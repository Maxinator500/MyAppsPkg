extern "C"
{
  #include <Uefi.h>
  #include <PiDxe.h> 
  #include <Library/UefiLib.h>
  #include <Library/PrintLib.h>
  #include <Library/HiiLib.h>
  #include <Library/UefiApplicationEntryPoint.h>
  #include <Library/UefiBootServicesTableLib.h>
  #include <Library/UefiRuntimeServicesTableLib.h>
  #include <Library/UefiHiiServicesLib.h>
  #include <Library/DebugLib.h>
  #include <Library/MemoryAllocationLib.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/ShellCEntryLib.h>
  #include <Protocol/FormBrowser2.h>
  #include <Protocol/FirmwareVolume2.h>
  #include <Guid/ZeroGuid.h>
}

typedef
EFI_STATUS
(EFIAPI *EFI_START_SETUP_UTILITY) (
  IN struct _OBFUSCATED_EFI_SETUP_UTILITY_PROTOCOL *This
);

struct _OBFUSCATED_EFI_SETUP_UTILITY_PROTOCOL {
  UINT64 U1;
  UINT64 U2;
  EFI_START_SETUP_UTILITY Start;
  UINT64 U3;
};
using EFI_SETUP_UTILITY_PROTOCOL = _OBFUSCATED_EFI_SETUP_UTILITY_PROTOCOL*;

static EFI_HII_PACKAGE_LIST_HEADER *
GetHandlePackageList(
  IN EFI_HII_HANDLE HiiHandle
)
{
  EFI_STATUS Status;
  EFI_HII_PACKAGE_LIST_HEADER *PackageList = NULL;
  UINTN BufferSize = 0;

  Status = gHiiDatabase->ExportPackageLists(gHiiDatabase, HiiHandle, &BufferSize, PackageList);
  if (EFI_ERROR(Status))
  {
    //Catch not enough memory here, allocate buffer pool
    PackageList = (EFI_HII_PACKAGE_LIST_HEADER *)AllocateZeroPool(BufferSize);
    if (PackageList == NULL) {
      AsciiPrint("\nFailed to Allocate Pool!\n\r");
      return NULL;
    }

    //Try again after allocation
    Status = gHiiDatabase->ExportPackageLists(gHiiDatabase, HiiHandle, &BufferSize, PackageList);
  }

  DEBUG_CODE(
    //Debug
    AsciiPrint("\nHII handle %X ExportPackageLists BufferSize: %X, PackageListGuid: %g, status: %r\n\r", HiiHandle, BufferSize, PackageList->PackageListGuid, Status);
  );

  if (EFI_ERROR(Status))
  {
    DEBUG_CODE(
      //Debug
      AsciiPrint("\nFailed to get HII handle packages! %r\n\r", Status);
    );
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

static BOOLEAN
DoesSupportFormsetGuidByPackageBuffer(
  IN EFI_GUID *FormSetGuid,
  IN UINT8 *Package
)
{
  EFI_HII_PACKAGE_HEADER *PackageHeader;
  UINT32 Offset;
  UINT8 *OpCodeData;
  UINT8 ClassGuidNum;
  EFI_GUID *ClassGuid;


  if (FormSetGuid == NULL || Package == NULL) {
    return FALSE;
  }

  PackageHeader = (EFI_HII_PACKAGE_HEADER *)Package;
  if (PackageHeader->Type != EFI_HII_PACKAGE_FORMS) {
    return FALSE;
  }

  Offset = sizeof(EFI_HII_PACKAGE_HEADER);
  while (Offset < PackageHeader->Length) {
    OpCodeData = Package + Offset;

    if (((EFI_IFR_OP_HEADER *)OpCodeData)->OpCode == EFI_IFR_FORM_SET_OP &&
      ((EFI_IFR_OP_HEADER *)OpCodeData)->Length > OFFSET_OF(EFI_IFR_FORM_SET, Flags)) {
      ClassGuidNum = (UINT8)(((EFI_IFR_FORM_SET *)OpCodeData)->Flags & 0x3);
      ClassGuid = (EFI_GUID *)(VOID *)(OpCodeData + sizeof(EFI_IFR_OP_HEADER));

      while (ClassGuidNum > 0) {
        if (CompareGuid(ClassGuid, FormSetGuid)) {
          return TRUE;
        }
        ClassGuidNum--;
        ClassGuid++;
      }
    }

    Offset += ((EFI_IFR_OP_HEADER *)OpCodeData)->Length;
  }

  return FALSE;
}

static BOOLEAN
DoesSupportFormsetGuidByHandle(
  IN EFI_GUID *FormSetGuid,
  IN EFI_HII_HANDLE HiiHandle
)
{
  EFI_STATUS Status;
  UINTN HiiPackageListSize;
  EFI_HII_PACKAGE_LIST_HEADER *HiiPackageList;
  UINT8 *Package;
  EFI_HII_PACKAGE_HEADER *PackageHeader;
  UINT32 Offset;
  UINT32 PackageListLength;


  if (HiiHandle == NULL || FormSetGuid == NULL) {
    return FALSE;
  }

  //
  // Get HII package list
  //
  HiiPackageList = NULL;
  HiiPackageListSize = 0;
  Status = gHiiDatabase->ExportPackageLists(gHiiDatabase, HiiHandle, &HiiPackageListSize, HiiPackageList);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return FALSE;
  }
  HiiPackageList = (EFI_HII_PACKAGE_LIST_HEADER*)AllocateZeroPool(HiiPackageListSize);
  if (HiiPackageList == NULL) {
    return FALSE;
  }
  Status = gHiiDatabase->ExportPackageLists(gHiiDatabase, HiiHandle, &HiiPackageListSize, HiiPackageList);
  if (EFI_ERROR (Status)) {
    gBS->FreePool(HiiPackageList);
    return FALSE;
  }

  //
  // In HII package list, find the Form package which support specific form set class GUID.
  //
  Offset = sizeof(EFI_HII_PACKAGE_LIST_HEADER);
  PackageListLength = ReadUnaligned32(&HiiPackageList->PackageLength);
  while (Offset < PackageListLength) {
    Package = (UINT8 *)HiiPackageList + Offset;
    PackageHeader = (EFI_HII_PACKAGE_HEADER *) Package;

    if (PackageHeader->Type == EFI_HII_PACKAGE_FORMS &&
        DoesSupportFormsetGuidByPackageBuffer(FormSetGuid, Package)){
      gBS->FreePool(HiiPackageList);
      return TRUE;
    }

    Offset += PackageHeader->Length;
  }

  gBS->FreePool(HiiPackageList);

  return FALSE;
}

static EFI_STATUS
GetAllHiiHandlesByFormsetGuid(
  IN EFI_GUID *FormSetGuid,
  OUT EFI_HII_HANDLE **HiiHandleList,
  OUT UINTN *HiiHandleListCount
)
{
  EFI_STATUS Status;
  EFI_HII_HANDLE *HiiHandles;
  EFI_HII_HANDLE *ListBuffer;
  EFI_HII_HANDLE *Buffer;
  UINTN ListCount;
  UINTN Index;
  UINTN Num;


  HiiHandles = HiiGetHiiHandles(NULL);
  if (HiiHandles == NULL) {
    return EFI_NOT_FOUND;
  }

  ListCount = 10;
  ListBuffer = (EFI_HII_HANDLE *)AllocatePool(ListCount * sizeof(EFI_HII_HANDLE));
  if (ListBuffer == NULL) {
    gBS->FreePool(HiiHandles);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = EFI_NOT_FOUND;
  Num = 0;
  for (Index = 0; HiiHandles[Index] != NULL; Index++) {
    if (DoesSupportFormsetGuidByHandle(FormSetGuid, HiiHandles[Index])) {
      if (Num == ListCount) {
        ListCount += 10;
        Buffer = (EFI_HII_HANDLE *)ReallocatePool(
                   Num * sizeof(EFI_HII_HANDLE),
                   ListCount * sizeof(EFI_HII_HANDLE),
                   ListBuffer
                   );
        if (Buffer == NULL) {
          gBS->FreePool(HiiHandles);
          gBS->FreePool(ListBuffer);
          return EFI_OUT_OF_RESOURCES;
        }

        ListBuffer = Buffer;
      }

      ListBuffer[Num] = HiiHandles[Index];
      Num++;
    }
  }

  if (Num != 0) {
    Buffer = (EFI_HII_HANDLE *)AllocateCopyPool(Num * sizeof(EFI_HII_HANDLE), ListBuffer);
    if (Buffer == NULL) {
      gBS->FreePool(HiiHandles);
      gBS->FreePool(ListBuffer);
      return EFI_OUT_OF_RESOURCES;
    }

    *HiiHandleList = Buffer;
    *HiiHandleListCount = Num;
    Status = EFI_SUCCESS;
  }

  gBS->FreePool(HiiHandles);
  gBS->FreePool(ListBuffer);

  return Status;
}


INTN EFIAPI
ShellAppMain(IN UINTN Argc, IN CHAR16 **Argv) {

    AsciiPrint("Started.\n");

    /*-----------------------------------------------------------------------------------*/
    //
    //Init variables
    //
    EFI_STATUS Status;
    EFI_SETUP_UTILITY_PROTOCOL SetupUtility = NULL;
    EFI_FORM_BROWSER2_PROTOCOL *FormBrowser2 = NULL;
    EFI_HII_PACKAGE_LIST_HEADER *PackageList = NULL;
    EFI_HII_HANDLE *HiiHandles = NULL;
    UINTN HiiHandlesCount = 0x1;
    EFI_BROWSER_ACTION_REQUEST ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
    UINT16 FormId = 0x0;
    //
    EFI_GUID AptioPackageListGuid = { 0x899407d7, 0x99fe, 0x43d8, 0x9a, 0x21, 0x79, 0xec, 0x32, 0x8c, 0xac, 0x21 };
    EFI_GUID InsydePackageListGuid = { 0xfe3542fe, 0xc1d3, 0x4ef8, 0x65, 0x7c, 0x80, 0x48, 0x60, 0x6f, 0xf6, 0x70 };
    EFI_GUID FormSetGuid = gZeroGuid, PackageListGuid = gZeroGuid;

    /*-----------------------------------------------------------------------------------*/
    //
    //Attempt to locate FB
    //
    Status = gBS->LocateProtocol(&gEfiFormBrowser2ProtocolGuid, NULL, (VOID **)&FormBrowser2);
    if (EFI_ERROR(Status))
    {
      AsciiPrint("Failed to locate protocol: %r.\n", Status);
      return Status;
    }

    /*-----------------------------------------------------------------------------------*/
    //
    //Process arguments
    //
    if (Argc != 3)
    {
      AsciiPrint("Usage: %s <FormSetGuid> <FormId>\nwhen filled, manual selection mode is active.\n\n", Argv[0]);
      Status = EFI_INVALID_PARAMETER;
    }
    else
    {
      //
      //Locate HiiHandles by GUID input
      //
      RETURN_STATUS StrToGuidStatus = StrToGuid(Argv[1], &FormSetGuid);
      RETURN_STATUS StrHexToUintnStatus = StrHexToUintnS(Argv[2], NULL, (UINTN *)&FormId);

      if (!RETURN_ERROR(StrToGuidStatus) && !RETURN_ERROR(StrHexToUintnStatus)) {

        Status = GetAllHiiHandlesByFormsetGuid(&FormSetGuid, &HiiHandles, &HiiHandlesCount);

        if (!EFI_ERROR(Status)) {
          AsciiPrint("FormSet GUID is set to: %g.\n", FormSetGuid);

          PackageList = GetHandlePackageList(HiiHandles[0]); //If reached here, we don't care if PackageList got from HiiHandles[0], as only PackageListGuid will be needed
        }
        else
        {
          AsciiPrint("Failed to locate HiiHandles with FormSet: %g, proceeding with defaults.\n", FormSetGuid);
        }
      }
      else
      {
        AsciiPrint("Failed converting input strings: %S - %r and %S - %r.\n", Argv[1], StrToGuidStatus, Argv[2], StrHexToUintnStatus);
        Status = EFI_INVALID_PARAMETER;
      }
    }
    gBS->Stall(3000000);


    /*-----------------------------------------------------------------------------------*/
    //
    //Locate HiiHandles if manual failed
    //

    if (EFI_ERROR(Status)) {
      //
      //Select default guids if not specified by user
      //
      if (HiiGetHiiHandles(&AptioPackageListGuid)) {
        PackageListGuid = AptioPackageListGuid;
      }
      else if (HiiGetHiiHandles(&InsydePackageListGuid)) {
        PackageListGuid = InsydePackageListGuid;

        //
        //Try to start Insyde setup by the old spec (rev 3.5)
        //
        if (!EFI_ERROR(gBS->LocateProtocol(const_cast<EFI_GUID *>(&InsydePackageListGuid),
                                          nullptr,
                                          reinterpret_cast<VOID **>(&SetupUtility)
                                          )
                      )
           )
        {
          SetupUtility->Start(SetupUtility);
        }
      }

      if (!IsZeroGuid(&PackageListGuid)) {
        HiiHandles = HiiGetHiiHandles(&PackageListGuid);
      }
      if (HiiHandles == NULL) {
        AsciiPrint("Trying to find any handle... ");

        HiiHandles = HiiGetHiiHandles(NULL);
        if (HiiHandles == NULL) {
          AsciiPrint("failed. HII DB probably not populated at all.\n");
          return EFI_NOT_FOUND;
        }

        AsciiPrint("success!\n");
      }

      for (UINTN i = 0; HiiHandles[i] != NULL; i++){
        //
        // In HII package list, find the package which has form set op code
        //
        PackageList = GetHandlePackageList(HiiHandles[i]);

        UINTN Offset = sizeof(EFI_HII_PACKAGE_LIST_HEADER);
        UINTN PackageListLength = ReadUnaligned32(&PackageList->PackageLength);
        while (Offset < PackageListLength) {
          UINT8 *Package = (UINT8 *)PackageList + Offset;
          EFI_HII_PACKAGE_HEADER *PackageHeader = (EFI_HII_PACKAGE_HEADER *)Package;

          if (PackageHeader->Type == EFI_HII_PACKAGE_FORMS) { //opcode 02
            AsciiPrint("Found Package with FormSet GUID %g.\n", (EFI_GUID *)(VOID *)(Package + sizeof(EFI_HII_PACKAGE_HEADER) + sizeof(EFI_IFR_OP_HEADER))); //offset + sizeof(12 34 0 02) + sizeof(0E A7)
            break;
          }

          Offset += PackageHeader->Length;
        }
      }
    }

    if (Status == EFI_SUCCESS && Argc == 2)
    {
      AsciiPrint("Located PackageList: %g with specified FormSet GUID: %g.\n", PackageList->PackageListGuid, FormSetGuid);
    }

    if (PackageList != NULL) {
      AsciiPrint("Sending PackageList: %g,\nof HiiHandle 0x%X.\n", PackageList->PackageListGuid, HiiHandles);
    }
    else {
      //Mustn't reach here
      AsciiPrint("Found HiiHandles 0x%X don't match any Package anymore.\n", HiiHandles);
      return EFI_NOT_FOUND;
    }

    gBS->Stall(3000000);
    Status = FormBrowser2->SendForm(
                             FormBrowser2,
                             HiiHandles,
                             HiiHandlesCount,
                             &PackageList->PackageListGuid,
                             FormId,
                             NULL,
                             &ActionRequest
                             );

    if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
      AsciiPrint("SendForm got reset request: %r.\n", Status);
      gBS->RaiseTPL(TPL_NOTIFY);
      gRT->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
    }
    if (HiiHandles != NULL) {
      FreePool(HiiHandles);
    }
    if (PackageList != NULL) {
      FreePool(PackageList);
    }

    gST->ConOut->SetAttribute(gST->ConOut, EFI_BLACK | EFI_BACKGROUND_BLACK);
    gST->ConOut->ClearScreen(gST->ConOut);

    gBS->FreePool(HiiHandles);

    AsciiPrint("Failed to display forms, FormBrowser may not support SendForm.\n");
    return Status;
}
