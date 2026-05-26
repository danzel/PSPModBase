#include <pspsdk.h>
#include <kubridge.h>
#include "module_config.h"
#include "../../includes/psp/injector.h"

//
// CheckModules
// Executes only once on startup
// This works both on a real PSP and PPSSPP, but we limit this to PPSSPP because kuKernelFindModuleByName is better
//
static int CheckModulesPPSSPP()
{
    SceUID modules[10];
    int count = 0;
    int bFoundMainModule = 0;
    int bFoundInternalModule = 0;
    if (sceKernelGetModuleIdList(modules, sizeof(modules), &count) >= 0)
    {
        int i;
        SceKernelModuleInfo info;
        for (i = 0; i < count; ++i)
        {
            info.size = sizeof(SceKernelModuleInfo);
            if (sceKernelQueryModuleInfo(modules[i], &info) < 0)
            {
                continue;
            }
            if (strcmp(info.name, MODULE_NAME_INTERNAL) == 0)
            {
#ifdef LOG
                logPrintf("Found module " MODULE_NAME_INTERNAL);
                logPrintf("text_addr: 0x%X\ntext_size: 0x%X", info.text_addr, info.text_size);
#endif
                injector.SetGameBaseAddress(info.text_addr, info.text_size);

                bFoundMainModule = 1;
            }
            else if (strcmp(info.name, MODULE_NAME) == 0)
            {
#ifdef LOG
                logPrintf("PRX module " MODULE_NAME);
                logPrintf("text_addr: 0x%X\ntext_size: 0x%X", info.text_addr, info.text_size);
#endif
                injector.SetModuleBaseAddress(info.text_addr, info.text_size);

                bFoundInternalModule = 1;
            }
        }
    }

    if (bFoundInternalModule)
    {
        if (bFoundMainModule)
        {
            return 1;
        }
    }

    // Since we can't use OnModuleStart like on a PSP CFW, we have to scan for modules again
    // if we want to intercept another one. Read the note at the bottom of OnModuleStart for more info.

    return 0;
}
//
// CheckModulesPSP
// Executes only once on startup
// Works only on PSP CFW
//
int CheckModulesPSP()
{
    SceModule mod = {0};
    int kuErrCode = kuKernelFindModuleByName(MODULE_NAME_INTERNAL, &mod);
    if (kuErrCode != 0)
        return 0;

    SceModule this_module = {0};
    kuErrCode = kuKernelFindModuleByName(MODULE_NAME, &this_module);
    if (kuErrCode != 0)
        return 0;

    injector.SetGameBaseAddress(mod.text_addr, mod.text_size);
    injector.SetModuleBaseAddress(this_module.text_addr, this_module.text_size);

    return 1;
}

int FindModulesAndConfigureInjector()
{
    // If a kemulator interface exists, we know that we're in an emulator
    if (sceIoDevctl("kemulator:", 0x00000003, NULL, 0, NULL, 0) == 0)
    {
        return CheckModulesPPSSPP();
    }
    else
    {
        return CheckModulesPSP();
    }
}