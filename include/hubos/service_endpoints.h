#ifndef HUBOS_SERVICE_ENDPOINTS_H
#define HUBOS_SERVICE_ENDPOINTS_H

#include "hubos/audit.h"
#include "hubos/capability_manager.h"
#include "hubos/device_server.h"
#include "hubos/driver_service.h"
#include "hubos/display_server.h"
#include "hubos/hub.h"
#include "hubos/network_server.h"
#include "hubos/resource_registry.h"
#include "hubos/storage_server.h"
#include "hubos/session_manager.h"

typedef struct {
  hubos_resource_registry_t *registry;
  hubos_audit_log_t *audit_log;
} hubos_resource_registry_endpoint_t;

typedef struct {
  hubos_capability_manager_t *manager;
  hubos_audit_log_t *audit_log;
} hubos_capability_manager_endpoint_t;

typedef struct {
  hubos_session_manager_t *manager;
  hubos_capability_manager_t *capability_manager;
  hubos_audit_log_t *audit_log;
} hubos_session_manager_endpoint_t;

typedef struct {
  hubos_hub_t *hub;
} hubos_hub_endpoint_t;

typedef struct {
  hubos_driver_service_t *service;
} hubos_driver_service_endpoint_t;

typedef struct {
  hubos_network_server_t *server;
} hubos_network_server_endpoint_t;

typedef struct {
  hubos_storage_server_t *server;
} hubos_storage_server_endpoint_t;

typedef struct {
  hubos_display_server_t *server;
} hubos_display_server_endpoint_t;

typedef struct {
  hubos_device_server_t *server;
} hubos_device_server_endpoint_t;

void hubos_resource_registry_endpoint_init(hubos_resource_registry_endpoint_t *endpoint,
                                           hubos_resource_registry_t *registry,
                                           hubos_audit_log_t *audit_log);
bool hubos_resource_registry_endpoint_register(hubos_resource_registry_endpoint_t *endpoint,
                                               const char *name,
                                               size_t name_len,
                                               hubos_resource_state_t state,
                                               hubos_id_t *out_resource_id,
                                               bool *out_is_new);
bool hubos_resource_registry_endpoint_update_state(hubos_resource_registry_endpoint_t *endpoint,
                                                  hubos_id_t resource_id,
                                                  hubos_resource_state_t state);
bool hubos_resource_registry_endpoint_quarantine(hubos_resource_registry_endpoint_t *endpoint,
                                                 hubos_id_t resource_id);
bool hubos_resource_registry_endpoint_retire(hubos_resource_registry_endpoint_t *endpoint,
                                             hubos_id_t resource_id);
const hubos_resource_t *hubos_resource_registry_endpoint_get(
  const hubos_resource_registry_endpoint_t *endpoint,
  hubos_id_t resource_id);

void hubos_capability_manager_endpoint_init(hubos_capability_manager_endpoint_t *endpoint,
                                            hubos_capability_manager_t *manager,
                                            hubos_audit_log_t *audit_log);
bool hubos_capability_manager_endpoint_issue(hubos_capability_manager_endpoint_t *endpoint,
                                             hubos_id_t owner_session_id,
                                             hubos_id_t resource_id,
                                             unsigned rights,
                                             bool delegatable,
                                             hubos_id_t *out_capability_id);
bool hubos_capability_manager_endpoint_copy(hubos_capability_manager_endpoint_t *endpoint,
                                            hubos_id_t source_capability_id,
                                            hubos_id_t owner_session_id,
                                            hubos_id_t *out_capability_id);
bool hubos_capability_manager_endpoint_mint_from(hubos_capability_manager_endpoint_t *endpoint,
                                                 hubos_id_t source_capability_id,
                                                 hubos_id_t owner_session_id,
                                                 unsigned rights,
                                                 bool delegatable,
                                                 hubos_id_t *out_capability_id);
bool hubos_capability_manager_endpoint_transfer(hubos_capability_manager_endpoint_t *endpoint,
                                                hubos_id_t capability_id,
                                                hubos_id_t new_owner_session_id);
bool hubos_capability_manager_endpoint_revoke(hubos_capability_manager_endpoint_t *endpoint,
                                              hubos_id_t capability_id);
