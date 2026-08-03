#include "hubos/system.h"

#include <string.h>

#if defined(HUBOS_USE_LINUX_USBIO_BACKEND) && HUBOS_USE_LINUX_USBIO_BACKEND
#include "hubos/linux_usbio_backend.h"
#endif

#include "hubos/runtime_config.h"

void hubos_system_init(hubos_system_t *system, const char *trusted_driver_key_id) {
  if (system == NULL) {
    return;
  }

  hubos_resource_registry_init(&system->resource_registry);
  hubos_capability_manager_init(&system->capability_manager);
  hubos_session_manager_init(&system->session_manager);
  hubos_audit_log_init(&system->audit_log);
  hubos_memory_manager_init(&system->memory_manager);
  hubos_dma_manager_init(&system->dma_manager);
  hubos_boot_capability_set_init(&system->boot_capabilities);
  (void)hubos_boot_capability_set_grant(&system->boot_capabilities, HUBOS_BOOT_CAP_FIRMWARE);
  (void)hubos_boot_capability_set_grant(&system->boot_capabilities, HUBOS_BOOT_CAP_SEL4);
  (void)hubos_boot_capability_set_grant(&system->boot_capabilities, HUBOS_BOOT_CAP_ROOT_TASK);
  (void)hubos_boot_capability_set_grant(&system->boot_capabilities,
                                        HUBOS_BOOT_CAP_RESOURCE_REGISTRY);
  (void)hubos_boot_capability_set_grant(&system->boot_capabilities,
                                        HUBOS_BOOT_CAP_CAPABILITY_MANAGER);
  (void)hubos_boot_capability_set_grant(&system->boot_capabilities, HUBOS_BOOT_CAP_MEMORY_MANAGER);
  (void)hubos_boot_capability_set_grant(&system->boot_capabilities, HUBOS_BOOT_CAP_DMA_MANAGER);
  (void)hubos_boot_capability_set_grant(&system->boot_capabilities, HUBOS_BOOT_CAP_HUB);
  hubos_driver_registry_init(&system->driver_registry);
  hubos_driver_loader_init(&system->driver_loader,
                           &system->driver_registry,
                           &system->audit_log,
                           trusted_driver_key_id);
  hubos_driver_service_init(&system->driver_service,
                            &system->driver_registry,
                            &system->driver_loader,
                            &system->audit_log);
  hubos_network_server_init(&system->network_server, 1, HUBOS_ID_INVALID);
  hubos_storage_server_init(&system->storage_server, 2, HUBOS_ID_INVALID, (hubos_namespace_handle_t){0});
  hubos_display_server_init(&system->display_server, 3, HUBOS_ID_INVALID, (hubos_namespace_handle_t){0});
  hubos_device_server_init(&system->device_server, 4, HUBOS_ID_INVALID, HUBOS_ID_INVALID, "device-server");
  hubos_vm_server_init(&system->vm_server,
                       "libvmm",
                       5,
                       HUBOS_ID_INVALID,
                       (hubos_vm_t){0},
                       (hubos_linux_vm_artifacts_t){0});
  (void)hubos_vm_server_select_runtime_profile(&system->vm_server,
                                               hubos_runtime_config_default_profile());
  hubos_microkit_graph_init(&system->microkit_graph);
  hubos_microkit_boot_manifest_init(&system->microkit_boot_manifest);
  hubos_microkit_ipc_layout_init(&system->microkit_ipc_layout);
  hubos_hub_init(&system->hub, &system->resource_registry, &system->capability_manager);
  hubos_resource_registry_endpoint_init(&system->resource_registry_endpoint,
                                        &system->resource_registry,
                                        &system->audit_log);
  hubos_capability_manager_endpoint_init(&system->capability_manager_endpoint,
                                         &system->capability_manager,
                                         &system->audit_log);
  hubos_session_manager_endpoint_init(&system->session_manager_endpoint,
                                      &system->session_manager,
                                      &system->capability_manager,
                                      &system->audit_log);
  hubos_hub_endpoint_init(&system->hub_endpoint, &system->hub);
  hubos_driver_service_endpoint_init(&system->driver_service_endpoint, &system->driver_service);
  hubos_network_server_endpoint_init(&system->network_server_endpoint, &system->network_server);
  hubos_storage_server_endpoint_init(&system->storage_server_endpoint, &system->storage_server);
  hubos_display_server_endpoint_init(&system->display_server_endpoint, &system->display_server);
  hubos_device_server_endpoint_init(&system->device_server_endpoint, &system->device_server);
  hubos_boot_state_init(&system->boot_state);
  hubos_bus_manager_init(&system->pcie_bus_manager,
                         HUBOS_BUS_PCIE,
                         "pcie",
                         &system->resource_registry,
                         &system->audit_log);
  hubos_bus_manager_init(&system->usb_bus_manager,
                         HUBOS_BUS_USB,
                         "usb",
                         &system->resource_registry,
                         &system->audit_log);
  hubos_bus_manager_init(&system->i2c_bus_manager,
                         HUBOS_BUS_I2C,
                         "i2c",
                         &system->resource_registry,
                         &system->audit_log);
  hubos_bus_manager_init(&system->spi_bus_manager,
                         HUBOS_BUS_SPI,
                         "spi",
                         &system->resource_registry,
                         &system->audit_log);
#if defined(HUBOS_USE_LINUX_USBIO_BACKEND) && HUBOS_USE_LINUX_USBIO_BACKEND
  if (hubos_linux_usbio_backend_is_requested()) {
    (void)hubos_system_enable_linux_usbio_backend(system);
  }
#endif
}

