#ifndef HUBOS_DRIVER_SERVICE_H
#define HUBOS_DRIVER_SERVICE_H

#include "hubos/audit.h"
#include "hubos/driver_loader.h"
#include "hubos/driver_registry.h"

typedef struct {
  hubos_driver_binding_t *items;
  size_t count;
  size_t capacity;
  const hubos_driver_registry_t *registry;
  const hubos_driver_loader_t *loader;
  hubos_audit_log_t *audit_log;
} hubos_driver_service_t;

void hubos_driver_service_init(hubos_driver_service_t *service,
                               const hubos_driver_registry_t *registry,
                               const hubos_driver_loader_t *loader,
                               hubos_audit_log_t *audit_log);

void hubos_driver_service_destroy(hubos_driver_service_t *service);

bool hubos_driver_service_bind(hubos_driver_service_t *service,
                               hubos_id_t resource_id,
                               hubos_id_t driver_id,
                               const hubos_driver_package_t *package);

bool hubos_driver_service_prepare_rebind(hubos_driver_service_t *service,
                                        hubos_id_t resource_id,
                                        hubos_id_t driver_id,
                                        const hubos_driver_package_t *package);

bool hubos_driver_service_complete_rebind(hubos_driver_service_t *service,
                                          hubos_id_t resource_id,
                                          hubos_id_t driver_id,
                                          const hubos_driver_package_t *package);

bool hubos_driver_service_rebind(hubos_driver_service_t *service,
                                 hubos_id_t resource_id,
                                 hubos_id_t driver_id,
                                 const hubos_driver_package_t *package);

bool hubos_driver_service_quarantine(hubos_driver_service_t *service, hubos_id_t resource_id);

bool hubos_driver_service_unbind(hubos_driver_service_t *service, hubos_id_t resource_id);

const hubos_driver_binding_t *hubos_driver_service_get(const hubos_driver_service_t *service,
                                                       hubos_id_t resource_id);

#endif
