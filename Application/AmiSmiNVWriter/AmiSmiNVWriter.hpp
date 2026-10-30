#pragma once

extern "C"
{
    #include <Uefi.h>
    #include <PiDxe.h> 
    #include <Library/UefiLib.h>
    #include <Library/UefiApplicationEntryPoint.h>
    #include <Library/UefiBootServicesTableLib.h>
    #include <Library/UefiRuntimeServicesTableLib.h>
    #include <Library/BaseMemoryLib.h>
    #include <Library/MemoryAllocationLib.h>
    #include <Library/IoLib.h>
    #include <Library/ShellCEntryLib.h>
    #include <Guid/GlobalVariable.h>

    #include <sal.h>
    VOID EFIAPI SendRequest(UINT32 Data);
    UINT8 EFIAPI GetAh();
}

#define FUNCTION_NUMBER                     0xef
#define GET_VARIABLE_SUBFUNCTION            0x01
#define GET_NEXT_VARIABLE_SUBFUNCTION       0x02
#define SET_VARIABLE_SUBFUNCTION            0x03
#define EXPORT_DATABASE_SUBFUNCTION         0x04
#define COPY_MEMORY_SUBFUNCTION             0x05

#define NO_ERROR         0x00
#define INVALID_VALUE    0x82
#define BUFFER_TOO_SMALL 0x85
#define NOT_SUPPORTED    0x86
#define DEVICE_ERROR     0x87
#define NOT_FOUND        0x8e
#define ACCESS_DENIED    0x8f

#define CARRY_FLAG       0x00000001