void hubos_system_destroy(hubos_system_t *system) {
  if (system == NULL) {
    return;
  }

  hubos_network_server_destroy(&system->network_server);
  hubos_storage_server_endpoint_init(&system->storage_server_endpoint, NULL);
  hubos_display_server_endpoint_init(&system->display_server_endpoint, NULL);
  hubos_device_server_endpoint_init(&system->device_server_endpoint, NULL);
  hubos_microkit_graph_destroy(&system->microkit_graph);
  system->microkit_boot_manifest.components = NULL;
  system->microkit_boot_manifest.component_count = 0;
  system->microkit_ipc_layout.bindings = NULL;
  system->microkit_ipc_layout.binding_count = 0;
  hubos_driver_service_destroy(&system->driver_service);
  hubos_driver_loader_destroy(&system->driver_loader);
  hubos_driver_registry_destroy(&system->driver_registry);
  hubos_dma_manager_destroy(&system->dma_manager);
  hubos_memory_manager_destroy(&system->memory_manager);
  hubos_audit_log_destroy(&system->audit_log);
  hubos_boot_state_init(&system->boot_state);
  hubos_boot_capability_set_init(&system->boot_capabilities);
  hubos_session_manager_destroy(&system->session_manager);
  hubos_capability_manager_destroy(&system->capability_manager);
  hubos_resource_registry_destroy(&system->resource_registry);
  hubos_resource_registry_endpoint_init(&system->resource_registry_endpoint, NULL, NULL);
  hubos_capability_manager_endpoint_init(&system->capability_manager_endpoint, NULL, NULL);
  hubos_session_manager_endpoint_init(&system->session_manager_endpoint, NULL, NULL, NULL);
  hubos_hub_endpoint_init(&system->hub_endpoint, NULL);
  hubos_driver_service_endpoint_init(&system->driver_service_endpoint, NULL);
  hubos_network_server_endpoint_init(&system->network_server_endpoint, NULL);
  hubos_storage_server_endpoint_init(&system->storage_server_endpoint, NULL);
  hubos_display_server_endpoint_init(&system->display_server_endpoint, NULL);
  hubos_device_server_endpoint_init(&system->device_server_endpoint, NULL);
  hubos_vm_server_init(&system->vm_server,
                       NULL,
                       0,
                       HUBOS_ID_INVALID,
                       (hubos_vm_t){0},
                       (hubos_linux_vm_artifacts_t){0});
}

const hubos_microkit_graph_t *hubos_system_microkit_graph(const hubos_system_t *system) {
  if (system == NULL) {
    return NULL;
  }

  return &system->microkit_graph;
}

const hubos_microkit_boot_manifest_t *hubos_system_microkit_boot_manifest(const hubos_system_t *system) {
  if (system == NULL) {
    return NULL;
  }

  return &system->microkit_boot_manifest;
}

const hubos_microkit_ipc_layout_t *hubos_system_microkit_ipc_layout(const hubos_system_t *system) {
  if (system == NULL) {
    return NULL;
  }

  return &system->microkit_ipc_layout;
}

