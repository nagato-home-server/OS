#include "hubos/service_endpoints.h"

void hubos_resource_registry_endpoint_init(hubos_resource_registry_endpoint_t *endpoint,
                                           hubos_resource_registry_t *registry,
                                           hubos_audit_log_t *audit_log) {
  if (endpoint == NULL) {
    return;
  }

  endpoint->registry = registry;
  endpoint->audit_log = audit_log;
}

bool hubos_resource_registry_endpoint_register(hubos_resource_registry_endpoint_t *endpoint,
                                               const char *name,
                                               size_t name_len,
                                               hubos_resource_state_t state,
                                               hubos_id_t *out_resource_id,
                                               bool *out_is_new) {
  bool ok = false;

  if (endpoint == NULL || endpoint->registry == NULL) {
    return false;
  }

  ok = hubos_resource_registry_register(endpoint->registry,
                                        name,
                                        name_len,
                                        state,
                                        out_resource_id,
                                        out_is_new);
  if (ok && endpoint->audit_log != NULL) {
    (void)hubos_audit_log_record(endpoint->audit_log,
                                 HUBOS_AUDIT_RESOURCE_REGISTERED,
                                 0,
                                 out_resource_id != NULL ? *out_resource_id : HUBOS_ID_INVALID,
                                 0,
                                 (unsigned)state);
  }
  return ok;
}

bool hubos_resource_registry_endpoint_update_state(hubos_resource_registry_endpoint_t *endpoint,
                                                   hubos_id_t resource_id,
                                                   hubos_resource_state_t state) {
  bool ok = false;

  if (endpoint == NULL || endpoint->registry == NULL) {
    return false;
  }

  ok = hubos_resource_registry_update_state(endpoint->registry, resource_id, state);
  if (ok && endpoint->audit_log != NULL) {
    (void)hubos_audit_log_record(endpoint->audit_log,
                                 HUBOS_AUDIT_RESOURCE_STATE_CHANGED,
                                 0,
                                 resource_id,
                                 0,
                                 (unsigned)state);
  }
  return ok;
}

bool hubos_resource_registry_endpoint_quarantine(hubos_resource_registry_endpoint_t *endpoint,
                                                 hubos_id_t resource_id) {
  bool ok = false;

  if (endpoint == NULL || endpoint->registry == NULL) {
    return false;
  }

  ok = hubos_resource_registry_quarantine(endpoint->registry, resource_id);
  if (ok && endpoint->audit_log != NULL) {
    (void)hubos_audit_log_record(endpoint->audit_log,
                                 HUBOS_AUDIT_RESOURCE_STATE_CHANGED,
                                 0,
                                 resource_id,
                                 0,
                                 (unsigned)HUBOS_RESOURCE_QUARANTINED);
  }
  return ok;
}

bool hubos_resource_registry_endpoint_retire(hubos_resource_registry_endpoint_t *endpoint,
                                             hubos_id_t resource_id) {
  bool ok = false;

  if (endpoint == NULL || endpoint->registry == NULL) {
    return false;
  }

  ok = hubos_resource_registry_retire(endpoint->registry, resource_id);
  if (ok && endpoint->audit_log != NULL) {
    (void)hubos_audit_log_record(endpoint->audit_log,
                                 HUBOS_AUDIT_RESOURCE_STATE_CHANGED,
                                 0,
                                 resource_id,
                                 0,
                                 (unsigned)HUBOS_RESOURCE_RETIRED);
  }
  return ok;
}

const hubos_resource_t *hubos_resource_registry_endpoint_get(
  const hubos_resource_registry_endpoint_t *endpoint,
  hubos_id_t resource_id) {
  if (endpoint == NULL || endpoint->registry == NULL) {
    return NULL;
  }

  return hubos_resource_registry_get(endpoint->registry, resource_id);
}

