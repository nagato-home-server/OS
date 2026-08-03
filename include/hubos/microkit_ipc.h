#ifndef HUBOS_MICROKIT_IPC_H
#define HUBOS_MICROKIT_IPC_H

#include "hubos/ipc.h"
#include "hubos/linux_vm_layout.h"
#include "hubos/microkit_graph.h"
#include "hubos/service_endpoints.h"
#include "hubos/vm_server.h"

typedef struct hubos_system hubos_system_t;

typedef struct {
  hubos_microkit_component_kind_t component;
  unsigned badge;
  const char *name;
  bool exposed;
} hubos_microkit_endpoint_binding_t;

typedef struct {
  const hubos_microkit_endpoint_binding_t *bindings;
  size_t binding_count;
} hubos_microkit_ipc_layout_t;

typedef enum {
  HUBOS_MICROKIT_RESOURCE_OP_REGISTER = 1,
  HUBOS_MICROKIT_RESOURCE_OP_UPDATE_STATE,
  HUBOS_MICROKIT_RESOURCE_OP_QUARANTINE,
  HUBOS_MICROKIT_RESOURCE_OP_RETIRE,
  HUBOS_MICROKIT_RESOURCE_OP_DESCRIBE,
} hubos_microkit_resource_operation_t;

typedef enum {
  HUBOS_MICROKIT_CAPABILITY_OP_ISSUE = 1,
  HUBOS_MICROKIT_CAPABILITY_OP_COPY,
  HUBOS_MICROKIT_CAPABILITY_OP_MINT_FROM,
  HUBOS_MICROKIT_CAPABILITY_OP_TRANSFER,
  HUBOS_MICROKIT_CAPABILITY_OP_REVOKE,
  HUBOS_MICROKIT_CAPABILITY_OP_AUTHORIZE,
} hubos_microkit_capability_operation_t;

typedef enum {
  HUBOS_MICROKIT_SESSION_OP_CREATE = 1,
  HUBOS_MICROKIT_SESSION_OP_REFRESH_CONTEXT,
  HUBOS_MICROKIT_SESSION_OP_SET_STATE,
  HUBOS_MICROKIT_SESSION_OP_IS_ANCESTOR,
  HUBOS_MICROKIT_SESSION_OP_CHILD_COUNT,
  HUBOS_MICROKIT_SESSION_OP_REVOKE_TREE,
} hubos_microkit_session_operation_t;

typedef enum {
  HUBOS_MICROKIT_HUB_OP_RESOLVE = 1,
  HUBOS_MICROKIT_HUB_OP_AUTHORIZE,
} hubos_microkit_hub_operation_t;

typedef enum {
  HUBOS_MICROKIT_DRIVER_OP_BIND = 1,
  HUBOS_MICROKIT_DRIVER_OP_REBIND,
  HUBOS_MICROKIT_DRIVER_OP_QUARANTINE,
  HUBOS_MICROKIT_DRIVER_OP_UNBIND,
} hubos_microkit_driver_operation_t;

typedef enum {
  HUBOS_MICROKIT_NETWORK_OP_BIND_NAMESPACE = 1,
  HUBOS_MICROKIT_NETWORK_OP_SET_POLICY,
  HUBOS_MICROKIT_NETWORK_OP_ADD_ROUTE,
  HUBOS_MICROKIT_NETWORK_OP_SET_DEFAULT_ROUTE,
  HUBOS_MICROKIT_NETWORK_OP_SELECT_NIC,
  HUBOS_MICROKIT_NETWORK_OP_BIND_PORT,
  HUBOS_MICROKIT_NETWORK_OP_SET_FAILOVER_POLICY,
  HUBOS_MICROKIT_NETWORK_OP_DESCRIBE,
} hubos_microkit_network_operation_t;

typedef enum {
  HUBOS_MICROKIT_STORAGE_OP_BIND_NAMESPACE = 1,
  HUBOS_MICROKIT_STORAGE_OP_RELEASE_NAMESPACE,
  HUBOS_MICROKIT_STORAGE_OP_FINALIZE_NAMESPACE,
  HUBOS_MICROKIT_STORAGE_OP_DESCRIBE,
} hubos_microkit_storage_operation_t;

