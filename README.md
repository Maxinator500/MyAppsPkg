A few helpful apps

# AmiEsaTseInterfaces
Accepts no arguments.</br>
Calls Enter Setup service. Self explanatory.

# AmiSetupVolatileDataSwitcher
Accepts 1 mandatory argument. The argument is offset in "SetupVolatileData" storage, usually left unprotected.</br>
Just an app to quickly switch a value from true to false and vice versa. It assigns Non Volatile attribute to a storage supposed being volatile, thus breaking the mechanism which resets values to default.

# AmiSmiNVWriter
Accepts 2 mandatory arguments. The 1st argument is offset in Ami "Setup" storage, the 2nd is strictly INT8 value.</br>
The app is an attempt to utilize SMI command port from FADT table.

# AodLoader
Accepts no arguments.</br>
An app that searches for Aod DXE on firmware volume, patches its buffer removing compatibility checks, and then starts it.

# GetMemoryUintn
Accepts as much arguments as Shell command line limit or your memory allows.
That's what a man makes when debugging remotely gets harder.

# MySuppressIfPatcher
Accepts no arguments.</br>
First it searches for HII handles, gets their parent EFI handles, then saves both. Second, blindly patches any "suppress if" statement in the handles' buffer.

# ResetApp
Accepts 1 optional argument.</br>
Pass the line "OsIndications" as argument and ResetApp reboots to UEFI. Pass no argument and it reboots with nothing else.

# RouteConfigHookerAndLoader
Accepts 1 mandatory argument. The argument is efi app on FS storage, which loads Setup Utility (e.g. UMAF).</br>
A copy of [RouteFixLoader](https://rog-forum.asus.com/t5/gaming-notebooks/scar-strix-advanced-bios/m-p/1155671/highlight/true#M15077), but with customizable loader name.

# SetupApp
Accepts 2 optional arguments. The 1st must be FormSet GUID, the 2nd must be FormId.</br>
Basically "FormBrowser2->SendForm" in a wrapper. Sends the hardcoded values if the 2 arguments weren't provided.

# UEFIDirtyDumper
Accepts no arguments.</br>
Saves UEFI image from RAM to root FS.