#include "hubos/system.h"
#include "hubos/runtime_config.h"

#include <string.h>

static const hubos_microkit_endpoint_binding_t hubos_microkit_endpoint_bindings[] = {
  { HUBOS_MICROKIT_COMPONENT_ROOT_TASK, 0, "Root Task", true },
  { HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY, 1, "Resource Registry", true },
  { HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER, 2, "Capability Manager", true },
  { HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER, 3, "Session Manager", true },
  { HUBOS_MICROKIT_COMPONENT_HUB, 4, "Hub", true },
  { HUBOS_MICROKIT_COMPONENT_DRIVER_SERVICE, 5, "Driver Service", true },
  { HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER, 6, "Network Server", true },
  { HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER, 12, "Device Server", true },
  { HUBOS_MICROKIT_COMPONENT_STORAGE_SERVER, 13, "Storage Server", true },
  { HUBOS_MICROKIT_COMPONENT_DISPLAY_SERVER, 14, "Display Server", true },
  { HUBOS_MICROKIT_COMPONENT_VM_SERVER, 15, "VM Server", true },
};

void hubos_microkit_ipc_request_init(hubos_microkit_ipc_request_t *request,
                                     hubos_microkit_component_kind_t service,
                                     unsigned operation) {
  if (request == NULL) {
    return;
  }

  memset(request, 0, sizeof(*request));
  request->service = service;
  request->operation = operation;
}

void hubos_microkit_ipc_response_init(hubos_microkit_ipc_response_t *response) {
  if (response == NULL) {
    return;
  }

  memset(response, 0, sizeof(*response));
  response->status = HUBOS_IPC_STATUS_INVALID_ARGUMENT;
  response->resource_id = HUBOS_ID_INVALID;
  response->capability_id = HUBOS_ID_INVALID;
  response->session_id = HUBOS_ID_INVALID;
  response->driver_id = HUBOS_ID_INVALID;
  response->boot_step = HUBOS_BOOT_STEP_COUNT;
  response->descriptor.resource_id = HUBOS_ID_INVALID;
  response->descriptor.name = NULL;
  response->descriptor.name_len = 0;
  response->descriptor.resource_state = HUBOS_RESOURCE_DISCOVERED;
  response->descriptor.endpoint = NULL;
  response->descriptor.version = NULL;
  response->descriptor.policy_hints = 0;
}

void hubos_microkit_ipc_layout_init(hubos_microkit_ipc_layout_t *layout) {
  if (layout == NULL) {
    return;
  }

  layout->bindings = hubos_microkit_endpoint_bindings;
  layout->binding_count = sizeof(hubos_microkit_endpoint_bindings) /
                          sizeof(hubos_microkit_endpoint_bindings[0]);
}

const hubos_microkit_endpoint_binding_t *hubos_microkit_ipc_layout_get(
  const hubos_microkit_ipc_layout_t *layout,
  hubos_microkit_component_kind_t component) {
  if (layout == NULL || layout->bindings == NULL) {
    return NULL;
  }

  for (size_t index = 0; index < layout->binding_count; ++index) {
    if (layout->bindings[index].component == component) {
      return &layout->bindings[index];
    }
  }

  return NULL;
}

const hubos_microkit_endpoint_binding_t *hubos_microkit_ipc_layout_get_by_badge(
  const hubos_microkit_ipc_layout_t *layout,
  unsigned badge) {
  if (layout == NULL || layout->bindings == NULL) {
    return NULL;
  }

  for (size_t index = 0; index < layout->binding_count; ++index) {
    if (layout->bindings[index].badge == badge) {
      return &layout->bindings[index];
    }
  }

  return NULL;
}

