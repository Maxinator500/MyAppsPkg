#include "RouteConfigHookerAndLoader.h"

EFI_STATUS
PrintRouteStatus(
  IN EFI_STATUS Status
)
{
  if (FileHandle == NULL) {
    return EFI_NOT_READY;
  }

  UINTN BufferSize = sizeof(EFI_STATUS) * 2 + 1;
  CHAR8 *Buffer = AllocateZeroPool(BufferSize);
  if (Buffer == NULL) gBS->Exit(gImageHandle, 0, 0, 0);

  AsciiSPrint(Buffer, BufferSize, "%X", Status);
  Status = FileHandle->Write(FileHandle, &BufferSize, Buffer);
  FreePool(Buffer);

  return Status;
}

STATIC EFI_STATUS
ExecFromFS(
  IN CHAR16 *FileName
)
{
  EFI_STATUS Status;
  EFI_DEVICE_PATH_PROTOCOL *FilePath = NULL;
  EFI_BLOCK_IO_PROTOCOL *BlkIo = NULL;
  EFI_HANDLE *Handles = NULL;
  EFI_HANDLE AppImageHandle = NULL;
  EFI_SHELL_PARAMETERS_PROTOCOL *ParentParams = NULL;
  EFI_SHELL_PARAMETERS_PROTOCOL ChildParams;
  CHAR16 *ChildArgv[] = { FileName };
  UINTN HandleBufferSize, ExitDataSize;
  //-----------------------------------------------------

  Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &HandleBufferSize, &Handles);
  if (EFI_ERROR(Status))
  {
    return Status;
  }

  for (UINTN Index = 0; Index < HandleBufferSize; Index++)
  {
    Status = gBS->OpenProtocol(
      Handles[Index],
      &gEfiSimpleFileSystemProtocolGuid,
      (VOID **)&BlkIo,
      gImageHandle,
      NULL,
      EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );

    FilePath = FileDevicePath(Handles[Index], FileName);
    Status = gBS->LoadImage(
      FALSE,
      gImageHandle,
      FilePath,
      (VOID *)NULL,
      0,
      &AppImageHandle
    );

    if (!EFI_ERROR(Status))
    {
      break;
    }
  }

  if (Handles != NULL) {
    FreePool(Handles);
  }
  if (EFI_ERROR(Status))
  {
    return Status;
  }

  AsciiPrint("Loading %s.\n\r", FileName);
  Status = gBS->HandleProtocol(
    gImageHandle,
    &gEfiShellParametersProtocolGuid,
    (VOID **)&ParentParams
  );

  ChildParams.Argc = sizeof(ChildArgv);
  ChildParams.Argv = ChildArgv;
  if (!EFI_ERROR(Status) && (ParentParams != NULL)) {
    ChildParams.StdIn = ParentParams->StdIn;
    ChildParams.StdOut = ParentParams->StdOut;
    ChildParams.StdErr = ParentParams->StdErr;
  }
  else {
    ChildParams.StdIn = gST->ConsoleInHandle;
    ChildParams.StdOut = gST->ConsoleOutHandle;
    ChildParams.StdErr = gST->StandardErrorHandle;
  }

  gBS->InstallProtocolInterface(
    &AppImageHandle,
    &gEfiShellParametersProtocolGuid,
    EFI_NATIVE_INTERFACE,
    &ChildParams
  );

  return gBS->StartImage(AppImageHandle, &ExitDataSize, (CHAR16 **)NULL);
}

STATIC EFI_STATUS
SetupLogFile(
  IN BOOLEAN IsLogRequired,
  IN EFI_HANDLE ImageHandle,
  OUT EFI_FILE_PROTOCOL **File
)
{
  if (!IsLogRequired) return EFI_ABORTED;

  EFI_STATUS Status;
  EFI_HANDLE_PROTOCOL HandleProtocol = NULL;
  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage = NULL;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem = NULL;
  EFI_FILE *Root = NULL;
  EFI_FILE_PROTOCOL *TempFile = NULL;

  HandleProtocol = gST->BootServices->HandleProtocol;
  HandleProtocol(gImageHandle, &gEfiLoadedImageProtocolGuid, (void **)&LoadedImage);
  HandleProtocol(LoadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void **)&FileSystem);
  FileSystem->OpenVolume(FileSystem, &Root);

  Status = Root->Open(Root, &TempFile, L"RouteConfigHooker.log", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
  if (EFI_ERROR(Status)) {
    Root->Close(Root);
    return Status;
  }

  *File = TempFile;
  Root->Close(Root);
  if (*File != NULL) {
    (*File)->SetPosition(*File, 0xFFFFFFFFFFFFFFFFULL);
  }
  return EFI_SUCCESS;
}

STATIC EFI_STATUS
WriteLog(
  IN EFI_FILE_PROTOCOL *File,
  IN CHAR8 *Text
)
{
  UINTN Size = AsciiStrLen(Text);
  return File->Write(File, &Size, Text);
}

INTN EFIAPI
ShellAppMain(IN UINTN Argc, IN CHAR16 **Argv)
{
  if (Argc < 2) {
    AsciiPrint("Argument count is invalid: %d.\nUsage: %s <app which opens setup>\n\r", Argc - 1, Argv[0]);
    return EFI_INVALID_PARAMETER;
  }

  EFI_STATUS Status = SetupLogFile(TRUE, gImageHandle, &FileHandle);
  if (EFI_ERROR(Status) && Status != EFI_ABORTED) {
    AsciiPrint("Failed to create log file, start aborted.\n\r");
    return Status;
  }
  AsciiPrint("\n=== Route hack start ===\n\r");
  if (FileHandle != NULL) {
    WriteLog(FileHandle, "\n=== Route hack start ===\n\r");
  }

  Status = gBS->LocateProtocol(&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID **)&HiiConfigRouting);
  AsciiPrint("Locate HII Config Routing: %r\n\r", Status);

  if (FileHandle != NULL) {
    WriteLog(FileHandle, "Locate HII Config Routing: 0x");
    PrintRouteStatus((UINT64)Status);
    WriteLog(FileHandle, "\n\r");
  }
  if (EFI_ERROR(Status) || HiiConfigRouting == NULL || HiiConfigRouting->RouteConfig == NULL) {
    if (FileHandle != NULL) {
      FileHandle->Close(FileHandle);
    }
    return Status;
  }

  #pragma warning(disable : 4047 4113)
  OriginalRouteConfig = HiiConfigRouting->RouteConfig;
  HiiConfigRouting->RouteConfig = &HackedRouteConfig;

  AsciiPrint("RouteConfig compatibility hook installed.\n\r");
  if (FileHandle != NULL) {
    WriteLog(FileHandle, HookInstalled);
    FileHandle->Flush(FileHandle);
  }

  Status = ExecFromFS(Argv[1]);
  if (EFI_ERROR(Status)) {
    AsciiPrint("Couldn't load %s from FS: %r.\n\r", Argv[1], Status);
    if (FileHandle != NULL) {
      WriteLog(FileHandle, "Couldn't load image from FS.\n\r");
    }
  }

  AsciiPrint("RouteConfig interface restored.\n\r");
  if (FileHandle != NULL) {
    WriteLog(FileHandle, "RouteConfig interface restored.\n\r");
  }
  if (HiiConfigRouting != NULL) {
    HiiConfigRouting->RouteConfig = OriginalRouteConfig;
  }

  if (FileHandle != NULL) {
    FileHandle->Close(FileHandle); //Closing file flushes the data
  }
  return Status;
}
