#ifndef HUBOS_BUS_MANAGER_H
#define HUBOS_BUS_MANAGER_H

#include "hubos/audit.h"
#include "hubos/resource_registry.h"

typedef enum {
  HUBOS_BUS_PCIE = 0,
  HUBOS_BUS_USB,
  HUBOS_BUS_I2C,
  HUBOS_BUS_SPI,
} hubos_bus_kind_t;

typedef struct hubos_bus_manager hubos_bus_manager_t;

typedef struct {
  bool (*discover)(void *context,
                   hubos_bus_manager_t *manager,
                   const char *resource_name,
                   size_t resource_name_len,
                   hubos_resource_state_t state);
} hubos_bus_manager_ops_t;

struct hubos_bus_manager {
  hubos_bus_kind_t kind;
  const char *name;
  hubos_resource_registry_t *registry;
  hubos_audit_log_t *audit_log;
  const hubos_bus_manager_ops_t *ops;
  void *ops_context;
};

void hubos_bus_manager_init(hubos_bus_manager_t *manager,
                            hubos_bus_kind_t kind,
                            const char *name,
                            hubos_resource_registry_t *registry,
                            hubos_audit_log_t *audit_log);

void hubos_bus_manager_set_ops(hubos_bus_manager_t *manager,
                               const hubos_bus_manager_ops_t *ops,
                               void *ops_context);

bool hubos_bus_manager_discover(hubos_bus_manager_t *manager,
                                const char *resource_name,
                                size_t resource_name_len,
                                hubos_resource_state_t state,
                                hubos_id_t *out_resource_id);

#endif
