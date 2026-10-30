# AmiEsaTseInterfaces
Accepts no arguments.
Calls Enter Setup service. Self explanatory.

# AmiSetupVolatileDataSwitcher
Accepts 1 mandatory argument. The argument is offset in "SetupVolatileData" storage, usually left unprotected.
Just an app to quickly switch a value from true to false and vice versa. It assigns Non Volatile attribute to a storage supposed being volatile, thus breaking the mechanism which resets values to default.

# AmiSmiNVWriter
Accepts 2 mandatory argument. The 1st argument is offset in Ami "Setup" storage, the 2nd is strictly INT8 value.
The app is an attempt to utilize SMI command port from FADT table.

# AodLoader
Accepts no arguments.
An app created a day ago that searches for existing Aod handle. Then loads its HII resources again.

# GetMemoryUintn
Accepts as much arguments as Shell command line limit or your memory allows.
That's what a man makes when debugging remotely gets harder.

# MySuppressIfPatcher
Accepts no arguments.
First it searches for HII handles, gets their parent EFI handles, then saves both. Second, blindly patches any "suppress if" statement in the handles' buffer.

# ResetApp
Accepts 1 optional argument.
Pass the line "OsIndications" as argument and ResetApp reboots to UEFI. Pass no argument and it reboots with nothing else.

# SetupApp
Accepts 2 optional arguments. The 1st must be FormSet GUID, the 2nd must be FormId.
Basically "FormBrowser2->SendForm" in a wrapper. Sends the biggest package the app could find if the 2 arguments weren't provided.

# UEFIDirtyDumper
Accepts no arguments.
Saves UEFI image from RAM to root FS.