typedef enum {
  HUBOS_MICROKIT_DISPLAY_OP_BIND_NAMESPACE = 1,
  HUBOS_MICROKIT_DISPLAY_OP_RELEASE_NAMESPACE,
  HUBOS_MICROKIT_DISPLAY_OP_FINALIZE_NAMESPACE,
  HUBOS_MICROKIT_DISPLAY_OP_DESCRIBE,
} hubos_microkit_display_operation_t;

typedef enum {
  HUBOS_MICROKIT_DEVICE_OP_SET_OWNER = 1,
  HUBOS_MICROKIT_DEVICE_OP_RELEASE_OWNER,
  HUBOS_MICROKIT_DEVICE_OP_RESET,
  HUBOS_MICROKIT_DEVICE_OP_QUARANTINE,
  HUBOS_MICROKIT_DEVICE_OP_CLEAR_QUARANTINE,
  HUBOS_MICROKIT_DEVICE_OP_ATTACH_MMIO,
  HUBOS_MICROKIT_DEVICE_OP_ATTACH_IRQ,
  HUBOS_MICROKIT_DEVICE_OP_ATTACH_DMA,
  HUBOS_MICROKIT_DEVICE_OP_DESCRIBE,
} hubos_microkit_device_operation_t;

typedef enum {
  HUBOS_MICROKIT_VM_OP_SET_GUEST_MEMORY = 1,
  HUBOS_MICROKIT_VM_OP_SET_VCPU_COUNT,
  HUBOS_MICROKIT_VM_OP_ATTACH_VIRTIO_NET,
  HUBOS_MICROKIT_VM_OP_ATTACH_VIRTIO_BLK,
  HUBOS_MICROKIT_VM_OP_ATTACH_VGPU,
  HUBOS_MICROKIT_VM_OP_SELECT_RUNTIME_PROFILE,
  HUBOS_MICROKIT_VM_OP_SET_ARTIFACTS,
  HUBOS_MICROKIT_VM_OP_SET_RESTART_POLICY,
  HUBOS_MICROKIT_VM_OP_START,
  HUBOS_MICROKIT_VM_OP_COMPLETE_BOOT,
  HUBOS_MICROKIT_VM_OP_FAIL,
  HUBOS_MICROKIT_VM_OP_STOP,
  HUBOS_MICROKIT_VM_OP_DESCRIBE,
} hubos_microkit_vm_operation_t;

typedef struct {
  const char *name;
  size_t name_len;
  hubos_resource_state_t state;
} hubos_microkit_resource_register_request_t;

typedef struct {
  hubos_id_t resource_id;
  hubos_resource_state_t state;
} hubos_microkit_resource_update_state_request_t;

typedef struct {
  hubos_id_t owner_session_id;
  hubos_id_t resource_id;
  unsigned rights;
  bool delegatable;
} hubos_microkit_capability_issue_request_t;

typedef struct {
  hubos_id_t source_capability_id;
  hubos_id_t owner_session_id;
} hubos_microkit_capability_copy_request_t;

typedef struct {
  hubos_id_t source_capability_id;
  hubos_id_t owner_session_id;
  unsigned rights;
  bool delegatable;
} hubos_microkit_capability_mint_request_t;

typedef struct {
  hubos_id_t capability_id;
  hubos_id_t new_owner_session_id;
} hubos_microkit_capability_transfer_request_t;

typedef struct {
  hubos_id_t capability_id;
  hubos_id_t resource_id;
  unsigned required_rights;
} hubos_microkit_capability_authorize_request_t;

typedef struct {
  hubos_id_t owner_id;
  hubos_id_t parent_id;
  hubos_session_type_t type;
} hubos_microkit_session_create_request_t;

typedef struct {
  hubos_id_t session_id;
  hubos_id_t namespace_view_version;
  hubos_id_t policy_context_version;
} hubos_microkit_session_refresh_context_request_t;

typedef struct {
  hubos_id_t session_id;
  hubos_session_state_t state;
} hubos_microkit_session_set_state_request_t;

typedef struct {
  hubos_id_t ancestor_id;
  hubos_id_t session_id;
} hubos_microkit_session_is_ancestor_request_t;

typedef struct {
  hubos_id_t session_id;
} hubos_microkit_session_revoke_tree_request_t;