void hubos_capability_manager_endpoint_init(hubos_capability_manager_endpoint_t *endpoint,
                                            hubos_capability_manager_t *manager,
                                            hubos_audit_log_t *audit_log) {
  if (endpoint == NULL) {
    return;
  }

  endpoint->manager = manager;
  endpoint->audit_log = audit_log;
}

bool hubos_capability_manager_endpoint_issue(hubos_capability_manager_endpoint_t *endpoint,
                                             hubos_id_t owner_session_id,
                                             hubos_id_t resource_id,
                                             unsigned rights,
                                             bool delegatable,
                                             hubos_id_t *out_capability_id) {
  bool ok = false;

  if (endpoint == NULL || endpoint->manager == NULL) {
    return false;
  }

  ok = hubos_capability_manager_issue(endpoint->manager,
                                      owner_session_id,
                                      resource_id,
                                      rights,
                                      delegatable,
                                      out_capability_id);
  if (ok && endpoint->audit_log != NULL) {
    (void)hubos_audit_log_record(endpoint->audit_log,
                                 HUBOS_AUDIT_CAPABILITY_ISSUED,
                                 owner_session_id,
                                 resource_id,
                                 out_capability_id != NULL ? *out_capability_id : HUBOS_ID_INVALID,
                                 rights);
  }
  return ok;
}

bool hubos_capability_manager_endpoint_copy(hubos_capability_manager_endpoint_t *endpoint,
                                            hubos_id_t source_capability_id,
                                            hubos_id_t owner_session_id,
                                            hubos_id_t *out_capability_id) {
  bool ok = false;

  if (endpoint == NULL || endpoint->manager == NULL) {
    return false;
  }

  ok = hubos_capability_manager_copy(endpoint->manager,
                                     source_capability_id,
                                     owner_session_id,
                                     out_capability_id);
  if (ok && endpoint->audit_log != NULL) {
    (void)hubos_audit_log_record(endpoint->audit_log,
                                 HUBOS_AUDIT_CAPABILITY_MINTED,
                                 owner_session_id,
                                 HUBOS_ID_INVALID,
                                 out_capability_id != NULL ? *out_capability_id : HUBOS_ID_INVALID,
                                 0);
  }
  return ok;
}

bool hubos_capability_manager_endpoint_mint_from(hubos_capability_manager_endpoint_t *endpoint,
                                                 hubos_id_t source_capability_id,
                                                 hubos_id_t owner_session_id,
                                                 unsigned rights,
                                                 bool delegatable,
                                                 hubos_id_t *out_capability_id) {
  bool ok = false;

  if (endpoint == NULL || endpoint->manager == NULL) {
    return false;
  }

  ok = hubos_capability_manager_mint_from(endpoint->manager,
                                          source_capability_id,
                                          owner_session_id,
                                          rights,
                                          delegatable,
                                          out_capability_id);
  if (ok && endpoint->audit_log != NULL) {
    (void)hubos_audit_log_record(endpoint->audit_log,
                                 HUBOS_AUDIT_CAPABILITY_MINTED,
                                 owner_session_id,
                                 HUBOS_ID_INVALID,
                                 out_capability_id != NULL ? *out_capability_id : HUBOS_ID_INVALID,
                                 rights);
  }
  return ok;
}

bool hubos_capability_manager_endpoint_transfer(hubos_capability_manager_endpoint_t *endpoint,
                                                hubos_id_t capability_id,
                                                hubos_id_t new_owner_session_id) {
  bool ok = false;

  if (endpoint == NULL || endpoint->manager == NULL) {
    return false;
  }

  ok = hubos_capability_manager_transfer(endpoint->manager, capability_id, new_owner_session_id);
  if (ok && endpoint->audit_log != NULL) {
    (void)hubos_audit_log_record(endpoint->audit_log,
                                 HUBOS_AUDIT_CAPABILITY_TRANSFERRED,
                                 new_owner_session_id,
                                 HUBOS_ID_INVALID,
                                 capability_id,
                                 0);
  }
  return ok;
}