bool hubos_microkit_ipc_layout_validate(const hubos_microkit_ipc_layout_t *layout,
                                        const hubos_microkit_graph_t *graph) {
  if (layout == NULL || graph == NULL || layout->bindings == NULL ||
      layout->binding_count == 0 || !hubos_microkit_graph_validate(graph)) {
    return false;
  }

  for (size_t index = 0; index < layout->binding_count; ++index) {
    const hubos_microkit_endpoint_binding_t *binding = &layout->bindings[index];
    if (hubos_microkit_graph_get(graph, binding->component) == NULL) {
      return false;
    }
    if (binding->name == NULL || binding->name[0] == '\0') {
      return false;
    }
    if (!binding->exposed) {
      return false;
    }
    for (size_t other_index = index + 1; other_index < layout->binding_count; ++other_index) {
      if (layout->bindings[other_index].badge == binding->badge) {
        return false;
      }
    }
  }

  return true;
}

static void hubos_microkit_ipc_set_not_supported(hubos_microkit_ipc_response_t *response) {
  if (response != NULL) {
    response->status = HUBOS_IPC_STATUS_DENIED;
  }
}

static void hubos_microkit_ipc_set_ok(hubos_microkit_ipc_response_t *response) {
  if (response != NULL) {
    response->status = HUBOS_IPC_STATUS_OK;
  }
}