const hubos_boot_capability_set_t *hubos_system_boot_capabilities(const hubos_system_t *system) {
  if (system == NULL) {
    return NULL;
  }

  return &system->boot_capabilities;
}

bool hubos_system_microkit_graph_validate(const hubos_system_t *system) {
  if (system == NULL) {
    return false;
  }

  return hubos_microkit_graph_validate(&system->microkit_graph);
}

bool hubos_system_microkit_boot_manifest_validate(const hubos_system_t *system) {
  if (system == NULL) {
    return false;
  }

  return hubos_microkit_boot_manifest_validate(&system->microkit_boot_manifest,
                                                &system->microkit_graph,
                                                &system->microkit_ipc_layout);
}

bool hubos_system_microkit_ipc_layout_validate(const hubos_system_t *system) {
  if (system == NULL) {
    return false;
  }

  return hubos_microkit_ipc_layout_validate(&system->microkit_ipc_layout, &system->microkit_graph);
}

bool hubos_system_boot_capabilities_validate(const hubos_system_t *system) {
  if (system == NULL) {
    return false;
  }

  return hubos_boot_capability_set_validate_minimal(&system->boot_capabilities);
}

size_t hubos_system_microkit_boot_order(const hubos_system_t *system,
                                        hubos_microkit_component_kind_t *out_order,
                                        size_t order_capacity) {
  if (system == NULL) {
    return 0;
  }

  return hubos_microkit_graph_boot_order(&system->microkit_graph, out_order, order_capacity);
}

const char *hubos_system_microkit_component_name(hubos_microkit_component_kind_t kind) {
  return hubos_microkit_component_name(kind);
}

bool hubos_system_complete_boot_step(hubos_system_t *system, hubos_boot_step_t step) {
  if (system == NULL) {
    return false;
  }

  return hubos_boot_state_complete_step(&system->boot_state, &system->audit_log, step);
}

bool hubos_system_boot_step_is_complete(const hubos_system_t *system, hubos_boot_step_t step) {
  if (system == NULL) {
    return false;
  }

  return hubos_boot_state_is_complete(&system->boot_state, step);
}

bool hubos_system_microkit_ipc_dispatch(hubos_system_t *system,
                                        const hubos_microkit_ipc_request_t *request,
                                        hubos_microkit_ipc_response_t *response) {
  return hubos_microkit_ipc_dispatch(system, request, response);
}

bool hubos_system_register_resource(hubos_system_t *system,
                                    const char *name,
                                    size_t name_len,
                                    hubos_resource_state_t state,
                                    hubos_id_t *out_resource_id,
                                    bool *out_is_new) {
  return hubos_resource_registry_endpoint_register(&system->resource_registry_endpoint,
                                                   name,
                                                   name_len,
                                                   state,
                                                   out_resource_id,
                                                   out_is_new);
}

bool hubos_system_quarantine_resource(hubos_system_t *system, hubos_id_t resource_id) {
  return hubos_resource_registry_endpoint_quarantine(&system->resource_registry_endpoint,
                                                    resource_id);
}

bool hubos_system_retire_resource(hubos_system_t *system, hubos_id_t resource_id) {
  return hubos_resource_registry_endpoint_retire(&system->resource_registry_endpoint,
                                                 resource_id);
}

bool hubos_system_register_driver(hubos_system_t *system,
                                  unsigned vendor_id,
                                  unsigned device_id,
                                  unsigned class_code,
                                  const char *driver_package,
                                  const char *version,
                                  hubos_id_t *out_driver_id,
                                  bool *out_is_new) {
  return hubos_driver_registry_register(&system->driver_registry,
                                        vendor_id,
                                        device_id,
                                        class_code,
                                        driver_package,
                                        version,
                                        out_driver_id,
                                        out_is_new);
}

bool hubos_system_update_trusted_driver_key(hubos_system_t *system,
                                            const hubos_driver_keyring_update_t *update) {
  return hubos_driver_loader_update_trusted_key(&system->driver_loader, update);
}

bool hubos_system_revoke_driver_key(hubos_system_t *system,
                                    const hubos_driver_keyring_revocation_t *revocation) {
  return hubos_driver_loader_revoke_key(&system->driver_loader, revocation);
}

bool hubos_system_validate_driver_package(const hubos_system_t *system,
                                          const hubos_driver_package_t *package) {
  return hubos_driver_loader_validate_package(&system->driver_loader, package);
}

