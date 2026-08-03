#ifndef HUBOS_RUNTIME_CONFIG_H
#define HUBOS_RUNTIME_CONFIG_H

#include "hubos/app_vm_runtime.h"

const hubos_app_vm_runtime_profile_t *hubos_runtime_config_profiles(size_t *out_count);
const hubos_app_vm_runtime_selection_t *hubos_runtime_config_default_selection(void);
const hubos_app_vm_runtime_profile_t *hubos_runtime_config_default_profile(void);

#endif
