#include <pspuser.h>
#include <pspmpeg.h>
#include <kubridge.h>
#include "injector_setup.h"
#include "../../includes/psp/injector.h"
#include "module_config.h"

uintptr_t addr_call_sceMpegCreate = 0;
uintptr_t inst_call_sceMpegCreate = 0x0E2DFD10;

//https://pspdev.github.io/pspsdk/pspmpeg_8h.html
int fake_sceMpegCreate (SceMpeg *Mpeg, ScePVoid pData, SceInt32 iSize, SceMpegRingbuffer *Ringbuffer, SceInt32 iFrameWidth, SceInt32 mode, SceInt32 ddrTop)
{
    sceKernelPrintf("sceMpegCreate called with Mpeg=0x%p, pData=0x%p, iSize=%li, Ringbuffer=0x%p, iFrameWidth=%li, mode=%li, ddrTop=%li\n", Mpeg, pData, iSize, Ringbuffer, iFrameWidth, mode, ddrTop);

    int result = sceMpegCreate(Mpeg, pData, iSize, Ringbuffer, iFrameWidth, mode, ddrTop);

    sceKernelPrintf("sceMpegCreate returned %d, Mpeg=0x%p Ringbuffer=0x%p\n", result, Mpeg, Ringbuffer);

    return result;
}

int module_start(SceSize args, void *argp)
{
    sceKernelPrintf("hello world\n");

    if (!FindModulesAndConfigureInjector())
    {
        sceKernelPrintf("Failed to find modules, injector not configured\n");
    }

    //Find the call to sceMpegCreate
    //TODO: This finds another address (0x880404C) correct is 0x89EE1FCmodst, I guess nid resolution doesn't find 'sceMpegCreate' pointer this way
    //TODO: PSPLink can hook things, check its code
    //injector.MakeJAL(inst_call_sceMpegCreate, (uintptr_t)sceMpegCreate); 
    for (uintptr_t addr = injector.base_addr; addr < injector.base_addr + injector.base_size; addr += 4)
    {
        if (injector.ReadMemory32(addr) == inst_call_sceMpegCreate)
        {
            addr_call_sceMpegCreate = addr;
            sceKernelPrintf("Found call to sceMpegCreate at 0x%X\n", addr);
            injector.MakeJAL(addr, (uintptr_t)fake_sceMpegCreate);
            break;
        }
    }
    
    sceKernelDcacheWritebackAll();
    kuKernelIcacheInvalidateAll();

    return 0;
}

int module_stop(SceSize args, void *argp)
{
    //Undo patches
    if (addr_call_sceMpegCreate)
        injector.WriteMemory32(addr_call_sceMpegCreate, inst_call_sceMpegCreate);

    sceKernelDcacheWritebackAll();
    kuKernelIcacheInvalidateAll();

    return 0;
}