size_t hubos_capability_manager_endpoint_revoke_owned(
  hubos_capability_manager_endpoint_t *endpoint,
  hubos_id_t owner_session_id);
const hubos_capability_t *hubos_capability_manager_endpoint_get(
  const hubos_capability_manager_endpoint_t *endpoint,
  hubos_id_t capability_id);
bool hubos_capability_manager_endpoint_authorize(
  const hubos_capability_manager_endpoint_t *endpoint,
  hubos_id_t capability_id,
  hubos_id_t resource_id,
  unsigned required_rights);

void hubos_session_manager_endpoint_init(hubos_session_manager_endpoint_t *endpoint,
                                         hubos_session_manager_t *manager,
                                         hubos_capability_manager_t *capability_manager,
                                         hubos_audit_log_t *audit_log);
bool hubos_session_manager_endpoint_create(hubos_session_manager_endpoint_t *endpoint,
                                           hubos_id_t owner_id,
                                           hubos_id_t parent_id,
                                           hubos_session_type_t type,
                                           hubos_id_t *out_session_id);
bool hubos_session_manager_endpoint_refresh_context(
  hubos_session_manager_endpoint_t *endpoint,
  hubos_id_t session_id,
  hubos_id_t namespace_view_version,
  hubos_id_t policy_context_version);
const hubos_session_t *hubos_session_manager_endpoint_get(
  const hubos_session_manager_endpoint_t *endpoint,
  hubos_id_t session_id);
bool hubos_session_manager_endpoint_set_state(hubos_session_manager_endpoint_t *endpoint,
                                              hubos_id_t session_id,
                                              hubos_session_state_t state);
bool hubos_session_manager_endpoint_is_ancestor(
  const hubos_session_manager_endpoint_t *endpoint,
  hubos_id_t ancestor_id,
  hubos_id_t session_id);
size_t hubos_session_manager_endpoint_child_count(
  const hubos_session_manager_endpoint_t *endpoint,
  hubos_id_t session_id);
bool hubos_session_manager_endpoint_revoke_tree(hubos_session_manager_endpoint_t *endpoint,
                                                hubos_id_t session_id);

void hubos_hub_endpoint_init(hubos_hub_endpoint_t *endpoint, hubos_hub_t *hub);
bool hubos_hub_endpoint_resolve(const hubos_hub_endpoint_t *endpoint,
                                const char *name,
                                size_t name_len,
                                hubos_service_descriptor_t *out_descriptor);

void hubos_driver_service_endpoint_init(hubos_driver_service_endpoint_t *endpoint,
                                        hubos_driver_service_t *service);
bool hubos_driver_service_endpoint_bind(hubos_driver_service_endpoint_t *endpoint,
                                        hubos_id_t resource_id,
                                        hubos_id_t driver_id,
                                        const hubos_driver_package_t *package);
bool hubos_driver_service_endpoint_prepare_rebind(
  hubos_driver_service_endpoint_t *endpoint,
  hubos_id_t resource_id,
  hubos_id_t driver_id,
  const hubos_driver_package_t *package);
bool hubos_driver_service_endpoint_complete_rebind(
  hubos_driver_service_endpoint_t *endpoint,
  hubos_id_t resource_id,
  hubos_id_t driver_id,
  const hubos_driver_package_t *package);
bool hubos_driver_service_endpoint_rebind(hubos_driver_service_endpoint_t *endpoint,
                                          hubos_id_t resource_id,
                                          hubos_id_t driver_id,
                                          const hubos_driver_package_t *package);
bool hubos_driver_service_endpoint_quarantine(hubos_driver_service_endpoint_t *endpoint,
                                              hubos_id_t resource_id);
bool hubos_driver_service_endpoint_unbind(hubos_driver_service_endpoint_t *endpoint,
                                          hubos_id_t resource_id);
const hubos_driver_binding_t *hubos_driver_service_endpoint_get(
  const hubos_driver_service_endpoint_t *endpoint,
  hubos_id_t resource_id);

void hubos_network_server_endpoint_init(hubos_network_server_endpoint_t *endpoint,
                                       hubos_network_server_t *server);
