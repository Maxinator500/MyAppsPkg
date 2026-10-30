#include "App.hpp"

EFI_STATUS EFIAPI
Main(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {

    AsciiPrint("\rStarted.\n");
    //Default vars
    EFI_STATUS Status;

    //ROM read vars
    UINT8 *Buffer = NULL;
    UINTN Size = (1 << 24); //2^24
    UINTN PBase = (UINT32)(0 - Size);
    UINT8 VolumesCount = 0;

    //Filesave vars
    EFI_HANDLE_PROTOCOL HandleProtocol;
    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage = NULL;;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem = NULL;
    EFI_FILE *Root;
    EFI_FILE *DumpFile = NULL;
    HandleProtocol = SystemTable->BootServices->HandleProtocol;
    HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (void **)&LoadedImage);
    HandleProtocol(LoadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void **)&FileSystem);
    FileSystem->OpenVolume(FileSystem, &Root);

    //SMBIOS vars
    EFI_SMBIOS_HANDLE SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
    EFI_SMBIOS_PROTOCOL *Smbios;
    EFI_SMBIOS_TABLE_HEADER *Record;
    SMBIOS_TABLE_TYPE0 *Type0Record;

    //Locate Smbios protocol
    Status = gBS->LocateProtocol(&gEfiSmbiosProtocolGuid, NULL, (VOID **)&Smbios);
    if (EFI_ERROR(Status))
    {
      AsciiPrint("\rFailed to locate protocol: %r.\n", Status);
    }
    else
    {
      //Get Smbios table header saved to Record var.
      Status = Smbios->GetNext(Smbios, &SmbiosHandle, NULL, &Record, NULL);
    }

    //Get ROM size
    if (!EFI_ERROR(Status) && Record->Type == SMBIOS_TYPE_BIOS_INFORMATION) {
      Type0Record = (SMBIOS_TABLE_TYPE0 *)Record;
      Size = (1 << 16) * (Type0Record->BiosSize + 1); //2^16 * Type0Record->BiosSize
      PBase = (UINT32)(0 - Size);

      AsciiPrint("\rBIOS ROM Size: %d Bytes. BIOS ROM base: 0x%X.\n", Size, PBase);
    }

    if (EFI_ERROR(Status) || (Size < (1 << 24)))
    {
      Size = 1 << 24;

      AsciiPrint("\rInvalid BIOS ROM Size, enforcing\nsize to: %d Bytes, ROM base to: 0x%X.\n", Size, PBase);

      //Search
      for (UINTN i = 0; i < Size; i++)
      {
        if (!(i % 0x100000)) {
          DisplayUpdateProgressSREP((UINTN)(((float)i / Size) * 100));
        }

        if (!CompareMem((UINT8 *)PBase + i, (UINT8 *)_FVHSIG, sizeof(_FVHSIG)))
        {
          /*Debug
          for (UINTN j = 0; j < 0x10; j++) {
            AsciiPrint("%02X ", *(UINT8 *)(PBase + i + j));
          }
          */
          //AsciiPrint("\n\rFound volume %g\nat 0x%X, with size 0x%X.\n", g, (UINTN *)(PBase + i - 0x28), *(UINTN *)(PBase + i - 0x28 + 0x20));

          VolumesCount++;
        }
      }

      DisplayUpdateProgressSREP(100);
      AsciiPrint("\n\rFound %d volumes.\n", VolumesCount );
    }

    if (!EFI_ERROR(Status) || VolumesCount) {
      AsciiPrint("\rSaving dump...\n" );

      Status = Root->Open(Root, &DumpFile, (CHAR16 *)L"Dump.bin", EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ | EFI_FILE_MODE_CREATE, 0);
      if (EFI_ERROR(Status))
      {
        AsciiPrint("\rFailed to create new file: %r.\n", Status);
        return Status;
      }

      gBS->AllocatePool(EfiBootServicesData, Size, (VOID **)&Buffer);
      CopyMem(Buffer, (UINT8 *)PBase, Size);

      DumpFile->Write(DumpFile, &Size, Buffer);
      DumpFile->Flush(DumpFile);
      DumpFile->Close(DumpFile);

      if (EFI_ERROR(Status))
      {
          AsciiPrint("\rFailed to write file: %r.\n", Status);
      }
    }
    else
    {
        AsciiPrint("\rFailed to locate any UEFI volume.\n");
        Status = EFI_UNSUPPORTED;
    }
    
    gBS->FreePool(Buffer);
    DumpFile = NULL;

    return Status;
}
