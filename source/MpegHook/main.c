#include <pspuser.h>
#include <pspmpeg.h>
#include <kubridge.h>
#include "injector_setup.h"
#include "../../includes/psp/injector.h"
#include "module_config.h"

typedef struct HookRecord
{
    uintptr_t addr_call;
    uintptr_t inst_at_dest;
    uintptr_t inst_call;
} HookRecord;

HookRecord hookMpegCreate = {0};
HookRecord hookMpegRingbufferPut = {0};
sceMpegRingbufferCB originalRingbufferCallback = NULL;

//https://pspdev.github.io/pspsdk/pspmpeg_8h.html

void dumpRingbuffer(SceMpegRingbuffer* ringbuffer, char* prefix)
{
    sceKernelPrintf("Ringbuffer %s\nPackets: %i  pRead: %i  pWritePos: %i  pAvail: %i\n", prefix, (int)ringbuffer->iPackets, (int)ringbuffer->iUnk0, (int)ringbuffer->iUnk1, (int)ringbuffer->iUnk2);
    sceKernelPrintf("pSize: %i  data: %p  dataUpperBound: %p\n", (int)ringbuffer->iUnk3, ringbuffer->pData, (ScePVoid)ringbuffer->iUnk4);
    //There are other fields
}

SceInt32 intercept_ringbufferCallback(ScePVoid pData, SceInt32 iNumPackets, ScePVoid pParam)
{
    sceKernelPrintf("Ringbuffer callback called with pData=0x%p, iNumPackets=%i, pParam=0x%p\n", pData, (int)iNumPackets, pParam);

    //Call original callback
    SceInt32 result = originalRingbufferCallback(pData, iNumPackets, pParam);

    if (result > iNumPackets)
    {
        sceKernelPrintf("Warning: callback wrote more packets than allowed! iNumPackets=%i, result=%i\n", (int)iNumPackets, (int)result);
    }
    else
    {
        sceKernelPrintf("Ringbuffer callback returned %i\n", (int)result);
    }

    return result;
}

/*
ldstart MpegHook.prx
modstun @MpegHook


On PPSSPP:
08:49:896 user_main    I[PRINTF]: HLE\sceKernelMemory.cpp:1035 0=sceKernelPrintf(sceMpegRingbufferPut called with Ringbuffer=0x%p, iNumPackets=%i, iAvailable=%i, freespace=%i
): "sceMpegRingbufferPut called with Ringbuffer=0x0905f5fc, iNumPackets=128, iAvailable=44, freespace=44"
08:49:896 user_main    I[PRINTF]: HLE\sceKernelMemory.cpp:1035 0=sceKernelPrintf(Ringbuffer callback called with pData=0x%p, iNumPackets=%i, pParam=0x%p
): "Ringbuffer callback called with pData=0x099d0640, iNumPackets=44, pParam=0x0905f560"
08:49:896 user_main    I[PRINTF]: HLE\sceKernelMemory.cpp:1035 0=sceKernelPrintf(Warning: callback wrote more packets than allowed! iNumPackets=%i, result=%i
): "Warning: callback wrote more packets than allowed! iNumPackets=44, result=56"
*/

SceInt32 fake_sceMpegCreate(SceMpeg *Mpeg, ScePVoid pData, SceInt32 iSize, SceMpegRingbuffer *Ringbuffer, SceInt32 iFrameWidth, SceInt32 mode, SceInt32 ddrTop)
{
    sceKernelPrintf("sceMpegCreate called with Mpeg=0x%p, pData=0x%p, iSize=%i, Ringbuffer=0x%p", Mpeg, pData, (int)iSize, Ringbuffer);
    sceKernelPrintf(", iFrameWidth=%i, mode=%i, ddrTop=%i\n", (int)iFrameWidth, (int)mode, (int)ddrTop);
    sceKernelPrintf("Ringbuffer callback: 0x%p, Param: 0x%p\n", Ringbuffer->Callback, Ringbuffer->pCBparam);
    dumpRingbuffer(Ringbuffer, "before");

    //Replace callback with our own
    originalRingbufferCallback = Ringbuffer->Callback;
    Ringbuffer->Callback = intercept_ringbufferCallback;

    SceInt32 result = sceMpegCreate(Mpeg, pData, iSize, Ringbuffer, iFrameWidth, mode, ddrTop);

    sceKernelPrintf("sceMpegCreate returned %i\n", (int)result);
    dumpRingbuffer(Ringbuffer, "after");

    return result;
}
SceInt32 fake_sceMpegRingbufferPut(SceMpegRingbuffer* Ringbuffer, SceInt32 iNumPackets, SceInt32 iAvailable)
{
    //TODO: On PSP freespace is always 320, but on PPSSPP it decreases, matching iAvailable. Log each arg and see what they are like
    sceKernelPrintf("sceMpegRingbufferPut called with Ringbuffer=0x%p, iNumPackets=%i, iAvailable=%i, RBiPackets=%i RBPacketsAvail=%i\n", Ringbuffer, (int)iNumPackets, (int)iAvailable, (int)Ringbuffer->iPackets, (int)Ringbuffer->iUnk2);

    if ((iNumPackets < iAvailable ? iNumPackets : iAvailable) > (Ringbuffer->iPackets - Ringbuffer->iUnk2))
    {
        sceKernelPrintf("Warning: trying to put more packets than the ringbuffer can handle! iNumPackets=%i, iAvailable=%i, ringbuffer free space=%i\n", (int)iNumPackets, (int)iAvailable, (int)(Ringbuffer->iPackets - Ringbuffer->iUnk2));
    }

    // dumpRingbuffer(Ringbuffer, "before");

    SceInt32 result = sceMpegRingbufferPut(Ringbuffer, iNumPackets, iAvailable);

    sceKernelPrintf("sceMpegRingbufferPut returned %i\n", (int)result);
    // dumpRingbuffer(Ringbuffer, "after");

    return result;
}

