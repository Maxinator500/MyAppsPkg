#pragma once
#include <Uefi.h>
#include <PiDxe.h> 

#define ESA_INTERFACES_FOR_TSE_PROTOCOL_GUID { 0x113fd31a, 0xbe8b, 0x418b, 0xb0, 0x34, 0x7e, 0xaf, 0xe5, 0xc6, 0xc, 0x99 }

constexpr EFI_GUID gEsaInterfacesForTseProtocolGuid = ESA_INTERFACES_FOR_TSE_PROTOCOL_GUID;

using LAUNCH_ESA = EFI_STATUS(EFIAPI*) ();

typedef unsigned char UINT104[104];
struct ESA_INTERFACES_FOR_TSE
{
  UINT104 U1;
  LAUNCH_ESA EnterSetup;
};

using ESA_INTERFACES_FOR_TSE_PROTOCOL = ESA_INTERFACES_FOR_TSE*;
