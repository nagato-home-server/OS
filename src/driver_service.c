#include "hubos/driver_service.h"

#include <stdlib.h>
#include <string.h>

static bool hubos_driver_service_reserve_bindings(hubos_driver_service_t *service,
                                                  size_t desired_capacity) {
  if (service == NULL) {
    return false;
  }

  if (service->capacity >= desired_capacity) {
    return true;
  }

  size_t new_capacity = service->capacity == 0 ? 4 : service->capacity;
  while (new_capacity < desired_capacity) {
    new_capacity *= 2;
  }

  void *new_items = realloc(service->items, new_capacity * sizeof(*service->items));
  if (new_items == NULL) {
    return false;
  }

  service->items = new_items;
  service->capacity = new_capacity;
  return true;
}

static hubos_driver_binding_t *hubos_driver_service_find_mutable(hubos_driver_service_t *service,
                                                                 hubos_id_t resource_id) {
  if (service == NULL || resource_id == HUBOS_ID_INVALID) {
    return NULL;
  }

  for (size_t index = 0; index < service->count; ++index) {
    if (service->items[index].resource_id == resource_id) {
      return &service->items[index];
    }
  }

  return NULL;
}

static const hubos_driver_binding_t *hubos_driver_service_find(const hubos_driver_service_t *service,
                                                               hubos_id_t resource_id) {
  if (service == NULL || resource_id == HUBOS_ID_INVALID) {
    return NULL;
  }

  for (size_t index = 0; index < service->count; ++index) {
    if (service->items[index].resource_id == resource_id) {
      return &service->items[index];
    }
  }

  return NULL;
}

static bool hubos_driver_service_validate_package(const hubos_driver_service_t *service,
                                                  hubos_id_t driver_id,
                                                  const hubos_driver_package_t *package) {
  const hubos_driver_record_t *record = NULL;

  if (service == NULL || service->registry == NULL || service->loader == NULL || package == NULL) {
    return false;
  }

  record = hubos_driver_registry_get(service->registry, driver_id);
  if (record == NULL) {
    return false;
  }

  if (!hubos_driver_loader_validate_package(service->loader, package)) {
    return false;
  }

  return strcmp(record->version, package->version) == 0;
}

static bool hubos_driver_service_bind_record(hubos_driver_service_t *service,
                                             hubos_id_t resource_id,
                                             hubos_id_t driver_id,
                                             hubos_driver_slot_state_t state) {
  hubos_driver_binding_t *binding = NULL;

  if (service == NULL || resource_id == HUBOS_ID_INVALID || driver_id == HUBOS_ID_INVALID) {
    return false;
  }

  binding = hubos_driver_service_find_mutable(service, resource_id);
  if (binding == NULL) {
    if (!hubos_driver_service_reserve_bindings(service, service->count + 1)) {
      return false;
    }
    binding = &service->items[service->count++];
    binding->resource_id = resource_id;
  } else if (binding->state == HUBOS_DRIVER_SLOT_QUARANTINED ||
             (binding->state != HUBOS_DRIVER_SLOT_UNBOUND &&
              binding->driver_id != driver_id)) {
    return false;
  }

  binding->driver_id = driver_id;
  binding->state = state;
  return true;
}

void hubos_driver_service_init(hubos_driver_service_t *service,
                               const hubos_driver_registry_t *registry,
                               const hubos_driver_loader_t *loader,
                               hubos_audit_log_t *audit_log) {
  if (service == NULL) {
    return;
  }

  service->items = NULL;
  service->count = 0;
  service->capacity = 0;
  service->registry = registry;
  service->loader = loader;
  service->audit_log = audit_log;
}

void hubos_driver_service_destroy(hubos_driver_service_t *service) {
  if (service == NULL) {
    return;
  }

  free(service->items);
  service->items = NULL;
  service->count = 0;
  service->capacity = 0;
  service->registry = NULL;
  service->loader = NULL;
  service->audit_log = NULL;
}

bool hubos_driver_service_bind(hubos_driver_service_t *service,
                               hubos_id_t resource_id,
                               hubos_id_t driver_id,
                               const hubos_driver_package_t *package) {
  if (!hubos_driver_service_validate_package(service, driver_id, package)) {
    if (service != NULL && service->audit_log != NULL) {
      (void)hubos_audit_log_record(service->audit_log,
                                   HUBOS_AUDIT_DRIVER_QUARANTINED,
                                   resource_id,
                                   driver_id,
                                   0,
                                   0);
    }
    return false;
  }

  if (!hubos_driver_service_bind_record(service, resource_id, driver_id, HUBOS_DRIVER_SLOT_BOUND)) {
    return false;
  }

  if (service->audit_log != NULL) {
    (void)hubos_audit_log_record(service->audit_log,
                                 HUBOS_AUDIT_DRIVER_BOUND,
                                 resource_id,
                                 driver_id,
                                 0,
                                 0);
  }
  return true;
}

