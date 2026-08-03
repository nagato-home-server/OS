#ifndef HUBOS_LINUX_USBIO_BACKEND_H
#define HUBOS_LINUX_USBIO_BACKEND_H

#include <stdbool.h>

#include "hubos/system.h"

bool hubos_linux_usbio_backend_is_requested(void);
bool hubos_system_enable_linux_usbio_backend(hubos_system_t *system);

#endif