typedef struct {
  hubos_id_t session_id;
} hubos_microkit_session_child_count_request_t;

typedef struct {
  hubos_id_t resource_id;
} hubos_microkit_resource_describe_request_t;

typedef struct {
  const char *name;
  size_t name_len;
} hubos_microkit_hub_resolve_request_t;

typedef struct {
  hubos_id_t capability_id;
  hubos_id_t resource_id;
  unsigned required_rights;
} hubos_microkit_hub_authorize_request_t;

typedef struct {
  hubos_id_t resource_id;
  hubos_id_t driver_id;
  hubos_driver_package_t package;
} hubos_microkit_driver_bind_request_t;

typedef struct {
  hubos_id_t resource_id;
  hubos_id_t driver_id;
  hubos_driver_package_t package;
} hubos_microkit_driver_rebind_request_t;

typedef struct {
  hubos_id_t resource_id;
} hubos_microkit_driver_quarantine_request_t;

typedef struct {
  hubos_id_t resource_id;
} hubos_microkit_driver_unbind_request_t;

typedef struct {
  hubos_id_t capability_id;
} hubos_microkit_capability_revoke_request_t;

typedef struct {
  hubos_namespace_handle_t namespace_handle;
} hubos_microkit_network_bind_namespace_request_t;

typedef struct {
  bool routing_enabled;
  bool firewall_enabled;
} hubos_microkit_network_set_policy_request_t;

typedef struct {
  const char *destination;
  size_t destination_len;
  hubos_id_t nic_resource_id;
  unsigned metric;
} hubos_microkit_network_add_route_request_t;

typedef struct {
  hubos_id_t nic_resource_id;
} hubos_microkit_network_set_default_route_request_t;

typedef struct {
  const char *destination;
  size_t destination_len;
} hubos_microkit_network_select_nic_request_t;

typedef struct {
  unsigned port;
  hubos_id_t nic_resource_id;
  hubos_id_t session_id;
} hubos_microkit_network_bind_port_request_t;

typedef struct {
  bool failover_enabled;
  hubos_id_t preferred_nic_resource_id;
} hubos_microkit_network_set_failover_policy_request_t;

typedef struct {
  hubos_namespace_handle_t namespace_handle;
} hubos_microkit_storage_bind_namespace_request_t;

typedef struct {
  hubos_microkit_storage_operation_t operation;
} hubos_microkit_storage_request_t;

typedef struct {
  hubos_namespace_handle_t namespace_handle;
} hubos_microkit_display_bind_namespace_request_t;

typedef struct {
  hubos_microkit_display_operation_t operation;
} hubos_microkit_display_request_t;

typedef struct {
  hubos_id_t owner_session_id;
} hubos_microkit_device_owner_request_t;

typedef struct {
  hubos_microkit_device_operation_t operation;
} hubos_microkit_device_request_t;

typedef struct {
  hubos_id_t guest_memory_id;
} hubos_microkit_vm_guest_memory_request_t;

typedef struct {
  unsigned vcpu_count;
} hubos_microkit_vm_vcpu_count_request_t;

typedef struct {
  hubos_id_t session_id;
} hubos_microkit_vm_session_request_t;

typedef struct {
  const char *runtime_profile_id;
  size_t runtime_profile_id_len;
} hubos_microkit_vm_select_runtime_profile_request_t;

typedef struct {
  hubos_linux_vm_artifacts_t artifacts;
} hubos_microkit_vm_set_artifacts_request_t;

typedef struct {
  hubos_vm_restart_policy_t policy;
  unsigned max_restart_attempts;
} hubos_microkit_vm_set_restart_policy_request_t;

typedef struct {
  unsigned failure_code;
} hubos_microkit_vm_fail_request_t;

typedef struct {
  hubos_microkit_vm_operation_t operation;
} hubos_microkit_vm_request_t;

