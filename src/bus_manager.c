#include "hubos/bus_manager.h"

void hubos_bus_manager_init(hubos_bus_manager_t *manager,
                            hubos_bus_kind_t kind,
                            const char *name,
                            hubos_resource_registry_t *registry,
                            hubos_audit_log_t *audit_log) {
  if (manager == NULL) {
    return;
  }

  manager->kind = kind;
  manager->name = name;
  manager->registry = registry;
  manager->audit_log = audit_log;
  manager->ops = NULL;
  manager->ops_context = NULL;
}

void hubos_bus_manager_set_ops(hubos_bus_manager_t *manager,
                               const hubos_bus_manager_ops_t *ops,
                               void *ops_context) {
  if (manager == NULL) {
    return;
  }

  manager->ops = ops;
  manager->ops_context = ops_context;
}

bool hubos_bus_manager_discover(hubos_bus_manager_t *manager,
                                const char *resource_name,
                                size_t resource_name_len,
                                hubos_resource_state_t state,
                                hubos_id_t *out_resource_id) {
  bool ok = false;

  if (manager == NULL || manager->registry == NULL) {
    return false;
  }

  if (manager->ops != NULL && manager->ops->discover != NULL &&
      !manager->ops->discover(manager->ops_context,
                              manager,
                              resource_name,
                              resource_name_len,
                              state)) {
    return false;
  }

  ok = hubos_resource_registry_discover(manager->registry,
                                        resource_name,
                                        resource_name_len,
                                        state,
                                        out_resource_id,
                                        NULL);
  if (ok && manager->audit_log != NULL) {
    (void)hubos_audit_log_record(manager->audit_log,
                                 HUBOS_AUDIT_RESOURCE_DISCOVERED,
                                 0,
                                 out_resource_id != NULL ? *out_resource_id : HUBOS_ID_INVALID,
                                 0,
                                 (unsigned)manager->kind);
  }

  return ok;
}
