#pragma once

#include <Uefi.h>
#include <PiDxe.h> 
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/PrintLib.h>
#include <Library/ShellLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/BlockIo.h>
#include <Guid/GlobalVariable.h>

EFI_HII_CONFIG_ROUTING_PROTOCOL *HiiConfigRouting;
EFI_HII_ROUTE_CONFIG OriginalRouteConfig;
EFI_FILE_PROTOCOL *FileHandle = NULL;

CONST CHAR8 GlobalVariableGuid[] =
"GUID=61dfe48bca93d211aa0d00e098032b8c";

CONST CHAR8 VendorKeys[] =
"&NAME=00560065006e0064006f0072004b006500790073";

CONST CHAR8 SetupMode[] =
"&NAME=00530065007400750070004d006f00640065";

CONST CHAR8 SecureBoot[] =
"&NAME=0053006500630075007200650042006f006f0074";

CONST CHAR8 Original[] = "ORIGINAL=0x";
CONST CHAR8 Returned[] = " RETURNED=0x";
CONST CHAR8 NA[] = "NA";
CONST CHAR8 Null[] = "<NULL>";
CONST CHAR8 AtProgress[] = "AT_PROGRESS=";
CONST CHAR8 ProgressChars[] = " PROGRESS_CHARS=";
CONST CHAR8 CFG[] = " CFG=";
CONST CHAR8 Bypassed[] = " BYPASSED=";
CONST CHAR8 Zero[] = "0";
CHAR8 HookInstalled[] = "RouteConfig compatibility hook installed.\n\r";

EFI_STATUS PrintRouteStatus(
  IN EFI_STATUS Status
);
extern EFI_HII_ROUTE_CONFIG HackedRouteConfig();