bool hubos_capability_manager_endpoint_revoke(hubos_capability_manager_endpoint_t *endpoint,
                                              hubos_id_t capability_id) {
  bool ok = false;

  if (endpoint == NULL || endpoint->manager == NULL) {
    return false;
  }

  ok = hubos_capability_manager_revoke(endpoint->manager, capability_id);
  if (ok && endpoint->audit_log != NULL) {
    (void)hubos_audit_log_record(endpoint->audit_log,
                                 HUBOS_AUDIT_CAPABILITY_REVOKED,
                                 0,
                                 HUBOS_ID_INVALID,
                                 capability_id,
                                 0);
  }
  return ok;
}

size_t hubos_capability_manager_endpoint_revoke_owned(
  hubos_capability_manager_endpoint_t *endpoint,
  hubos_id_t owner_session_id) {
  if (endpoint == NULL || endpoint->manager == NULL) {
    return 0;
  }

  return hubos_capability_manager_revoke_owned(endpoint->manager, owner_session_id);
}

const hubos_capability_t *hubos_capability_manager_endpoint_get(
  const hubos_capability_manager_endpoint_t *endpoint,
  hubos_id_t capability_id) {
  if (endpoint == NULL || endpoint->manager == NULL) {
    return NULL;
  }

  return hubos_capability_manager_get(endpoint->manager, capability_id);
}

bool hubos_capability_manager_endpoint_authorize(
  const hubos_capability_manager_endpoint_t *endpoint,
  hubos_id_t capability_id,
  hubos_id_t resource_id,
  unsigned required_rights) {
  if (endpoint == NULL || endpoint->manager == NULL) {
    return false;
  }

  return hubos_capability_manager_authorize(endpoint->manager,
                                            capability_id,
                                            resource_id,
                                            required_rights);
}

void hubos_session_manager_endpoint_init(hubos_session_manager_endpoint_t *endpoint,
                                         hubos_session_manager_t *manager,
                                         hubos_capability_manager_t *capability_manager,
                                         hubos_audit_log_t *audit_log) {
  if (endpoint == NULL) {
    return;
  }

  endpoint->manager = manager;
  endpoint->capability_manager = capability_manager;
  endpoint->audit_log = audit_log;
}

bool hubos_session_manager_endpoint_create(hubos_session_manager_endpoint_t *endpoint,
                                           hubos_id_t owner_id,
                                           hubos_id_t parent_id,
                                           hubos_session_type_t type,
                                           hubos_id_t *out_session_id) {
  bool ok = false;

  if (endpoint == NULL || endpoint->manager == NULL) {
    return false;
  }

  ok = hubos_session_manager_create(endpoint->manager,
                                    owner_id,
                                    parent_id,
                                    type,
                                    out_session_id);
  if (ok && endpoint->audit_log != NULL) {
    (void)hubos_audit_log_record(endpoint->audit_log,
                                 HUBOS_AUDIT_SESSION_CREATED,
                                 owner_id,
                                 HUBOS_ID_INVALID,
                                 out_session_id != NULL ? *out_session_id : HUBOS_ID_INVALID,
                                 (unsigned)type);
  }
  return ok;
}

bool hubos_session_manager_endpoint_refresh_context(
  hubos_session_manager_endpoint_t *endpoint,
  hubos_id_t session_id,
  hubos_id_t namespace_view_version,
  hubos_id_t policy_context_version) {
  if (endpoint == NULL || endpoint->manager == NULL) {
    return false;
  }

  return hubos_session_manager_refresh_context(endpoint->manager,
                                               session_id,
                                               namespace_view_version,
                                               policy_context_version);
}

const hubos_session_t *hubos_session_manager_endpoint_get(
  const hubos_session_manager_endpoint_t *endpoint,
  hubos_id_t session_id) {
  if (endpoint == NULL || endpoint->manager == NULL) {
    return NULL;
  }

  return hubos_session_manager_get(endpoint->manager, session_id);
}