bool hubos_network_server_endpoint_bind_namespace(
  hubos_network_server_endpoint_t *endpoint,
  hubos_namespace_handle_t namespace_handle);
bool hubos_network_server_endpoint_set_policy(hubos_network_server_endpoint_t *endpoint,
                                              bool routing_enabled,
                                              bool firewall_enabled);
bool hubos_network_server_endpoint_add_route(hubos_network_server_endpoint_t *endpoint,
                                             const char *destination,
                                             hubos_id_t nic_resource_id,
                                             unsigned metric);
bool hubos_network_server_endpoint_set_default_route(
  hubos_network_server_endpoint_t *endpoint,
  hubos_id_t nic_resource_id);
bool hubos_network_server_endpoint_select_nic(hubos_network_server_endpoint_t *endpoint,
                                              const char *destination,
                                              size_t destination_len,
                                              hubos_id_t *out_nic_resource_id);
bool hubos_network_server_endpoint_bind_port(hubos_network_server_endpoint_t *endpoint,
                                            unsigned port,
                                            hubos_id_t nic_resource_id,
                                            hubos_id_t session_id);
bool hubos_network_server_endpoint_set_failover_policy(
  hubos_network_server_endpoint_t *endpoint,
  bool failover_enabled,
  hubos_id_t preferred_nic_resource_id);
bool hubos_network_server_endpoint_describe(const hubos_network_server_endpoint_t *endpoint,
                                            hubos_service_descriptor_t *out_descriptor);

void hubos_storage_server_endpoint_init(hubos_storage_server_endpoint_t *endpoint,
                                       hubos_storage_server_t *server);
bool hubos_storage_server_endpoint_bind_namespace(
  hubos_storage_server_endpoint_t *endpoint,
  hubos_namespace_handle_t namespace_handle);
bool hubos_storage_server_endpoint_release_namespace(
  hubos_storage_server_endpoint_t *endpoint);
bool hubos_storage_server_endpoint_finalize_namespace(
  hubos_storage_server_endpoint_t *endpoint);
bool hubos_storage_server_endpoint_describe(const hubos_storage_server_endpoint_t *endpoint,
                                            hubos_service_descriptor_t *out_descriptor);

void hubos_display_server_endpoint_init(hubos_display_server_endpoint_t *endpoint,
                                       hubos_display_server_t *server);
bool hubos_display_server_endpoint_bind_namespace(
  hubos_display_server_endpoint_t *endpoint,
  hubos_namespace_handle_t namespace_handle);
bool hubos_display_server_endpoint_release_namespace(
  hubos_display_server_endpoint_t *endpoint);
bool hubos_display_server_endpoint_finalize_namespace(
  hubos_display_server_endpoint_t *endpoint);
bool hubos_display_server_endpoint_describe(const hubos_display_server_endpoint_t *endpoint,
                                            hubos_service_descriptor_t *out_descriptor);

void hubos_device_server_endpoint_init(hubos_device_server_endpoint_t *endpoint,
                                       hubos_device_server_t *server);
bool hubos_device_server_endpoint_quarantine(hubos_device_server_endpoint_t *endpoint);
bool hubos_device_server_endpoint_clear_quarantine(hubos_device_server_endpoint_t *endpoint);
bool hubos_device_server_endpoint_reset(hubos_device_server_endpoint_t *endpoint);
bool hubos_device_server_endpoint_set_owner(hubos_device_server_endpoint_t *endpoint,
                                            hubos_id_t owner_session_id);
bool hubos_device_server_endpoint_release_owner(hubos_device_server_endpoint_t *endpoint);
bool hubos_device_server_endpoint_is_active(const hubos_device_server_endpoint_t *endpoint);
bool hubos_device_server_endpoint_attach_mmio(hubos_device_server_endpoint_t *endpoint,
                                              hubos_id_t owner_session_id);
bool hubos_device_server_endpoint_attach_irq(hubos_device_server_endpoint_t *endpoint,
                                             hubos_id_t owner_session_id);
bool hubos_device_server_endpoint_attach_dma(hubos_device_server_endpoint_t *endpoint,
                                             hubos_id_t owner_session_id);

#endif
