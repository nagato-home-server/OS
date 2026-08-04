#ifndef HUBOS_SYSTEM_H
#define HUBOS_SYSTEM_H

#include "hubos/audit.h"
#include "hubos/app_vm_runtime.h"
#include "hubos/bus_manager.h"
#include "hubos/boot.h"
#include "hubos/capability_manager.h"
#include "hubos/dma_manager.h"
#include "hubos/driver_loader.h"
#include "hubos/driver_service.h"
#include "hubos/hub.h"
#include "hubos/memory_manager.h"
#include "hubos/microkit_graph.h"
#include "hubos/microkit_boot.h"
#include "hubos/microkit_ipc.h"
#include "hubos/network_server.h"
#include "hubos/service_endpoints.h"
#include "hubos/resource_registry.h"
#include "hubos/storage_server.h"
#include "hubos/display_server.h"
#include "hubos/device_server.h"
#include "hubos/vm_server.h"
#include "hubos/session_manager.h"

typedef struct hubos_system {
  hubos_resource_registry_t resource_registry;
  hubos_capability_manager_t capability_manager;
  hubos_session_manager_t session_manager;
  hubos_audit_log_t audit_log;
  hubos_memory_manager_t memory_manager;
  hubos_dma_manager_t dma_manager;
  hubos_boot_capability_set_t boot_capabilities;
  hubos_driver_registry_t driver_registry;
  hubos_driver_loader_t driver_loader;
  hubos_driver_service_t driver_service;
  hubos_network_server_t network_server;
  hubos_storage_server_t storage_server;
  hubos_display_server_t display_server;
  hubos_device_server_t device_server;
  hubos_vm_server_t vm_server;
  hubos_microkit_graph_t microkit_graph;
  hubos_microkit_boot_manifest_t microkit_boot_manifest;
  hubos_microkit_ipc_layout_t microkit_ipc_layout;
  hubos_resource_registry_endpoint_t resource_registry_endpoint;
  hubos_capability_manager_endpoint_t capability_manager_endpoint;
  hubos_session_manager_endpoint_t session_manager_endpoint;
  hubos_hub_endpoint_t hub_endpoint;
  hubos_driver_service_endpoint_t driver_service_endpoint;
  hubos_network_server_endpoint_t network_server_endpoint;
  hubos_storage_server_endpoint_t storage_server_endpoint;
  hubos_display_server_endpoint_t display_server_endpoint;
  hubos_device_server_endpoint_t device_server_endpoint;
  hubos_hub_t hub;
  hubos_boot_state_t boot_state;
  hubos_bus_manager_t pcie_bus_manager;
  hubos_bus_manager_t usb_bus_manager;
  hubos_bus_manager_t i2c_bus_manager;
  hubos_bus_manager_t spi_bus_manager;
} hubos_system_t;

void hubos_system_init(hubos_system_t *system, const char *trusted_driver_key_id);
void hubos_system_destroy(hubos_system_t *system);
const hubos_microkit_graph_t *hubos_system_microkit_graph(const hubos_system_t *system);
const hubos_microkit_boot_manifest_t *hubos_system_microkit_boot_manifest(const hubos_system_t *system);
const hubos_microkit_ipc_layout_t *hubos_system_microkit_ipc_layout(const hubos_system_t *system);
const hubos_boot_capability_set_t *hubos_system_boot_capabilities(const hubos_system_t *system);
bool hubos_system_microkit_graph_validate(const hubos_system_t *system);
bool hubos_system_microkit_boot_manifest_validate(const hubos_system_t *system);
bool hubos_system_microkit_ipc_layout_validate(const hubos_system_t *system);
bool hubos_system_boot_capabilities_validate(const hubos_system_t *system);
size_t hubos_system_microkit_boot_order(const hubos_system_t *system,
                                        hubos_microkit_component_kind_t *out_order,
                                        size_t order_capacity);
const char *hubos_system_microkit_component_name(hubos_microkit_component_kind_t kind);
bool hubos_system_complete_boot_step(hubos_system_t *system, hubos_boot_step_t step);
bool hubos_system_boot_step_is_complete(const hubos_system_t *system, hubos_boot_step_t step);
bool hubos_system_microkit_ipc_dispatch(hubos_system_t *system,
                                        const hubos_microkit_ipc_request_t *request,
                                        hubos_microkit_ipc_response_t *response);

bool hubos_system_register_resource(hubos_system_t *system,
                                    const char *name,
                                    size_t name_len,
                                    hubos_resource_state_t state,
                                    hubos_id_t *out_resource_id,
                                    bool *out_is_new);
bool hubos_system_quarantine_resource(hubos_system_t *system, hubos_id_t resource_id);
bool hubos_system_retire_resource(hubos_system_t *system, hubos_id_t resource_id);