bool hubos_session_manager_endpoint_set_state(hubos_session_manager_endpoint_t *endpoint,
                                              hubos_id_t session_id,
                                              hubos_session_state_t state) {
  if (endpoint == NULL || endpoint->manager == NULL) {
    return false;
  }

  return hubos_session_manager_set_state(endpoint->manager, session_id, state);
}

bool hubos_session_manager_endpoint_is_ancestor(
  const hubos_session_manager_endpoint_t *endpoint,
  hubos_id_t ancestor_id,
  hubos_id_t session_id) {
  if (endpoint == NULL || endpoint->manager == NULL) {
    return false;
  }

  return hubos_session_manager_is_ancestor(endpoint->manager, ancestor_id, session_id);
}

size_t hubos_session_manager_endpoint_child_count(
  const hubos_session_manager_endpoint_t *endpoint,
  hubos_id_t session_id) {
  if (endpoint == NULL || endpoint->manager == NULL) {
    return 0;
  }

  return hubos_session_manager_child_count(endpoint->manager, session_id);
}

bool hubos_session_manager_endpoint_revoke_tree(hubos_session_manager_endpoint_t *endpoint,
                                                hubos_id_t session_id) {
  bool ok = false;

  if (endpoint == NULL || endpoint->manager == NULL) {
    return false;
  }

  ok = hubos_session_manager_revoke_tree(endpoint->manager,
                                         endpoint->capability_manager,
                                         session_id);
  if (ok && endpoint->audit_log != NULL) {
    (void)hubos_audit_log_record(endpoint->audit_log,
                                 HUBOS_AUDIT_SESSION_DESTROYED,
                                 session_id,
                                 HUBOS_ID_INVALID,
                                 0,
                                 0);
  }
  return ok;
}

void hubos_hub_endpoint_init(hubos_hub_endpoint_t *endpoint, hubos_hub_t *hub) {
  if (endpoint == NULL) {
    return;
  }

  endpoint->hub = hub;
}

bool hubos_hub_endpoint_resolve(const hubos_hub_endpoint_t *endpoint,
                                const char *name,
                                size_t name_len,
                                hubos_service_descriptor_t *out_descriptor) {
  if (endpoint == NULL || endpoint->hub == NULL) {
    return false;
  }

  return hubos_hub_resolve(endpoint->hub, name, name_len, out_descriptor);
}

void hubos_driver_service_endpoint_init(hubos_driver_service_endpoint_t *endpoint,
                                        hubos_driver_service_t *service) {
  if (endpoint == NULL) {
    return;
  }

  endpoint->service = service;
}

bool hubos_driver_service_endpoint_bind(hubos_driver_service_endpoint_t *endpoint,
                                        hubos_id_t resource_id,
                                        hubos_id_t driver_id,
                                        const hubos_driver_package_t *package) {
  if (endpoint == NULL || endpoint->service == NULL) {
    return false;
  }

  return hubos_driver_service_bind(endpoint->service, resource_id, driver_id, package);
}

bool hubos_driver_service_endpoint_prepare_rebind(
  hubos_driver_service_endpoint_t *endpoint,
  hubos_id_t resource_id,
  hubos_id_t driver_id,
  const hubos_driver_package_t *package) {
  if (endpoint == NULL || endpoint->service == NULL) {
    return false;
  }

  return hubos_driver_service_prepare_rebind(endpoint->service,
                                             resource_id,
                                             driver_id,
                                             package);
}

bool hubos_driver_service_endpoint_complete_rebind(
  hubos_driver_service_endpoint_t *endpoint,
  hubos_id_t resource_id,
  hubos_id_t driver_id,
  const hubos_driver_package_t *package) {
  if (endpoint == NULL || endpoint->service == NULL) {
    return false;
  }

  return hubos_driver_service_complete_rebind(endpoint->service,
                                              resource_id,
                                              driver_id,
                                              package);
}

