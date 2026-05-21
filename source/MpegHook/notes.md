# Finding the instructions to hook

## Way that is more of a hack but works (Currently implemented)

We have a method that calls sceMpegCreate, we scan that for a JAL, we follow that JAL to a trampoline which is a J.
The value of this trampoline is always the same (It jumps to real sceMpegCreate I assume).
So we remember that value.

Then to hook - scan all game code for JALs, if their destination has the same value as our trampoline then it's a match and can hook it.

## Way that is probably nicer I haven't tried
On devices we could use kuKernelFindModuleByName on the game module, loop its stubs (ref modulemgr_patches.c) and then hook all those we want.

We can't do this on PPSSPP because ku isn't supported there (according to the base injector docs).
Maybe we could use sceKernelFindModuleByName instead? Haven't tested it. (Shouldn't be allowed for user modules)

https://github.com/pspdev/psplinkusb/blob/master/psplink/apihook.c
https://github.com/pspdev/psplinkusb/blob/master/psplink/libs.c
https://github.com/pspdev/kubridge/blob/main/include/kubridge.h
SceModule def https://github.com/pspdev/pspsdk/blob/master/src/kernel/psploadcore.h#L78
Example of modifying a target, should be able to use this to find the address https://github.com/pspdev/pspsdk/blob/master/src/sdk/modulemgr_patches.c
It actually looks like the module we want to get is one that imports the target. injector_setup.c already gets the game module on device. Not sure what to do on PPSSPP though. Maybe call sceKernelFindModuleByName direct?