bool hubos_system_issue_capability(hubos_system_t *system,
                                   hubos_id_t owner_session_id,
                                   hubos_id_t resource_id,
                                   unsigned rights,
                                   bool delegatable,
                                   hubos_id_t *out_capability_id) {
  return hubos_capability_manager_endpoint_issue(&system->capability_manager_endpoint,
                                                 owner_session_id,
                                                 resource_id,
                                                 rights,
                                                 delegatable,
                                                 out_capability_id);
}

bool hubos_system_copy_capability(hubos_system_t *system,
                                  hubos_id_t source_capability_id,
                                  hubos_id_t owner_session_id,
                                  hubos_id_t *out_capability_id) {
  return hubos_capability_manager_endpoint_copy(&system->capability_manager_endpoint,
                                                source_capability_id,
                                                owner_session_id,
                                                out_capability_id);
}

bool hubos_system_mint_capability(hubos_system_t *system,
                                  hubos_id_t source_capability_id,
                                  hubos_id_t owner_session_id,
                                  unsigned rights,
                                  bool delegatable,
                                  hubos_id_t *out_capability_id) {
  return hubos_capability_manager_endpoint_mint_from(&system->capability_manager_endpoint,
                                                     source_capability_id,
                                                     owner_session_id,
                                                     rights,
                                                     delegatable,
                                                     out_capability_id);
}

bool hubos_system_transfer_capability(hubos_system_t *system,
                                      hubos_id_t capability_id,
                                      hubos_id_t new_owner_session_id) {
  return hubos_capability_manager_endpoint_transfer(&system->capability_manager_endpoint,
                                                    capability_id,
                                                    new_owner_session_id);
}

bool hubos_system_revoke_capability(hubos_system_t *system, hubos_id_t capability_id) {
  return hubos_capability_manager_endpoint_revoke(&system->capability_manager_endpoint,
                                                  capability_id);
}

bool hubos_system_create_session(hubos_system_t *system,
                                 hubos_id_t owner_id,
                                 hubos_id_t parent_id,
                                 hubos_session_type_t type,
                                 hubos_id_t *out_session_id) {
  return hubos_session_manager_endpoint_create(&system->session_manager_endpoint,
                                               owner_id,
                                               parent_id,
                                               type,
                                               out_session_id);
}

bool hubos_system_refresh_session_context(hubos_system_t *system,
                                          hubos_id_t session_id,
                                          hubos_id_t namespace_view_version,
                                          hubos_id_t policy_context_version) {
  return hubos_session_manager_endpoint_refresh_context(&system->session_manager_endpoint,
                                                        session_id,
                                                        namespace_view_version,
                                                        policy_context_version);
}

bool hubos_system_refresh_session_assets(hubos_system_t *system,
                                         hubos_id_t session_id,
                                         hubos_id_t resource_set_version,
                                         hubos_id_t lease_version) {
  return hubos_session_manager_refresh_assets(&system->session_manager,
                                              session_id,
                                              resource_set_version,
                                              lease_version);
}

bool hubos_system_revoke_session_tree(hubos_system_t *system, hubos_id_t session_id) {
  return hubos_session_manager_endpoint_revoke_tree(&system->session_manager_endpoint,
                                                    session_id);
}

bool hubos_system_resolve(const hubos_system_t *system,
                          const char *name,
                          size_t name_len,
                          hubos_service_descriptor_t *out_descriptor) {
  return hubos_hub_endpoint_resolve(&system->hub_endpoint, name, name_len, out_descriptor);
}

bool hubos_system_authorize(const hubos_system_t *system,
                            hubos_id_t capability_id,
                            hubos_id_t resource_id,
                            unsigned required_rights) {
  return hubos_capability_manager_endpoint_authorize(&system->capability_manager_endpoint,
                                                     capability_id,
                                                     resource_id,
                                                     required_rights);
}

bool hubos_system_map_dma(hubos_system_t *system, hubos_id_t resource_id) {
  bool ok = hubos_dma_manager_map(&system->dma_manager, resource_id);
  if (ok) {
    (void)hubos_audit_log_record(&system->audit_log,
                                 HUBOS_AUDIT_DMA_MAPPED,
                                 0,
                                 resource_id,
                                 0,
                                 0);
  }
  return ok;
}