static bool hubos_microkit_ipc_dispatch_resource_registry(
  hubos_system_t *system,
  const hubos_microkit_ipc_request_t *request,
  hubos_microkit_ipc_response_t *response) {
  switch ((hubos_microkit_resource_operation_t)request->operation) {
  case HUBOS_MICROKIT_RESOURCE_OP_REGISTER:
    if (hubos_system_register_resource(system,
                                       request->payload.resource_register.name,
                                       request->payload.resource_register.name_len,
                                       request->payload.resource_register.state,
                                       &response->resource_id,
                                       &response->is_new)) {
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  case HUBOS_MICROKIT_RESOURCE_OP_UPDATE_STATE:
    if (hubos_resource_registry_endpoint_update_state(&system->resource_registry_endpoint,
                                                      request->payload.resource_update_state.resource_id,
                                                      request->payload.resource_update_state.state)) {
      hubos_microkit_ipc_set_ok(response);
      response->resource_id = request->payload.resource_update_state.resource_id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_RESOURCE_OP_QUARANTINE:
    if (hubos_system_quarantine_resource(system,
                                         request->payload.resource_update_state.resource_id)) {
      hubos_microkit_ipc_set_ok(response);
      response->resource_id = request->payload.resource_update_state.resource_id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_RESOURCE_OP_RETIRE:
    if (hubos_system_retire_resource(system, request->payload.resource_update_state.resource_id)) {
      hubos_microkit_ipc_set_ok(response);
      response->resource_id = request->payload.resource_update_state.resource_id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_RESOURCE_OP_DESCRIBE: {
    const hubos_resource_t *resource = hubos_resource_registry_endpoint_get(
      &system->resource_registry_endpoint,
      request->payload.resource_describe.resource_id);
    if (resource != NULL) {
      response->descriptor.resource_id = resource->id;
      response->descriptor.name = resource->name;
      response->descriptor.name_len = resource->name_len;
      response->descriptor.resource_state = resource->state;
      response->descriptor.endpoint = resource->name;
      response->descriptor.version = NULL;
      response->descriptor.policy_hints = 0;
      response->resource_id = resource->id;
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  }
  }

  return false;
}

static bool hubos_microkit_ipc_dispatch_capability(
  hubos_system_t *system,
  const hubos_microkit_ipc_request_t *request,
  hubos_microkit_ipc_response_t *response) {
  switch ((hubos_microkit_capability_operation_t)request->operation) {
  case HUBOS_MICROKIT_CAPABILITY_OP_ISSUE:
    if (hubos_system_issue_capability(system,
                                      request->payload.capability_issue.owner_session_id,
                                      request->payload.capability_issue.resource_id,
                                      request->payload.capability_issue.rights,
                                      request->payload.capability_issue.delegatable,
                                      &response->capability_id)) {
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  case HUBOS_MICROKIT_CAPABILITY_OP_COPY:
    if (hubos_system_copy_capability(system,
                                     request->payload.capability_copy.source_capability_id,
                                     request->payload.capability_copy.owner_session_id,
                                     &response->capability_id)) {
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  case HUBOS_MICROKIT_CAPABILITY_OP_MINT_FROM:
    if (hubos_system_mint_capability(system,
                                     request->payload.capability_mint.source_capability_id,
                                     request->payload.capability_mint.owner_session_id,
                                     request->payload.capability_mint.rights,
                                     request->payload.capability_mint.delegatable,
                                     &response->capability_id)) {
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  case HUBOS_MICROKIT_CAPABILITY_OP_TRANSFER:
    if (hubos_system_transfer_capability(system,
                                         request->payload.capability_transfer.capability_id,
                                         request->payload.capability_transfer.new_owner_session_id)) {
      hubos_microkit_ipc_set_ok(response);
      response->capability_id = request->payload.capability_transfer.capability_id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_CAPABILITY_OP_REVOKE:
    if (hubos_system_revoke_capability(system, request->payload.capability_revoke.capability_id)) {
      hubos_microkit_ipc_set_ok(response);
      response->capability_id = request->payload.capability_revoke.capability_id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_CAPABILITY_OP_AUTHORIZE:
    response->bool_result = hubos_system_authorize(system,
                                                   request->payload.capability_authorize.capability_id,
                                                   request->payload.capability_authorize.resource_id,
                                                   request->payload.capability_authorize.required_rights);
    if (response->bool_result) {
      hubos_microkit_ipc_set_ok(response);
    } else {
      response->status = HUBOS_IPC_STATUS_DENIED;
    }
    return true;
  }

  return false;
}

static bool hubos_microkit_ipc_dispatch_session(
  hubos_system_t *system,
  const hubos_microkit_ipc_request_t *request,
  hubos_microkit_ipc_response_t *response) {
  switch ((hubos_microkit_session_operation_t)request->operation) {
  case HUBOS_MICROKIT_SESSION_OP_CREATE:
    if (hubos_system_create_session(system,
                                    request->payload.session_create.owner_id,
                                    request->payload.session_create.parent_id,
                                    request->payload.session_create.type,
                                    &response->session_id)) {
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  case HUBOS_MICROKIT_SESSION_OP_REFRESH_CONTEXT:
    if (hubos_system_refresh_session_context(system,
                                             request->payload.session_refresh_context.session_id,
                                             request->payload.session_refresh_context.namespace_view_version,
                                             request->payload.session_refresh_context.policy_context_version)) {
      hubos_microkit_ipc_set_ok(response);
      response->session_id = request->payload.session_refresh_context.session_id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_SESSION_OP_SET_STATE:
    if (hubos_session_manager_endpoint_set_state(&system->session_manager_endpoint,
                                                 request->payload.session_set_state.session_id,
                                                 request->payload.session_set_state.state)) {
      hubos_microkit_ipc_set_ok(response);
      response->session_id = request->payload.session_set_state.session_id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_SESSION_OP_IS_ANCESTOR:
    response->bool_result = hubos_session_manager_endpoint_is_ancestor(
      &system->session_manager_endpoint,
      request->payload.session_is_ancestor.ancestor_id,
      request->payload.session_is_ancestor.session_id);
    if (response->bool_result) {
      hubos_microkit_ipc_set_ok(response);
    } else {
      response->status = HUBOS_IPC_STATUS_NOT_FOUND;
    }
    return true;
  case HUBOS_MICROKIT_SESSION_OP_CHILD_COUNT:
    response->count = hubos_session_manager_endpoint_child_count(
      &system->session_manager_endpoint,
      request->payload.session_child_count.session_id);
    hubos_microkit_ipc_set_ok(response);
    return true;
  case HUBOS_MICROKIT_SESSION_OP_REVOKE_TREE:
    if (hubos_system_revoke_session_tree(system, request->payload.session_revoke_tree.session_id)) {
      hubos_microkit_ipc_set_ok(response);
      response->session_id = request->payload.session_revoke_tree.session_id;
      return true;
    }
    break;
  }

  return false;
}

static bool hubos_microkit_ipc_dispatch_hub(hubos_system_t *system,
                                            const hubos_microkit_ipc_request_t *request,
                                            hubos_microkit_ipc_response_t *response) {
  switch ((hubos_microkit_hub_operation_t)request->operation) {
  case HUBOS_MICROKIT_HUB_OP_RESOLVE:
    if (hubos_system_resolve(system,
                             request->payload.hub_resolve.name,
                             request->payload.hub_resolve.name_len,
                             &response->descriptor)) {
      response->resource_id = response->descriptor.resource_id;
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  case HUBOS_MICROKIT_HUB_OP_AUTHORIZE:
    response->bool_result = hubos_system_authorize(system,
                                                   request->payload.hub_authorize.capability_id,
                                                   request->payload.hub_authorize.resource_id,
                                                   request->payload.hub_authorize.required_rights);
    if (response->bool_result) {
      hubos_microkit_ipc_set_ok(response);
    } else {
      response->status = HUBOS_IPC_STATUS_DENIED;
    }
    return true;
  }

  return false;
}

static bool hubos_microkit_ipc_dispatch_driver(
  hubos_system_t *system,
  const hubos_microkit_ipc_request_t *request,
  hubos_microkit_ipc_response_t *response) {
  switch ((hubos_microkit_driver_operation_t)request->operation) {
  case HUBOS_MICROKIT_DRIVER_OP_BIND:
    if (hubos_system_bind_driver(system,
                                 request->payload.driver_bind.resource_id,
                                 request->payload.driver_bind.driver_id,
                                 &request->payload.driver_bind.package)) {
      hubos_microkit_ipc_set_ok(response);
      response->resource_id = request->payload.driver_bind.resource_id;
      response->driver_id = request->payload.driver_bind.driver_id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_DRIVER_OP_REBIND:
    if (hubos_system_rebind_driver(system,
                                   request->payload.driver_rebind.resource_id,
                                   request->payload.driver_rebind.driver_id,
                                   &request->payload.driver_rebind.package)) {
      hubos_microkit_ipc_set_ok(response);
      response->resource_id = request->payload.driver_rebind.resource_id;
      response->driver_id = request->payload.driver_rebind.driver_id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_DRIVER_OP_QUARANTINE:
    if (hubos_system_quarantine_driver(system, request->payload.driver_quarantine.resource_id)) {
      hubos_microkit_ipc_set_ok(response);
      response->resource_id = request->payload.driver_quarantine.resource_id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_DRIVER_OP_UNBIND:
    if (hubos_system_unbind_driver(system, request->payload.driver_unbind.resource_id)) {
      hubos_microkit_ipc_set_ok(response);
      response->resource_id = request->payload.driver_unbind.resource_id;
      return true;
    }
    break;
  }

  return false;
}

static bool hubos_microkit_ipc_dispatch_network(
  hubos_system_t *system,
  const hubos_microkit_ipc_request_t *request,
  hubos_microkit_ipc_response_t *response) {
  switch ((hubos_microkit_network_operation_t)request->operation) {
  case HUBOS_MICROKIT_NETWORK_OP_BIND_NAMESPACE:
    if (hubos_system_bind_network_namespace(system,
                                            request->payload.network_bind_namespace.namespace_handle)) {
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  case HUBOS_MICROKIT_NETWORK_OP_SET_POLICY:
    if (hubos_system_set_network_policy(system,
                                        request->payload.network_set_policy.routing_enabled,
                                        request->payload.network_set_policy.firewall_enabled)) {
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  case HUBOS_MICROKIT_NETWORK_OP_ADD_ROUTE:
    if (hubos_system_add_network_route(system,
                                       request->payload.network_add_route.destination,
                                       request->payload.network_add_route.nic_resource_id,
                                       request->payload.network_add_route.metric)) {
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  case HUBOS_MICROKIT_NETWORK_OP_SET_DEFAULT_ROUTE:
    if (hubos_system_set_network_default_route(system,
                                               request->payload.network_set_default_route.nic_resource_id)) {
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  case HUBOS_MICROKIT_NETWORK_OP_SELECT_NIC:
    if (hubos_system_select_network_nic(system,
                                        request->payload.network_select_nic.destination,
                                        request->payload.network_select_nic.destination_len,
                                        &response->resource_id)) {
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  case HUBOS_MICROKIT_NETWORK_OP_BIND_PORT:
    if (hubos_system_bind_network_port(system,
                                       request->payload.network_bind_port.port,
                                       request->payload.network_bind_port.nic_resource_id,
                                       request->payload.network_bind_port.session_id)) {
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  case HUBOS_MICROKIT_NETWORK_OP_SET_FAILOVER_POLICY:
    if (hubos_system_set_network_failover_policy(
          system,
          request->payload.network_set_failover_policy.failover_enabled,
          request->payload.network_set_failover_policy.preferred_nic_resource_id)) {
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  case HUBOS_MICROKIT_NETWORK_OP_DESCRIBE:
    if (hubos_system_describe_network_server(system, &response->descriptor)) {
      response->resource_id = response->descriptor.resource_id;
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  }

  return false;
}

static bool hubos_microkit_ipc_dispatch_storage(
  hubos_system_t *system,
  const hubos_microkit_ipc_request_t *request,
  hubos_microkit_ipc_response_t *response) {
  switch ((hubos_microkit_storage_operation_t)request->operation) {
  case HUBOS_MICROKIT_STORAGE_OP_BIND_NAMESPACE:
    if (hubos_system_bind_storage_namespace(system,
                                            request->payload.storage_bind_namespace.namespace_handle)) {
      hubos_microkit_ipc_set_ok(response);
      response->resource_id = request->payload.storage_bind_namespace.namespace_handle.id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_STORAGE_OP_RELEASE_NAMESPACE:
    if (hubos_system_release_storage_namespace(system)) {
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  case HUBOS_MICROKIT_STORAGE_OP_FINALIZE_NAMESPACE:
    if (hubos_system_finalize_storage_namespace(system)) {
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  case HUBOS_MICROKIT_STORAGE_OP_DESCRIBE:
    if (hubos_system_describe_storage_server(system, &response->descriptor)) {
      response->resource_id = response->descriptor.resource_id;
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  }

  return false;
}

static bool hubos_microkit_ipc_dispatch_display(
  hubos_system_t *system,
  const hubos_microkit_ipc_request_t *request,
  hubos_microkit_ipc_response_t *response) {
  switch ((hubos_microkit_display_operation_t)request->operation) {
  case HUBOS_MICROKIT_DISPLAY_OP_BIND_NAMESPACE:
    if (hubos_system_bind_display_namespace(system,
                                            request->payload.display_bind_namespace.namespace_handle)) {
      hubos_microkit_ipc_set_ok(response);
      response->resource_id = request->payload.display_bind_namespace.namespace_handle.id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_DISPLAY_OP_RELEASE_NAMESPACE:
    if (hubos_system_release_display_namespace(system)) {
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  case HUBOS_MICROKIT_DISPLAY_OP_FINALIZE_NAMESPACE:
    if (hubos_system_finalize_display_namespace(system)) {
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  case HUBOS_MICROKIT_DISPLAY_OP_DESCRIBE:
    if (hubos_system_describe_display_server(system, &response->descriptor)) {
      response->resource_id = response->descriptor.resource_id;
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  }

  return false;
}

static bool hubos_microkit_ipc_dispatch_device(
  hubos_system_t *system,
  const hubos_microkit_ipc_request_t *request,
  hubos_microkit_ipc_response_t *response) {
  switch ((hubos_microkit_device_operation_t)request->operation) {
  case HUBOS_MICROKIT_DEVICE_OP_SET_OWNER:
    if (hubos_system_set_device_owner(system,
                                      request->payload.device_owner.owner_session_id)) {
      hubos_microkit_ipc_set_ok(response);
      response->resource_id = request->payload.device_owner.owner_session_id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_DEVICE_OP_RELEASE_OWNER:
    if (hubos_system_release_device_owner(system)) {
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  case HUBOS_MICROKIT_DEVICE_OP_RESET:
    if (hubos_system_reset_device(system)) {
      hubos_microkit_ipc_set_ok(response);
      response->resource_id = system->device_server.resource_id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_DEVICE_OP_QUARANTINE:
    if (hubos_system_quarantine_device(system)) {
      hubos_microkit_ipc_set_ok(response);
      response->resource_id = system->device_server.resource_id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_DEVICE_OP_CLEAR_QUARANTINE:
    if (hubos_system_clear_device_quarantine(system)) {
      hubos_microkit_ipc_set_ok(response);
      response->resource_id = system->device_server.resource_id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_DEVICE_OP_ATTACH_MMIO:
    if (hubos_system_attach_device_mmio(system,
                                        request->payload.device_owner.owner_session_id)) {
      hubos_microkit_ipc_set_ok(response);
      response->resource_id = request->payload.device_owner.owner_session_id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_DEVICE_OP_ATTACH_IRQ:
    if (hubos_system_attach_device_irq(system,
                                       request->payload.device_owner.owner_session_id)) {
      hubos_microkit_ipc_set_ok(response);
      response->resource_id = request->payload.device_owner.owner_session_id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_DEVICE_OP_ATTACH_DMA:
    if (hubos_system_attach_device_dma(system,
                                       request->payload.device_owner.owner_session_id)) {
      hubos_microkit_ipc_set_ok(response);
      response->resource_id = request->payload.device_owner.owner_session_id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_DEVICE_OP_DESCRIBE:
    if (hubos_system_describe_device(system, &response->descriptor)) {
      response->resource_id = response->descriptor.resource_id;
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  }

  return false;
}

static bool hubos_microkit_ipc_dispatch_vm(
  hubos_system_t *system,
  const hubos_microkit_ipc_request_t *request,
  hubos_microkit_ipc_response_t *response) {
  switch ((hubos_microkit_vm_operation_t)request->operation) {
  case HUBOS_MICROKIT_VM_OP_SET_GUEST_MEMORY:
    if (hubos_system_set_vm_guest_memory(system,
                                         request->payload.vm_guest_memory.guest_memory_id)) {
      hubos_microkit_ipc_set_ok(response);
      response->resource_id = request->payload.vm_guest_memory.guest_memory_id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_VM_OP_SET_VCPU_COUNT:
    if (hubos_system_set_vm_vcpu_count(system, request->payload.vm_vcpu_count.vcpu_count)) {
      hubos_microkit_ipc_set_ok(response);
      response->count = request->payload.vm_vcpu_count.vcpu_count;
      return true;
    }
    break;
  case HUBOS_MICROKIT_VM_OP_ATTACH_VIRTIO_NET:
    if (hubos_system_attach_vm_virtio_net(system, request->payload.vm_session.session_id)) {
      hubos_microkit_ipc_set_ok(response);
      response->session_id = request->payload.vm_session.session_id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_VM_OP_ATTACH_VIRTIO_BLK:
    if (hubos_system_attach_vm_virtio_blk(system, request->payload.vm_session.session_id)) {
      hubos_microkit_ipc_set_ok(response);
      response->session_id = request->payload.vm_session.session_id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_VM_OP_ATTACH_VGPU:
    if (hubos_system_attach_vm_vgpu(system, request->payload.vm_session.session_id)) {
      hubos_microkit_ipc_set_ok(response);
      response->session_id = request->payload.vm_session.session_id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_VM_OP_SELECT_RUNTIME_PROFILE: {
    size_t profile_count = 0;
    const hubos_app_vm_runtime_profile_t *profiles = hubos_runtime_config_profiles(&profile_count);
    const hubos_app_vm_runtime_profile_t *profile =
      hubos_app_vm_runtime_catalog_find(profiles,
                                        profile_count,
                                        request->payload.vm_select_runtime_profile.runtime_profile_id);
    if (hubos_system_select_vm_runtime_profile(system, profile)) {
      hubos_microkit_ipc_set_ok(response);
      response->descriptor.version = profile->id;
      response->count = profile->resources.vcpus;
      return true;
    }
    break;
  }
  case HUBOS_MICROKIT_VM_OP_SET_ARTIFACTS:
    if (hubos_system_set_vm_artifacts(system, request->payload.vm_set_artifacts.artifacts)) {
      hubos_microkit_ipc_set_ok(response);
      response->descriptor.version = request->payload.vm_set_artifacts.artifacts.kernel_image;
      return true;
    }
    break;
  case HUBOS_MICROKIT_VM_OP_SET_RESTART_POLICY:
    if (hubos_system_set_vm_restart_policy(system,
                                           request->payload.vm_set_restart_policy.policy,
                                           request->payload.vm_set_restart_policy.max_restart_attempts)) {
      hubos_microkit_ipc_set_ok(response);
      response->count = request->payload.vm_set_restart_policy.max_restart_attempts;
      return true;
    }
    break;
  case HUBOS_MICROKIT_VM_OP_START:
    if (hubos_system_start_vm(system)) {
      hubos_microkit_ipc_set_ok(response);
      response->resource_id = system->vm_server.id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_VM_OP_COMPLETE_BOOT:
    if (hubos_system_complete_vm_boot(system)) {
      hubos_microkit_ipc_set_ok(response);
      response->resource_id = system->vm_server.id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_VM_OP_FAIL:
    if (hubos_system_fail_vm(system, request->payload.vm_fail.failure_code)) {
      hubos_microkit_ipc_set_ok(response);
      response->resource_id = system->vm_server.id;
      response->bool_result = system->vm_server.state == HUBOS_VM_BOOTING;
      return true;
    }
    break;
  case HUBOS_MICROKIT_VM_OP_STOP:
    if (hubos_system_stop_vm(system)) {
      hubos_microkit_ipc_set_ok(response);
      response->resource_id = system->vm_server.id;
      return true;
    }
    break;
  case HUBOS_MICROKIT_VM_OP_DESCRIBE:
    if (hubos_system_describe_vm(system, &response->descriptor)) {
      response->resource_id = response->descriptor.resource_id;
      hubos_microkit_ipc_set_ok(response);
      return true;
    }
    break;
  }

  return false;
}

bool hubos_microkit_ipc_dispatch(hubos_system_t *system,
                                 const hubos_microkit_ipc_request_t *request,
                                 hubos_microkit_ipc_response_t *response) {
  if (system == NULL || request == NULL || response == NULL) {
    return false;
  }

  hubos_microkit_ipc_response_init(response);

  switch (request->service) {
  case HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY:
    return hubos_microkit_ipc_dispatch_resource_registry(system, request, response);
  case HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER:
    return hubos_microkit_ipc_dispatch_capability(system, request, response);
  case HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER:
    return hubos_microkit_ipc_dispatch_session(system, request, response);
  case HUBOS_MICROKIT_COMPONENT_HUB:
    return hubos_microkit_ipc_dispatch_hub(system, request, response);
  case HUBOS_MICROKIT_COMPONENT_DRIVER_SERVICE:
    return hubos_microkit_ipc_dispatch_driver(system, request, response);
  case HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER:
    return hubos_microkit_ipc_dispatch_network(system, request, response);
  case HUBOS_MICROKIT_COMPONENT_STORAGE_SERVER:
    return hubos_microkit_ipc_dispatch_storage(system, request, response);
  case HUBOS_MICROKIT_COMPONENT_DISPLAY_SERVER:
    return hubos_microkit_ipc_dispatch_display(system, request, response);
  case HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER:
    return hubos_microkit_ipc_dispatch_device(system, request, response);
  case HUBOS_MICROKIT_COMPONENT_VM_SERVER:
    return hubos_microkit_ipc_dispatch_vm(system, request, response);
  default:
    hubos_microkit_ipc_set_not_supported(response);
    return false;
  }
}
