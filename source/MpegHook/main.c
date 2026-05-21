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

//https://pspdev.github.io/pspsdk/pspmpeg_8h.html

void dumpRingbuffer(SceMpegRingbuffer* ringbuffer, char* prefix)
{
    sceKernelPrintf("Ringbuffer %s\nPackets: %ld  pRead: %ld  pWritePos: %ld  pAvail: %ld  pSize: %ld\ndata: %p  dataUpperBound: %p\n", prefix, ringbuffer->iPackets, ringbuffer->iUnk0, ringbuffer->iUnk1, ringbuffer->iUnk2, ringbuffer->iUnk3, ringbuffer->pData, (ScePVoid)ringbuffer->iUnk4);
    //There are other fields
}


SceInt32 fake_sceMpegCreate(SceMpeg *Mpeg, ScePVoid pData, SceInt32 iSize, SceMpegRingbuffer *Ringbuffer, SceInt32 iFrameWidth, SceInt32 mode, SceInt32 ddrTop)
{
    sceKernelPrintf("sceMpegCreate called with Mpeg=0x%p, pData=0x%p, iSize=%li, Ringbuffer=0x%p, iFrameWidth=%li, mode=%li, ddrTop=%li\n", Mpeg, pData, iSize, Ringbuffer, iFrameWidth, mode, ddrTop);
    dumpRingbuffer(Ringbuffer, "before");

    SceInt32 result = sceMpegCreate(Mpeg, pData, iSize, Ringbuffer, iFrameWidth, mode, ddrTop);

    sceKernelPrintf("sceMpegCreate returned %ld\n", result);
    dumpRingbuffer(Ringbuffer, "after");

    return result;
}
SceInt32 fake_sceMpegRingbufferPut(SceMpegRingbuffer* Ringbuffer, SceInt32 iNumPackets, SceInt32 iAvailable)
{
    sceKernelPrintf("sceMpegRingbufferPut called with Ringbuffer=0x%p, iNumPackets=%li, iAvailable=%li\n", Ringbuffer, iNumPackets, iAvailable);
    dumpRingbuffer(Ringbuffer, "before");

    SceInt32 result = sceMpegRingbufferPut(Ringbuffer, iNumPackets, iAvailable);

    sceKernelPrintf("sceMpegRingbufferPut returned %ld\n", result);
    dumpRingbuffer(Ringbuffer, "after");

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
void hook(char* method, HookRecord* record, uintptr_t passthroughMethod, uintptr_t ourMethod)
{
    //Find out call to the method
    for (uintptr_t addr = passthroughMethod; addr < passthroughMethod + 0x100; addr += 4)
    {
        uint32_t inst = *(uint32_t*)(addr);
        if (isJal(inst))
        {
            uint32_t dest = jalDestination(inst);
            sceKernelPrintf("Our call to %s is 0x%X [0x%lX] -> 0x%lX\n", method, addr, inst, dest);
            record->inst_at_dest = *(uintptr_t*)dest;
            record->inst_call = inst;
            break;
        }
    }

    //Find the call to the method
    for (uintptr_t addr = injector.base_addr; addr < injector.base_addr + injector.base_size; addr += 4)
    {
        uint32_t inst = injector.ReadMemory32(addr);
        if (isJal(inst))
        {
            uint32_t dest = jalDestination(inst);
            if (dest >= injector.base_addr && *(uintptr_t*)jalDestination(inst) == record->inst_at_dest)
            {
                record->addr_call = addr;
                sceKernelPrintf("Found call to %s at 0x%X [0x%lX]\n", method, addr, inst);
                injector.MakeJAL(addr, ourMethod);
                break;
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
    }

    hook("sceMpegCreate", &hookMpegCreate, (uintptr_t)passthrough_sceMpegCreate, (uintptr_t)fake_sceMpegCreate);
    hook("sceMpegRingbufferPut", &hookMpegRingbufferPut, (uintptr_t)passthrough_sceMpegRingbufferPut, (uintptr_t)fake_sceMpegRingbufferPut);
    
    sceKernelDcacheWritebackAll();
    kuKernelIcacheInvalidateAll();

    return 0;
}

int module_stop(SceSize args, void *argp)
{
    //Undo hooks
    if (hookMpegCreate.addr_call)
        injector.WriteMemory32(hookMpegCreate.addr_call, hookMpegCreate.inst_call);
    if (hookMpegRingbufferPut.addr_call)
        injector.WriteMemory32(hookMpegRingbufferPut.addr_call, hookMpegRingbufferPut.inst_call);

    sceKernelDcacheWritebackAll();
    kuKernelIcacheInvalidateAll();

    return 0;
}