bool hubos_system_finalize_dma_revoke(hubos_system_t *system, hubos_id_t resource_id) {
  bool ok = hubos_dma_manager_finalize_revoke(&system->dma_manager, resource_id);
  if (ok) {
    (void)hubos_audit_log_record(&system->audit_log,
                                 HUBOS_AUDIT_DMA_REVOKED,
                                 0,
                                 resource_id,
                                 0,
                                 0);
  }
  return ok;
}

bool hubos_system_abort_dma(hubos_system_t *system, hubos_id_t resource_id) {
  bool ok = hubos_dma_manager_abort(&system->dma_manager, resource_id);
  if (ok) {
    (void)hubos_audit_log_record(&system->audit_log,
                                 HUBOS_AUDIT_DMA_ABORTED,
                                 0,
                                 resource_id,
                                 0,
                                 0);
  }
  return ok;
}

bool hubos_system_allocate_frame(hubos_system_t *system,
                                 size_t size,
                                 unsigned numa_node,
                                 hubos_id_t *out_memory_id) {
  return hubos_memory_manager_allocate_frame(&system->memory_manager,
                                             size,
                                             numa_node,
                                             out_memory_id);
}

bool hubos_system_allocate_hugepage(hubos_system_t *system,
                                    size_t size,
                                    unsigned numa_node,
                                    hubos_id_t *out_memory_id) {
  return hubos_memory_manager_allocate_hugepage(&system->memory_manager,
                                                size,
                                                numa_node,
                                                out_memory_id);
}

bool hubos_system_share_memory(hubos_system_t *system, hubos_id_t memory_id) {
  return hubos_memory_manager_share(&system->memory_manager, memory_id);
}

bool hubos_system_reclaim_memory(hubos_system_t *system, hubos_id_t memory_id) {
  return hubos_memory_manager_reclaim(&system->memory_manager, memory_id);
}

bool hubos_system_bind_driver(hubos_system_t *system,
                              hubos_id_t resource_id,
                              hubos_id_t driver_id,
                              const hubos_driver_package_t *package) {
  return hubos_driver_service_endpoint_bind(&system->driver_service_endpoint,
                                            resource_id,
                                            driver_id,
                                            package);
}

bool hubos_system_prepare_rebind_driver(hubos_system_t *system,
                                        hubos_id_t resource_id,
                                        hubos_id_t driver_id,
                                        const hubos_driver_package_t *package) {
  return hubos_driver_service_endpoint_prepare_rebind(&system->driver_service_endpoint,
                                                      resource_id,
                                                      driver_id,
                                                      package);
}

bool hubos_system_complete_rebind_driver(hubos_system_t *system,
                                         hubos_id_t resource_id,
                                         hubos_id_t driver_id,
                                         const hubos_driver_package_t *package) {
  return hubos_driver_service_endpoint_complete_rebind(&system->driver_service_endpoint,
                                                       resource_id,
                                                       driver_id,
                                                       package);
}

bool hubos_system_rebind_driver(hubos_system_t *system,
                                hubos_id_t resource_id,
                                hubos_id_t driver_id,
                                const hubos_driver_package_t *package) {
  return hubos_system_prepare_rebind_driver(system, resource_id, driver_id, package) &&
         hubos_system_complete_rebind_driver(system, resource_id, driver_id, package);
}

bool hubos_system_quarantine_driver(hubos_system_t *system, hubos_id_t resource_id) {
  return hubos_driver_service_endpoint_quarantine(&system->driver_service_endpoint,
                                                  resource_id);
}

bool hubos_system_unbind_driver(hubos_system_t *system, hubos_id_t resource_id) {
  return hubos_driver_service_endpoint_unbind(&system->driver_service_endpoint, resource_id);
}

bool hubos_system_bind_network_namespace(hubos_system_t *system,
                                        hubos_namespace_handle_t namespace_handle) {
  return hubos_network_server_endpoint_bind_namespace(&system->network_server_endpoint,
                                                      namespace_handle);
}

bool hubos_system_set_network_policy(hubos_system_t *system,
                                     bool routing_enabled,
                                     bool firewall_enabled) {
  return hubos_network_server_endpoint_set_policy(&system->network_server_endpoint,
                                                   routing_enabled,
                                                   firewall_enabled);
}

bool hubos_system_add_network_route(hubos_system_t *system,
                                    const char *destination,
                                    hubos_id_t nic_resource_id,
                                    unsigned metric) {
  return hubos_network_server_endpoint_add_route(&system->network_server_endpoint,
                                                 destination,
                                                 nic_resource_id,
                                                 metric);
}