bool hubos_driver_service_endpoint_rebind(hubos_driver_service_endpoint_t *endpoint,
                                          hubos_id_t resource_id,
                                          hubos_id_t driver_id,
                                          const hubos_driver_package_t *package) {
  if (endpoint == NULL || endpoint->service == NULL) {
    return false;
  }

  return hubos_driver_service_rebind(endpoint->service, resource_id, driver_id, package);
}

bool hubos_driver_service_endpoint_quarantine(hubos_driver_service_endpoint_t *endpoint,
                                              hubos_id_t resource_id) {
  if (endpoint == NULL || endpoint->service == NULL) {
    return false;
  }

  return hubos_driver_service_quarantine(endpoint->service, resource_id);
}

bool hubos_driver_service_endpoint_unbind(hubos_driver_service_endpoint_t *endpoint,
                                          hubos_id_t resource_id) {
  if (endpoint == NULL || endpoint->service == NULL) {
    return false;
  }

  return hubos_driver_service_unbind(endpoint->service, resource_id);
}

const hubos_driver_binding_t *hubos_driver_service_endpoint_get(
  const hubos_driver_service_endpoint_t *endpoint,
  hubos_id_t resource_id) {
  if (endpoint == NULL || endpoint->service == NULL) {
    return NULL;
  }

  return hubos_driver_service_get(endpoint->service, resource_id);
}

void hubos_network_server_endpoint_init(hubos_network_server_endpoint_t *endpoint,
                                       hubos_network_server_t *server) {
  if (endpoint == NULL) {
    return;
  }

  endpoint->server = server;
}

bool hubos_network_server_endpoint_bind_namespace(
  hubos_network_server_endpoint_t *endpoint,
  hubos_namespace_handle_t namespace_handle) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_network_server_bind_namespace(endpoint->server, namespace_handle);
}

bool hubos_network_server_endpoint_set_policy(hubos_network_server_endpoint_t *endpoint,
                                              bool routing_enabled,
                                              bool firewall_enabled) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_network_server_set_policy(endpoint->server, routing_enabled, firewall_enabled);
}

bool hubos_network_server_endpoint_add_route(hubos_network_server_endpoint_t *endpoint,
                                             const char *destination,
                                             hubos_id_t nic_resource_id,
                                             unsigned metric) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_network_server_add_route(endpoint->server, destination, nic_resource_id, metric);
}

bool hubos_network_server_endpoint_set_default_route(
  hubos_network_server_endpoint_t *endpoint,
  hubos_id_t nic_resource_id) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_network_server_set_default_route(endpoint->server, nic_resource_id);
}

bool hubos_network_server_endpoint_select_nic(hubos_network_server_endpoint_t *endpoint,
                                              const char *destination,
                                              size_t destination_len,
                                              hubos_id_t *out_nic_resource_id) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_network_server_select_nic(endpoint->server,
                                         destination,
                                         destination_len,
                                         out_nic_resource_id);
}

bool hubos_network_server_endpoint_bind_port(hubos_network_server_endpoint_t *endpoint,
                                            unsigned port,
                                            hubos_id_t nic_resource_id,
                                            hubos_id_t session_id) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_network_server_bind_port(endpoint->server, port, nic_resource_id, session_id);
}

bool hubos_network_server_endpoint_set_failover_policy(
  hubos_network_server_endpoint_t *endpoint,
  bool failover_enabled,
  hubos_id_t preferred_nic_resource_id) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_network_server_set_failover_policy(endpoint->server,
                                                  failover_enabled,
                                                  preferred_nic_resource_id);
}

bool hubos_network_server_endpoint_describe(const hubos_network_server_endpoint_t *endpoint,
                                            hubos_service_descriptor_t *out_descriptor) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_network_server_describe(endpoint->server, out_descriptor);
}

void hubos_storage_server_endpoint_init(hubos_storage_server_endpoint_t *endpoint,
                                       hubos_storage_server_t *server) {
  if (endpoint == NULL) {
    return;
  }

  endpoint->server = server;
}