bool hubos_system_register_driver(hubos_system_t *system,
                                  unsigned vendor_id,
                                  unsigned device_id,
                                  unsigned class_code,
                                  const char *driver_package,
                                  const char *version,
                                  hubos_id_t *out_driver_id,
                                  bool *out_is_new);

bool hubos_system_update_trusted_driver_key(hubos_system_t *system,
                                            const hubos_driver_keyring_update_t *update);

bool hubos_system_revoke_driver_key(hubos_system_t *system,
                                    const hubos_driver_keyring_revocation_t *revocation);

bool hubos_system_validate_driver_package(const hubos_system_t *system,
                                          const hubos_driver_package_t *package);

bool hubos_system_issue_capability(hubos_system_t *system,
                                   hubos_id_t owner_session_id,
                                   hubos_id_t resource_id,
                                   unsigned rights,
                                   bool delegatable,
                                   hubos_id_t *out_capability_id);

bool hubos_system_copy_capability(hubos_system_t *system,
                                  hubos_id_t source_capability_id,
                                  hubos_id_t owner_session_id,
                                  hubos_id_t *out_capability_id);

bool hubos_system_mint_capability(hubos_system_t *system,
                                  hubos_id_t source_capability_id,
                                  hubos_id_t owner_session_id,
                                  unsigned rights,
                                  bool delegatable,
                                  hubos_id_t *out_capability_id);

bool hubos_system_transfer_capability(hubos_system_t *system,
                                      hubos_id_t capability_id,
                                      hubos_id_t new_owner_session_id);

bool hubos_system_revoke_capability(hubos_system_t *system, hubos_id_t capability_id);

bool hubos_system_create_session(hubos_system_t *system,
                                 hubos_id_t owner_id,
                                 hubos_id_t parent_id,
                                 hubos_session_type_t type,
                                 hubos_id_t *out_session_id);
bool hubos_system_refresh_session_context(hubos_system_t *system,
                                          hubos_id_t session_id,
                                          hubos_id_t namespace_view_version,
                                          hubos_id_t policy_context_version);

bool hubos_system_refresh_session_assets(hubos_system_t *system,
                                         hubos_id_t session_id,
                                         hubos_id_t resource_set_version,
                                         hubos_id_t lease_version);

bool hubos_system_revoke_session_tree(hubos_system_t *system, hubos_id_t session_id);

bool hubos_system_resolve(const hubos_system_t *system,
                          const char *name,
                          size_t name_len,
                          hubos_service_descriptor_t *out_descriptor);

bool hubos_system_authorize(const hubos_system_t *system,
                            hubos_id_t capability_id,
                            hubos_id_t resource_id,
                            unsigned required_rights);

bool hubos_system_map_dma(hubos_system_t *system, hubos_id_t resource_id);
bool hubos_system_finalize_dma_revoke(hubos_system_t *system, hubos_id_t resource_id);
bool hubos_system_abort_dma(hubos_system_t *system, hubos_id_t resource_id);

bool hubos_system_allocate_frame(hubos_system_t *system,
                                 size_t size,
                                 unsigned numa_node,
                                 hubos_id_t *out_memory_id);
bool hubos_system_allocate_hugepage(hubos_system_t *system,
                                    size_t size,
                                    unsigned numa_node,
                                    hubos_id_t *out_memory_id);
bool hubos_system_share_memory(hubos_system_t *system, hubos_id_t memory_id);
bool hubos_system_reclaim_memory(hubos_system_t *system, hubos_id_t memory_id);

bool hubos_system_bind_driver(hubos_system_t *system,
                              hubos_id_t resource_id,
                              hubos_id_t driver_id,
                              const hubos_driver_package_t *package);

bool hubos_system_prepare_rebind_driver(hubos_system_t *system,
                                        hubos_id_t resource_id,
                                        hubos_id_t driver_id,
                                        const hubos_driver_package_t *package);

bool hubos_system_complete_rebind_driver(hubos_system_t *system,
                                         hubos_id_t resource_id,
                                         hubos_id_t driver_id,
                                         const hubos_driver_package_t *package);

bool hubos_system_rebind_driver(hubos_system_t *system,
                                hubos_id_t resource_id,
                                hubos_id_t driver_id,
                                const hubos_driver_package_t *package);

bool hubos_system_quarantine_driver(hubos_system_t *system, hubos_id_t resource_id);

bool hubos_system_unbind_driver(hubos_system_t *system, hubos_id_t resource_id);

bool hubos_system_bind_network_namespace(hubos_system_t *system,
                                        hubos_namespace_handle_t namespace_handle);

bool hubos_system_set_network_policy(hubos_system_t *system,
                                     bool routing_enabled,
                                     bool firewall_enabled);

bool hubos_system_add_network_route(hubos_system_t *system,
                                    const char *destination,
                                    hubos_id_t nic_resource_id,
                                    unsigned metric);

bool hubos_system_set_network_default_route(hubos_system_t *system,
                                            hubos_id_t nic_resource_id);