bool hubos_system_set_network_default_route(hubos_system_t *system,
                                            hubos_id_t nic_resource_id) {
  return hubos_network_server_endpoint_set_default_route(&system->network_server_endpoint,
                                                         nic_resource_id);
}

bool hubos_system_select_network_nic(hubos_system_t *system,
                                     const char *destination,
                                     size_t destination_len,
                                     hubos_id_t *out_nic_resource_id) {
  return hubos_network_server_endpoint_select_nic(&system->network_server_endpoint,
                                                  destination,
                                                  destination_len,
                                                  out_nic_resource_id);
}

bool hubos_system_bind_network_port(hubos_system_t *system,
                                    unsigned port,
                                    hubos_id_t nic_resource_id,
                                    hubos_id_t session_id) {
  return hubos_network_server_endpoint_bind_port(&system->network_server_endpoint,
                                                 port,
                                                 nic_resource_id,
                                                 session_id);
}

bool hubos_system_set_network_failover_policy(hubos_system_t *system,
                                              bool failover_enabled,
                                              hubos_id_t preferred_nic_resource_id) {
  return hubos_network_server_endpoint_set_failover_policy(&system->network_server_endpoint,
                                                           failover_enabled,
                                                           preferred_nic_resource_id);
}

bool hubos_system_describe_network_server(const hubos_system_t *system,
                                          hubos_service_descriptor_t *out_descriptor) {
  return hubos_network_server_endpoint_describe(&system->network_server_endpoint, out_descriptor);
}

bool hubos_system_bind_storage_namespace(hubos_system_t *system,
                                         hubos_namespace_handle_t namespace_handle) {
  return hubos_storage_server_endpoint_bind_namespace(&system->storage_server_endpoint,
                                                      namespace_handle);
}

bool hubos_system_release_storage_namespace(hubos_system_t *system) {
  return hubos_storage_server_endpoint_release_namespace(&system->storage_server_endpoint);
}

bool hubos_system_finalize_storage_namespace(hubos_system_t *system) {
  return hubos_storage_server_endpoint_finalize_namespace(&system->storage_server_endpoint);
}

bool hubos_system_describe_storage_server(const hubos_system_t *system,
                                          hubos_service_descriptor_t *out_descriptor) {
  return hubos_storage_server_endpoint_describe(&system->storage_server_endpoint, out_descriptor);
}

bool hubos_system_bind_display_namespace(hubos_system_t *system,
                                         hubos_namespace_handle_t namespace_handle) {
  return hubos_display_server_endpoint_bind_namespace(&system->display_server_endpoint,
                                                      namespace_handle);
}

bool hubos_system_release_display_namespace(hubos_system_t *system) {
  return hubos_display_server_endpoint_release_namespace(&system->display_server_endpoint);
}

bool hubos_system_finalize_display_namespace(hubos_system_t *system) {
  return hubos_display_server_endpoint_finalize_namespace(&system->display_server_endpoint);
}

bool hubos_system_describe_display_server(const hubos_system_t *system,
                                          hubos_service_descriptor_t *out_descriptor) {
  return hubos_display_server_endpoint_describe(&system->display_server_endpoint, out_descriptor);
}

bool hubos_system_set_vm_guest_memory(hubos_system_t *system, hubos_id_t guest_memory_id) {
  return hubos_vm_server_attach_guest_memory(&system->vm_server, guest_memory_id);
}

bool hubos_system_set_vm_vcpu_count(hubos_system_t *system, unsigned vcpu_count) {
  return hubos_vm_server_set_vcpu_count(&system->vm_server, vcpu_count);
}

bool hubos_system_attach_vm_virtio_net(hubos_system_t *system, hubos_id_t session_id) {
  return hubos_vm_server_attach_virtio_net_session(&system->vm_server, session_id);
}

bool hubos_system_attach_vm_virtio_blk(hubos_system_t *system, hubos_id_t session_id) {
  return hubos_vm_server_attach_virtio_blk_session(&system->vm_server, session_id);
}

bool hubos_system_attach_vm_vgpu(hubos_system_t *system, hubos_id_t session_id) {
  return hubos_vm_server_attach_vgpu_session(&system->vm_server, session_id);
}