bool hubos_storage_server_endpoint_bind_namespace(
  hubos_storage_server_endpoint_t *endpoint,
  hubos_namespace_handle_t namespace_handle) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_storage_server_bind_namespace(endpoint->server, namespace_handle);
}

bool hubos_storage_server_endpoint_release_namespace(
  hubos_storage_server_endpoint_t *endpoint) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_storage_server_release_namespace(endpoint->server);
}

bool hubos_storage_server_endpoint_finalize_namespace(
  hubos_storage_server_endpoint_t *endpoint) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_storage_server_finalize_namespace(endpoint->server);
}

bool hubos_storage_server_endpoint_describe(const hubos_storage_server_endpoint_t *endpoint,
                                            hubos_service_descriptor_t *out_descriptor) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_storage_server_describe(endpoint->server, out_descriptor);
}

void hubos_display_server_endpoint_init(hubos_display_server_endpoint_t *endpoint,
                                       hubos_display_server_t *server) {
  if (endpoint == NULL) {
    return;
  }

  endpoint->server = server;
}

bool hubos_display_server_endpoint_bind_namespace(
  hubos_display_server_endpoint_t *endpoint,
  hubos_namespace_handle_t namespace_handle) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_display_server_bind_namespace(endpoint->server, namespace_handle);
}

bool hubos_display_server_endpoint_release_namespace(
  hubos_display_server_endpoint_t *endpoint) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_display_server_release_namespace(endpoint->server);
}

bool hubos_display_server_endpoint_finalize_namespace(
  hubos_display_server_endpoint_t *endpoint) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_display_server_finalize_namespace(endpoint->server);
}

bool hubos_display_server_endpoint_describe(const hubos_display_server_endpoint_t *endpoint,
                                            hubos_service_descriptor_t *out_descriptor) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_display_server_describe(endpoint->server, out_descriptor);
}

void hubos_device_server_endpoint_init(hubos_device_server_endpoint_t *endpoint,
                                       hubos_device_server_t *server) {
  if (endpoint == NULL) {
    return;
  }

  endpoint->server = server;
}

bool hubos_device_server_endpoint_quarantine(hubos_device_server_endpoint_t *endpoint) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_device_server_quarantine(endpoint->server);
}

bool hubos_device_server_endpoint_clear_quarantine(hubos_device_server_endpoint_t *endpoint) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_device_server_clear_quarantine(endpoint->server);
}

bool hubos_device_server_endpoint_reset(hubos_device_server_endpoint_t *endpoint) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_device_server_reset(endpoint->server);
}

bool hubos_device_server_endpoint_set_owner(hubos_device_server_endpoint_t *endpoint,
                                            hubos_id_t owner_session_id) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_device_server_set_owner(endpoint->server, owner_session_id);
}

bool hubos_device_server_endpoint_release_owner(hubos_device_server_endpoint_t *endpoint) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_device_server_release_owner(endpoint->server);
}

bool hubos_device_server_endpoint_is_active(const hubos_device_server_endpoint_t *endpoint) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_device_server_is_active(endpoint->server);
}

bool hubos_device_server_endpoint_attach_mmio(hubos_device_server_endpoint_t *endpoint,
                                              hubos_id_t owner_session_id) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_device_server_attach_mmio(endpoint->server, owner_session_id);
}

bool hubos_device_server_endpoint_attach_irq(hubos_device_server_endpoint_t *endpoint,
                                             hubos_id_t owner_session_id) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_device_server_attach_irq(endpoint->server, owner_session_id);
}

bool hubos_device_server_endpoint_attach_dma(hubos_device_server_endpoint_t *endpoint,
                                             hubos_id_t owner_session_id) {
  if (endpoint == NULL || endpoint->server == NULL) {
    return false;
  }

  return hubos_device_server_attach_dma(endpoint->server, owner_session_id);
}
