#include <pspuser.h>
#include "injector_setup.h"
#include "module_config.h"

int module_start(SceSize args, void *argp)
{
    sceKernelPrintf("hello world\n");

    if (!FindModulesAndConfigureInjector())
    {
        sceKernelPrintf("Failed to find modules, injector not configured\n");
    }

    return 0;
}

int module_stop(SceSize args, void *argp)
{
    return 0;
}