bool hubos_system_set_vm_artifacts(hubos_system_t *system,
                                   hubos_linux_vm_artifacts_t artifacts) {
  hubos_vm_server_set_artifacts(&system->vm_server, artifacts);
  return true;
}

bool hubos_system_select_vm_runtime_profile(hubos_system_t *system,
                                            const hubos_app_vm_runtime_profile_t *profile) {
  if (system == NULL) {
    return false;
  }

  return hubos_vm_server_select_runtime_profile(&system->vm_server, profile);
}

bool hubos_system_select_vm_runtime_for_app(hubos_system_t *system,
                                            const hubos_app_vm_runtime_selection_t *selection,
                                            hubos_id_t app_id) {
  const hubos_app_vm_runtime_profile_t *profile = NULL;

  if (system == NULL || selection == NULL) {
    return false;
  }

  profile = hubos_app_vm_runtime_selection_resolve(selection, app_id);
  if (profile == NULL) {
    return false;
  }

  return hubos_system_select_vm_runtime_profile(system, profile);
}

bool hubos_system_set_vm_restart_policy(hubos_system_t *system,
                                        hubos_vm_restart_policy_t policy,
                                        unsigned max_restart_attempts) {
  bool ok = hubos_vm_server_set_restart_policy(&system->vm_server, policy, max_restart_attempts);
  if (ok) {
    (void)hubos_audit_log_record(&system->audit_log,
                                 HUBOS_AUDIT_VM_RESTART_POLICY_CHANGED,
                                 system->vm_server.owner_session_id,
                                 system->vm_server.id,
                                 0,
                                 max_restart_attempts);
  }
  return ok;
}

bool hubos_system_start_vm(hubos_system_t *system) {
  bool ok = hubos_vm_server_start(&system->vm_server);
  if (ok) {
    (void)hubos_audit_log_record(&system->audit_log,
                                 HUBOS_AUDIT_VM_START_REQUESTED,
                                 system->vm_server.owner_session_id,
                                 system->vm_server.id,
                                 0,
                                 0);
  }
  return ok;
}

bool hubos_system_complete_vm_boot(hubos_system_t *system) {
  bool ok = hubos_vm_server_complete_boot(&system->vm_server);
  if (ok) {
    (void)hubos_audit_log_record(&system->audit_log,
                                 HUBOS_AUDIT_VM_BOOT_COMPLETED,
                                 system->vm_server.owner_session_id,
                                 system->vm_server.id,
                                 0,
                                 0);
  }
  return ok;
}

bool hubos_system_fail_vm(hubos_system_t *system, unsigned failure_code) {
  bool restarting = false;
  bool ok = hubos_vm_server_fail(&system->vm_server, failure_code, &restarting);
  if (ok) {
    (void)hubos_audit_log_record(&system->audit_log,
                                 HUBOS_AUDIT_VM_FAILED,
                                 system->vm_server.owner_session_id,
                                 system->vm_server.id,
                                 0,
                                 failure_code);
    if (restarting) {
      (void)hubos_audit_log_record(&system->audit_log,
                                   HUBOS_AUDIT_VM_RESTART_SCHEDULED,
                                   system->vm_server.owner_session_id,
                                   system->vm_server.id,
                                   0,
                                   system->vm_server.restart_attempts);
    }
  }
  return ok;
}

bool hubos_system_stop_vm(hubos_system_t *system) {
  bool ok = hubos_vm_server_stop(&system->vm_server);
  if (ok) {
    (void)hubos_audit_log_record(&system->audit_log,
                                 HUBOS_AUDIT_VM_STOPPED,
                                 system->vm_server.owner_session_id,
                                 system->vm_server.id,
                                 0,
                                 0);
  }
  return ok;
}

bool hubos_system_describe_vm(const hubos_system_t *system,
                              hubos_service_descriptor_t *out_descriptor) {
  return hubos_vm_server_describe(&system->vm_server, out_descriptor);
}

bool hubos_system_set_device_owner(hubos_system_t *system, hubos_id_t owner_session_id) {
  return hubos_device_server_endpoint_set_owner(&system->device_server_endpoint,
                                                owner_session_id);
}

bool hubos_system_release_device_owner(hubos_system_t *system) {
  return hubos_device_server_endpoint_release_owner(&system->device_server_endpoint);
}

bool hubos_system_reset_device(hubos_system_t *system) {
  return hubos_device_server_endpoint_reset(&system->device_server_endpoint);
}