#pragma region passthroughs
#pragma GCC push_options
#pragma GCC optimize ("O0")
SceInt32 passthrough_sceMpegCreate(SceMpeg *Mpeg, ScePVoid pData, SceInt32 iSize, SceMpegRingbuffer *Ringbuffer, SceInt32 iFrameWidth, SceInt32 mode, SceInt32 ddrTop)
{
    return sceMpegCreate(Mpeg, pData, iSize, Ringbuffer, iFrameWidth, mode, ddrTop);
}
SceInt32 passthrough_sceMpegRingbufferPut(SceMpegRingbuffer* Ringbuffer, SceInt32 iNumPackets, SceInt32 iAvailable)
{
    return sceMpegRingbufferPut(Ringbuffer, iNumPackets, iAvailable);
}

#pragma GCC pop_options
#pragma endregion

#pragma region asm helpers
int isJ(uint32_t inst)
{
    return (inst & 0xFC000000) == 0x08000000;
}
int isJal(uint32_t inst)
{
    return (inst & 0xFC000000) == 0x0C000000;
}
uint32_t jalDestination(uint32_t inst)
{
    return (inst & 0x03FFFFFF) << 2;
}
#pragma endregion

#pragma region hook helpers

const uint32_t jr_ra = 0x03E00008;

void hook(char* method, HookRecord* record, uintptr_t passthroughMethod, uintptr_t ourMethod)
{
    sceKernelPrintf("Searching our call for %s\n", method);

    //Usually on PSP the destination is "J method", rarely is it "jr $ra; syscall method".
    //On PPSSPP it is always(?) "jr $ra; syscall method", so we need to handle both


    //Find our call to the method
    for (uintptr_t addr = passthroughMethod; addr < passthroughMethod + 0x100; addr += 4)
    {
        uint32_t inst = *(uint32_t*)(addr);
        if (isJal(inst))
        {
            uint32_t dest = jalDestination(inst);
            record->inst_at_dest = *(uintptr_t*)dest;
            if (record->inst_at_dest == jr_ra)
            {
                sceKernelPrintf("Our call to %s is jr $ra; syscall\n", method);
                record->inst_at_dest = *(uintptr_t*)(dest + 4);
            }
            sceKernelPrintf("Our call to %s is offset 0x%X  instruction 0x%X  dest 0x%X  dest_inst 0x%X\n", method, addr - passthroughMethod, (unsigned int)inst, (unsigned int)dest, (unsigned int)record->inst_at_dest);
            record->inst_call = inst;
            break;
        }
    }

    if (!record->inst_at_dest)
    {
        sceKernelPrintf("Failed to find call to %s\n", method);
        return;
    }

    sceKernelPrintf("Hooking %s\n", method);
    //Find the call to the method
    for (uintptr_t addr = injector.base_addr; addr < injector.base_addr + injector.base_size; addr += 4)
    {
        uint32_t inst = injector.ReadMemory32(addr);
        if (isJal(inst))
        {
            //Jump goes within its memory
            uint32_t dest = jalDestination(inst);
            if (dest >= injector.base_addr && dest < injector.base_addr + injector.base_size)
            {
                //Destination matches us, or destination is "jr ra" followed by the instruction
                uint32_t dest_inst = *(uintptr_t*)dest; 
                if (dest_inst ==  record->inst_at_dest || (dest_inst == jr_ra && (*(uintptr_t*)(dest + 4) == record->inst_at_dest)))
                {
                    record->addr_call = addr;
                    sceKernelPrintf("Found call to %s at offset 0x%X  instruction 0x%X\n", method, addr - injector.base_addr, (unsigned int)inst);
                    injector.MakeJAL(addr, ourMethod);
                    break;
            }
            }
        }
    }
}
#pragma endregion

int module_start(SceSize args, void *argp)
{
    sceKernelPrintf("hello world\n");

    if (!FindModulesAndConfigureInjector())
    {
        sceKernelPrintf("Failed to find modules, injector not configured\n");
        return 0;
    }

    hook("sceMpegCreate", &hookMpegCreate, (uintptr_t)passthrough_sceMpegCreate, (uintptr_t)fake_sceMpegCreate);
    hook("sceMpegRingbufferPut", &hookMpegRingbufferPut, (uintptr_t)passthrough_sceMpegRingbufferPut, (uintptr_t)fake_sceMpegRingbufferPut);
    
    // sceKernelDcacheWritebackAll();
    // kuKernelIcacheInvalidateAll();

    return 0;
}

int module_stop(SceSize args, void *argp)
{
    //Undo hooks
    if (hookMpegCreate.addr_call)
        injector.WriteMemory32(hookMpegCreate.addr_call, hookMpegCreate.inst_call);
    if (hookMpegRingbufferPut.addr_call)
        injector.WriteMemory32(hookMpegRingbufferPut.addr_call, hookMpegRingbufferPut.inst_call);

    // sceKernelDcacheWritebackAll();
    // kuKernelIcacheInvalidateAll();

    return 0;
}
