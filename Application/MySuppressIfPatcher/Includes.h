#pragma once

#include <Uefi.h>
#include <PiDxe.h> 
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/HandleParsingLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/LoadedImage.h>
#include <Guid/ZeroGuid.h>

EFI_GUID AptioPackageListGuid = { 0x899407d7, 0x99fe, 0x43d8, 0x9a, 0x21, 0x79, 0xec, 0x32, 0x8c, 0xac, 0x21 };
EFI_GUID InsydePackageListGuid = { 0xfe3542fe, 0xc1d3, 0x4ef8, 0x65, 0x7c, 0x80, 0x48, 0x60, 0x6f, 0xf6, 0x70 };