bool hubos_system_quarantine_device(hubos_system_t *system) {
  return hubos_device_server_endpoint_quarantine(&system->device_server_endpoint);
}

bool hubos_system_clear_device_quarantine(hubos_system_t *system) {
  return hubos_device_server_endpoint_clear_quarantine(&system->device_server_endpoint);
}

bool hubos_system_attach_device_mmio(hubos_system_t *system, hubos_id_t owner_session_id) {
  return hubos_device_server_endpoint_attach_mmio(&system->device_server_endpoint,
                                                  owner_session_id);
}

bool hubos_system_attach_device_irq(hubos_system_t *system, hubos_id_t owner_session_id) {
  return hubos_device_server_endpoint_attach_irq(&system->device_server_endpoint,
                                                 owner_session_id);
}

bool hubos_system_attach_device_dma(hubos_system_t *system, hubos_id_t owner_session_id) {
  return hubos_device_server_endpoint_attach_dma(&system->device_server_endpoint,
                                                 owner_session_id);
}

bool hubos_system_describe_device(const hubos_system_t *system,
                                  hubos_service_descriptor_t *out_descriptor) {
  if (system == NULL || out_descriptor == NULL) {
    return false;
  }

  out_descriptor->resource_id = system->device_server.resource_id;
  out_descriptor->name = system->device_server.name;
  out_descriptor->name_len = system->device_server.name != NULL ?
                               strlen(system->device_server.name) :
                               0;
  out_descriptor->resource_state = system->device_server.quarantined ?
                                     HUBOS_RESOURCE_QUARANTINED :
                                     (hubos_device_server_is_active(&system->device_server) ?
                                        HUBOS_RESOURCE_READY :
                                        HUBOS_RESOURCE_DISCOVERED);
  out_descriptor->endpoint = system->device_server.name;
  out_descriptor->version = NULL;
  out_descriptor->policy_hints =
    (system->device_server.mmio_attached ? 1u : 0u) |
    (system->device_server.irq_attached ? 2u : 0u) |
    (system->device_server.dma_attached ? 4u : 0u) |
    (system->device_server.quarantined ? 8u : 0u);
  return true;
}

void hubos_system_set_device_hardware_backend(hubos_system_t *system,
                                              const hubos_device_server_ops_t *ops,
                                              void *ops_context) {
  if (system == NULL) {
    return;
  }

  hubos_device_server_set_ops(&system->device_server, ops, ops_context);
}

void hubos_system_set_bus_hardware_backend(hubos_system_t *system,
                                           hubos_bus_kind_t kind,
                                           const hubos_bus_manager_ops_t *ops,
                                           void *ops_context) {
  hubos_bus_manager_t *manager = NULL;

  if (system == NULL) {
    return;
  }

  switch (kind) {
  case HUBOS_BUS_PCIE:
    manager = &system->pcie_bus_manager;
    break;
  case HUBOS_BUS_USB:
    manager = &system->usb_bus_manager;
    break;
  case HUBOS_BUS_I2C:
    manager = &system->i2c_bus_manager;
    break;
  case HUBOS_BUS_SPI:
    manager = &system->spi_bus_manager;
    break;
  }

  hubos_bus_manager_set_ops(manager, ops, ops_context);
}

bool hubos_system_bus_discover(hubos_system_t *system,
                               hubos_bus_kind_t kind,
                               const char *resource_name,
                               size_t resource_name_len,
                               hubos_resource_state_t state,
                               hubos_id_t *out_resource_id) {
  switch (kind) {
  case HUBOS_BUS_PCIE:
    return hubos_bus_manager_discover(&system->pcie_bus_manager,
                                      resource_name,
                                      resource_name_len,
                                      state,
                                      out_resource_id);
  case HUBOS_BUS_USB:
    return hubos_bus_manager_discover(&system->usb_bus_manager,
                                      resource_name,
                                      resource_name_len,
                                      state,
                                      out_resource_id);
  case HUBOS_BUS_I2C:
    return hubos_bus_manager_discover(&system->i2c_bus_manager,
                                      resource_name,
                                      resource_name_len,
                                      state,
                                      out_resource_id);
  case HUBOS_BUS_SPI:
    return hubos_bus_manager_discover(&system->spi_bus_manager,
                                      resource_name,
                                      resource_name_len,
                                      state,
                                      out_resource_id);
  }

  return false;
}
