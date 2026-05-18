#include <pspuser.h>
#include "module_config.h"

PSP_MODULE_INFO(MODULE_NAME, 0, 1, 0);
PSP_DISABLE_NEWLIB();

// From https://github.com/uofw/uofw/blob/master/include/common/module.h#L69
/*
 * Entry thread structure - an entry thread is used for executing the
 * module entry functions.
 */
typedef struct
{
    /* The number of entry thread parameters, typically 3. */
    u32 numParams;
    /* The initial priority of the entry thread. */
    u32 initPriority;
    /* The stack size of the entry thread. */
    u32 stackSize;
    /* The attributes of the entry thread. */
    u32 attr;
} SceModuleEntryThread;

const SceModuleEntryThread module_start_thread_parameter = {3, 0x20, 0x400, 0};
const SceModuleEntryThread module_stop_thread_parameter = {3, 0x20, 0x400, 0};