bool hubos_system_select_network_nic(hubos_system_t *system,
                                     const char *destination,
                                     size_t destination_len,
                                     hubos_id_t *out_nic_resource_id);

bool hubos_system_bind_network_port(hubos_system_t *system,
                                    unsigned port,
                                    hubos_id_t nic_resource_id,
                                    hubos_id_t session_id);

bool hubos_system_set_network_failover_policy(hubos_system_t *system,
                                              bool failover_enabled,
                                              hubos_id_t preferred_nic_resource_id);

bool hubos_system_describe_network_server(const hubos_system_t *system,
                                          hubos_service_descriptor_t *out_descriptor);

bool hubos_system_bind_storage_namespace(hubos_system_t *system,
                                         hubos_namespace_handle_t namespace_handle);
bool hubos_system_release_storage_namespace(hubos_system_t *system);
bool hubos_system_finalize_storage_namespace(hubos_system_t *system);
bool hubos_system_describe_storage_server(const hubos_system_t *system,
                                          hubos_service_descriptor_t *out_descriptor);

bool hubos_system_bind_display_namespace(hubos_system_t *system,
                                         hubos_namespace_handle_t namespace_handle);
bool hubos_system_release_display_namespace(hubos_system_t *system);
bool hubos_system_finalize_display_namespace(hubos_system_t *system);
bool hubos_system_describe_display_server(const hubos_system_t *system,
                                          hubos_service_descriptor_t *out_descriptor);
bool hubos_system_set_vm_guest_memory(hubos_system_t *system, hubos_id_t guest_memory_id);
bool hubos_system_set_vm_vcpu_count(hubos_system_t *system, unsigned vcpu_count);
bool hubos_system_attach_vm_virtio_net(hubos_system_t *system, hubos_id_t session_id);
bool hubos_system_attach_vm_virtio_blk(hubos_system_t *system, hubos_id_t session_id);
bool hubos_system_attach_vm_vgpu(hubos_system_t *system, hubos_id_t session_id);
bool hubos_system_set_vm_artifacts(hubos_system_t *system,
                                   hubos_linux_vm_artifacts_t artifacts);
bool hubos_system_select_vm_runtime_profile(hubos_system_t *system,
                                            const hubos_app_vm_runtime_profile_t *profile);
bool hubos_system_select_vm_runtime_for_app(hubos_system_t *system,
                                            const hubos_app_vm_runtime_selection_t *selection,
                                            hubos_id_t app_id);
bool hubos_system_set_vm_restart_policy(hubos_system_t *system,
                                        hubos_vm_restart_policy_t policy,
                                        unsigned max_restart_attempts);
bool hubos_system_start_vm(hubos_system_t *system);
bool hubos_system_complete_vm_boot(hubos_system_t *system);
bool hubos_system_fail_vm(hubos_system_t *system, unsigned failure_code);
bool hubos_system_stop_vm(hubos_system_t *system);
void hubos_system_set_vm_console_relay(hubos_system_t *system,
                                       bool available,
                                       const char *backend_name);
bool hubos_system_vm_console_relay_available(const hubos_system_t *system);
bool hubos_system_attach_vm_console(hubos_system_t *system);
bool hubos_system_detach_vm_console(hubos_system_t *system);
bool hubos_system_write_vm_console(hubos_system_t *system, const char *text, size_t text_len);
bool hubos_system_describe_vm(const hubos_system_t *system,
                              hubos_service_descriptor_t *out_descriptor);

bool hubos_system_set_device_owner(hubos_system_t *system, hubos_id_t owner_session_id);
bool hubos_system_release_device_owner(hubos_system_t *system);
bool hubos_system_reset_device(hubos_system_t *system);
bool hubos_system_quarantine_device(hubos_system_t *system);
bool hubos_system_clear_device_quarantine(hubos_system_t *system);
bool hubos_system_attach_device_mmio(hubos_system_t *system, hubos_id_t owner_session_id);
bool hubos_system_attach_device_irq(hubos_system_t *system, hubos_id_t owner_session_id);
bool hubos_system_attach_device_dma(hubos_system_t *system, hubos_id_t owner_session_id);
bool hubos_system_describe_device(const hubos_system_t *system,
                                  hubos_service_descriptor_t *out_descriptor);
void hubos_system_set_device_hardware_backend(hubos_system_t *system,
                                              const hubos_device_server_ops_t *ops,
                                              void *ops_context);
void hubos_system_set_bus_hardware_backend(hubos_system_t *system,
                                           hubos_bus_kind_t kind,
                                           const hubos_bus_manager_ops_t *ops,
                                           void *ops_context);

bool hubos_system_bus_discover(hubos_system_t *system,
                               hubos_bus_kind_t kind,
                               const char *resource_name,
                               size_t resource_name_len,
                               hubos_resource_state_t state,
                               hubos_id_t *out_resource_id);

#endif