typedef struct {
  hubos_microkit_component_kind_t service;
  unsigned operation;
  union {
    hubos_root_task_boot_step_request_t boot_step;
    hubos_microkit_resource_register_request_t resource_register;
    hubos_microkit_resource_update_state_request_t resource_update_state;

    hubos_microkit_capability_issue_request_t capability_issue;
    hubos_microkit_capability_copy_request_t capability_copy;
    hubos_microkit_capability_mint_request_t capability_mint;
    hubos_microkit_capability_transfer_request_t capability_transfer;
    hubos_microkit_capability_revoke_request_t capability_revoke;
    hubos_microkit_capability_authorize_request_t capability_authorize;

    hubos_microkit_session_create_request_t session_create;
    hubos_microkit_session_refresh_context_request_t session_refresh_context;
    hubos_microkit_session_set_state_request_t session_set_state;
    hubos_microkit_session_is_ancestor_request_t session_is_ancestor;
    hubos_microkit_session_child_count_request_t session_child_count;
    hubos_microkit_session_revoke_tree_request_t session_revoke_tree;

    hubos_microkit_resource_describe_request_t resource_describe;

    hubos_microkit_hub_resolve_request_t hub_resolve;
    hubos_microkit_hub_authorize_request_t hub_authorize;

    hubos_microkit_driver_bind_request_t driver_bind;
    hubos_microkit_driver_rebind_request_t driver_rebind;
    hubos_microkit_driver_quarantine_request_t driver_quarantine;
    hubos_microkit_driver_unbind_request_t driver_unbind;

    hubos_microkit_network_bind_namespace_request_t network_bind_namespace;
    hubos_microkit_network_set_policy_request_t network_set_policy;
    hubos_microkit_network_add_route_request_t network_add_route;
    hubos_microkit_network_set_default_route_request_t network_set_default_route;
    hubos_microkit_network_select_nic_request_t network_select_nic;
    hubos_microkit_network_bind_port_request_t network_bind_port;
    hubos_microkit_network_set_failover_policy_request_t network_set_failover_policy;
    hubos_microkit_storage_bind_namespace_request_t storage_bind_namespace;
    hubos_microkit_storage_request_t storage;
    hubos_microkit_display_bind_namespace_request_t display_bind_namespace;
    hubos_microkit_display_request_t display;
    hubos_microkit_device_owner_request_t device_owner;
    hubos_microkit_device_request_t device;
    hubos_microkit_vm_guest_memory_request_t vm_guest_memory;
    hubos_microkit_vm_vcpu_count_request_t vm_vcpu_count;
    hubos_microkit_vm_session_request_t vm_session;
    hubos_microkit_vm_select_runtime_profile_request_t vm_select_runtime_profile;
    hubos_microkit_vm_set_artifacts_request_t vm_set_artifacts;
    hubos_microkit_vm_set_restart_policy_request_t vm_set_restart_policy;
    hubos_microkit_vm_fail_request_t vm_fail;
    hubos_microkit_vm_request_t vm;
  } payload;
} hubos_microkit_ipc_request_t;

typedef struct {
  hubos_ipc_status_t status;
  hubos_id_t resource_id;
  hubos_id_t capability_id;
  hubos_id_t session_id;
  hubos_id_t driver_id;
  hubos_boot_step_t boot_step;
  size_t count;
  bool is_new;
  bool bool_result;
  hubos_service_descriptor_t descriptor;
} hubos_microkit_ipc_response_t;

void hubos_microkit_ipc_request_init(hubos_microkit_ipc_request_t *request,
                                     hubos_microkit_component_kind_t service,
                                     unsigned operation);
void hubos_microkit_ipc_response_init(hubos_microkit_ipc_response_t *response);

void hubos_microkit_ipc_layout_init(hubos_microkit_ipc_layout_t *layout);
const hubos_microkit_endpoint_binding_t *hubos_microkit_ipc_layout_get(
  const hubos_microkit_ipc_layout_t *layout,
  hubos_microkit_component_kind_t component);
const hubos_microkit_endpoint_binding_t *hubos_microkit_ipc_layout_get_by_badge(
  const hubos_microkit_ipc_layout_t *layout,
  unsigned badge);
bool hubos_microkit_ipc_layout_validate(const hubos_microkit_ipc_layout_t *layout,
                                        const hubos_microkit_graph_t *graph);

bool hubos_microkit_ipc_dispatch(hubos_system_t *system,
                                 const hubos_microkit_ipc_request_t *request,
                                 hubos_microkit_ipc_response_t *response);

#endif
