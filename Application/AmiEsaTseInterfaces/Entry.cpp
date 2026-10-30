extern "C"
{
  #include <Uefi.h>
  #include <PiDxe.h> 
  #include <Library/UefiLib.h>
  #include <Library/UefiApplicationEntryPoint.h>
  #include <Library/UefiBootServicesTableLib.h>
  #include <Library/PrintLib.h>
}
#include "EsaInterfacesForTseProtocol.hpp"

EFI_STATUS EFIAPI
Main(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {

    AsciiPrint("Started, trying to locate: %g.\n", gEsaInterfacesForTseProtocolGuid);

    ESA_INTERFACES_FOR_TSE_PROTOCOL AmiEsa = nullptr;
    EFI_STATUS Status = gBS->LocateProtocol(const_cast<EFI_GUID*>(&gEsaInterfacesForTseProtocolGuid), nullptr, reinterpret_cast<VOID**>(&AmiEsa));

    if (EFI_ERROR(Status))
    {
        AsciiPrint("Failed to locate protocol: %r.\n", Status);
        return Status;
    }

    AmiEsa->EnterSetup();

    return EFI_SUCCESS;
}