bool hubos_driver_service_prepare_rebind(hubos_driver_service_t *service,
                                         hubos_id_t resource_id,
                                         hubos_id_t driver_id,
                                         const hubos_driver_package_t *package) {
  hubos_driver_binding_t *binding = NULL;

  if (service == NULL) {
    return false;
  }

  binding = hubos_driver_service_find_mutable(service, resource_id);
  if (binding == NULL || binding->state == HUBOS_DRIVER_SLOT_QUARANTINED) {
    return false;
  }

  if (!hubos_driver_service_validate_package(service, driver_id, package)) {
    binding->state = HUBOS_DRIVER_SLOT_QUARANTINED;
    if (service->audit_log != NULL) {
      (void)hubos_audit_log_record(service->audit_log,
                                   HUBOS_AUDIT_DRIVER_QUARANTINED,
                                   resource_id,
                                   driver_id,
                                   0,
                                   0);
    }
    return false;
  }

  binding->state = HUBOS_DRIVER_SLOT_REBINDING;
  if (service->audit_log != NULL) {
    (void)hubos_audit_log_record(service->audit_log,
                                 HUBOS_AUDIT_DRIVER_REBIND_PREPARED,
                                 resource_id,
                                 driver_id,
                                 0,
                                 0);
  }
  return true;
}

bool hubos_driver_service_complete_rebind(hubos_driver_service_t *service,
                                          hubos_id_t resource_id,
                                          hubos_id_t driver_id,
                                          const hubos_driver_package_t *package) {
  hubos_driver_binding_t *binding = NULL;

  if (service == NULL) {
    return false;
  }

  binding = hubos_driver_service_find_mutable(service, resource_id);
  if (binding == NULL || binding->state != HUBOS_DRIVER_SLOT_REBINDING) {
    return false;
  }

  if (!hubos_driver_service_validate_package(service, driver_id, package) ||
      !hubos_driver_service_bind_record(service, resource_id, driver_id, HUBOS_DRIVER_SLOT_BOUND)) {
    binding->state = HUBOS_DRIVER_SLOT_QUARANTINED;
    if (service->audit_log != NULL) {
      (void)hubos_audit_log_record(service->audit_log,
                                   HUBOS_AUDIT_DRIVER_QUARANTINED,
                                   resource_id,
                                   driver_id,
                                   0,
                                   0);
    }
    return false;
  }

  if (service->audit_log != NULL) {
    (void)hubos_audit_log_record(service->audit_log,
                                 HUBOS_AUDIT_DRIVER_REBOUND,
                                 resource_id,
                                 driver_id,
                                 0,
                                 0);
  }
  return true;
}

bool hubos_driver_service_rebind(hubos_driver_service_t *service,
                                 hubos_id_t resource_id,
                                 hubos_id_t driver_id,
                                 const hubos_driver_package_t *package) {
  return hubos_driver_service_prepare_rebind(service, resource_id, driver_id, package) &&
         hubos_driver_service_complete_rebind(service, resource_id, driver_id, package);
}

bool hubos_driver_service_quarantine(hubos_driver_service_t *service, hubos_id_t resource_id) {
  hubos_driver_binding_t *binding = hubos_driver_service_find_mutable(service, resource_id);

  if (binding == NULL) {
    return false;
  }

  binding->state = HUBOS_DRIVER_SLOT_QUARANTINED;

  if (service != NULL && service->audit_log != NULL) {
    (void)hubos_audit_log_record(service->audit_log,
                                 HUBOS_AUDIT_DRIVER_QUARANTINED,
                                 resource_id,
                                 binding->driver_id,
                                 0,
                                 0);
  }

  return true;
}

bool hubos_driver_service_unbind(hubos_driver_service_t *service, hubos_id_t resource_id) {
  hubos_driver_binding_t *binding = hubos_driver_service_find_mutable(service, resource_id);

  if (binding == NULL) {
    return false;
  }

  binding->state = HUBOS_DRIVER_SLOT_UNBOUND;
  binding->driver_id = HUBOS_ID_INVALID;

  if (service != NULL && service->audit_log != NULL) {
    (void)hubos_audit_log_record(service->audit_log,
                                 HUBOS_AUDIT_DRIVER_UNBOUND,
                                 resource_id,
                                 0,
                                 0,
                                 0);
  }

  return true;
}

const hubos_driver_binding_t *hubos_driver_service_get(const hubos_driver_service_t *service,
                                                       hubos_id_t resource_id) {
  return hubos_driver_service_find(service, resource_id);
}
