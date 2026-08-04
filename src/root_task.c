#include "hubos/root_task.h"

#include <string.h>

static const hubos_boot_step_t hubos_root_task_control_plane_steps[] = {
  HUBOS_BOOT_RESOURCE_REGISTRY,
  HUBOS_BOOT_SESSION_MANAGER,
  HUBOS_BOOT_CAPABILITY_MANAGER,
  HUBOS_BOOT_MEMORY_MANAGER,
  HUBOS_BOOT_DMA_MANAGER,
  HUBOS_BOOT_HUB,
  HUBOS_BOOT_DRIVER_REGISTRY,
  HUBOS_BOOT_BUS_MANAGERS,
  HUBOS_BOOT_DEVICE_DISCOVERY,
  HUBOS_BOOT_DRIVER_BINDING,
  HUBOS_BOOT_SYSTEM_SERVERS,
  HUBOS_BOOT_APP_MANAGERS,
  HUBOS_BOOT_APPS,
};

static const char *const hubos_root_task_hub_resource_name = "service://hub";
static const char *const hubos_root_task_registry_resource_name = "service://resource-registry";
static const char *const hubos_root_task_driver_registry_resource_name = "service://driver-registry";
static const char *const hubos_root_task_bootstrap_driver_package = "hubos.control-plane";
static const char *const hubos_root_task_bootstrap_driver_version = "bootstrap";

__attribute__((weak)) bool hubos_root_task_platform_init_vm(hubos_root_task_t *root_task) {
  (void)root_task;
  return true;
}

static bool hubos_root_task_register_resource(hubos_root_task_t *root_task,
                                              hubos_id_t *resource_id,
                                              const char *name) {
  if (root_task == NULL || root_task->system == NULL || resource_id == NULL || name == NULL) {
    return false;
  }

  if (*resource_id != HUBOS_ID_INVALID) {
    return true;
  }

  return hubos_system_register_resource(root_task->system,
                                        name,
                                        strlen(name),
                                        HUBOS_RESOURCE_READY,
                                        resource_id,
                                        NULL);
}

static bool hubos_root_task_seed_hub(hubos_root_task_t *root_task) {
  return hubos_root_task_register_resource(root_task,
                                           &root_task->hub_resource_id,
                                           hubos_root_task_hub_resource_name);
}

static bool hubos_root_task_seed_registry(hubos_root_task_t *root_task) {
  return hubos_root_task_register_resource(root_task,
                                           &root_task->resource_registry_resource_id,
                                           hubos_root_task_registry_resource_name);
}

static bool hubos_root_task_seed_root_session(hubos_root_task_t *root_task) {
  if (root_task == NULL || root_task->system == NULL) {
    return false;
  }

  if (root_task->root_session_id != HUBOS_ID_INVALID) {
    return true;
  }

  if (!hubos_system_create_session(root_task->system,
                                   HUBOS_ID_INVALID,
                                   HUBOS_ID_INVALID,
                                   HUBOS_SESSION_PERMANENT,
                                   &root_task->root_session_id)) {
    return false;
  }

  return hubos_session_manager_endpoint_set_state(&root_task->system->session_manager_endpoint,
                                                  root_task->root_session_id,
                                                  HUBOS_SESSION_ACTIVE);
}

static bool hubos_root_task_seed_root_capability(hubos_root_task_t *root_task) {
  if (root_task == NULL || root_task->system == NULL) {
    return false;
  }

  if (root_task->root_capability_id != HUBOS_ID_INVALID) {
    return true;
  }

  if (!hubos_root_task_seed_root_session(root_task) || !hubos_root_task_seed_registry(root_task)) {
    return false;
  }

  return hubos_system_issue_capability(root_task->system,
                                       root_task->root_session_id,
                                       root_task->resource_registry_resource_id,
                                       HUBOS_CAP_RIGHT_INSPECT,
                                       true,
                                       &root_task->root_capability_id);
}

static bool hubos_root_task_seed_driver_registry(hubos_root_task_t *root_task) {
  if (root_task == NULL || root_task->system == NULL) {
    return false;
  }

  if (!hubos_root_task_register_resource(root_task,
                                         &root_task->driver_registry_resource_id,
                                         hubos_root_task_driver_registry_resource_name)) {
    return false;
  }

  if (root_task->bootstrap_driver_id != HUBOS_ID_INVALID) {
    return true;
  }

  return hubos_system_register_driver(root_task->system,
                                      0,
                                      0,
                                      0,
                                      hubos_root_task_bootstrap_driver_package,
                                      hubos_root_task_bootstrap_driver_version,
                                      &root_task->bootstrap_driver_id,
                                      NULL);
}

static bool hubos_root_task_seed_control_plane(hubos_root_task_t *root_task) {
  return hubos_root_task_seed_hub(root_task) &&
         hubos_root_task_seed_registry(root_task) &&
         hubos_root_task_seed_root_session(root_task) &&
         hubos_root_task_seed_root_capability(root_task) &&
         hubos_root_task_seed_driver_registry(root_task);
}

static bool hubos_root_task_next_control_plane_step(const hubos_system_t *system,
                                                    hubos_boot_step_t *out_step) {
  if (system == NULL || out_step == NULL) {
    return false;
  }

  for (size_t index = 0; index < sizeof(hubos_root_task_control_plane_steps) /
                                   sizeof(hubos_root_task_control_plane_steps[0]);
       ++index) {
    hubos_boot_step_t step = hubos_root_task_control_plane_steps[index];
    if (!hubos_system_boot_step_is_complete(system, step)) {
      *out_step = step;
      return true;
    }
  }

  *out_step = HUBOS_BOOT_STEP_COUNT;
  return false;
}

void hubos_root_task_init(hubos_root_task_t *root_task, hubos_system_t *system) {
  if (root_task == NULL) {
    return;
  }

  root_task->system = system;
  root_task->bootstrapped = false;
  root_task->dormant = false;
  root_task->hub_resource_id = HUBOS_ID_INVALID;
  root_task->resource_registry_resource_id = HUBOS_ID_INVALID;
  root_task->root_session_id = HUBOS_ID_INVALID;
  root_task->root_capability_id = HUBOS_ID_INVALID;
  root_task->driver_registry_resource_id = HUBOS_ID_INVALID;
  root_task->bootstrap_driver_id = HUBOS_ID_INVALID;
}

bool hubos_root_task_bootstrap(hubos_root_task_t *root_task) {
  if (root_task == NULL || root_task->system == NULL || root_task->bootstrapped) {
    return false;
  }

  if (!hubos_system_complete_boot_step(root_task->system, HUBOS_BOOT_FIRMWARE) ||
      !hubos_system_complete_boot_step(root_task->system, HUBOS_BOOT_SEL4) ||
      !hubos_system_complete_boot_step(root_task->system, HUBOS_BOOT_ROOT_TASK)) {
    return false;
  }

  if (!hubos_root_task_seed_control_plane(root_task)) {
    return false;
  }

  if (!hubos_root_task_platform_init_vm(root_task)) {
    return false;
  }

  root_task->bootstrapped = true;
  root_task->dormant = true;
  return true;
}

bool hubos_root_task_is_dormant(const hubos_root_task_t *root_task) {
  return root_task != NULL && root_task->bootstrapped && root_task->dormant;
}

bool hubos_root_task_dispatch(hubos_root_task_t *root_task,
                              const hubos_root_task_request_t *request,
                              hubos_root_task_response_t *response) {
  if (root_task == NULL || request == NULL || response == NULL) {
    return false;
  }

  hubos_root_task_response_init(response);

  switch (request->operation) {
  case HUBOS_ROOT_TASK_OP_BOOTSTRAP:
    if (!hubos_root_task_bootstrap(root_task)) {
      response->status = HUBOS_IPC_STATUS_DENIED;
      return false;
    }
    response->status = HUBOS_IPC_STATUS_OK;
    return true;
  case HUBOS_ROOT_TASK_OP_COMPLETE_BOOT_STEP:
    if (!root_task->bootstrapped) {
      response->status = HUBOS_IPC_STATUS_DENIED;
      return false;
    }
    response->boot_step = request->payload.boot_step.step;
    if (!hubos_system_complete_boot_step(root_task->system, response->boot_step)) {
      response->status = HUBOS_IPC_STATUS_CONFLICT;
      return false;
    }
    response->status = HUBOS_IPC_STATUS_OK;
    return true;
  case HUBOS_ROOT_TASK_OP_QUERY_BOOT_STEP:
    if (!root_task->bootstrapped) {
      response->status = HUBOS_IPC_STATUS_DENIED;
      return false;
    }
    response->boot_step = request->payload.boot_step.step;
    response->bool_result = hubos_system_boot_step_is_complete(root_task->system,
                                                               response->boot_step);
    response->status = HUBOS_IPC_STATUS_OK;
    return true;
  case HUBOS_ROOT_TASK_OP_ADVANCE_CONTROL_PLANE:
    if (!root_task->bootstrapped) {
      response->status = HUBOS_IPC_STATUS_DENIED;
      return false;
    }
    {
      hubos_service_descriptor_t descriptor = {0};
    if (!hubos_root_task_next_control_plane_step(root_task->system, &response->boot_step)) {
      response->bool_result = false;
      response->status = HUBOS_IPC_STATUS_OK;
      return true;
    }
    if (response->boot_step == HUBOS_BOOT_RESOURCE_REGISTRY) {
      response->bool_result = hubos_root_task_seed_registry(root_task);
    } else if (response->boot_step == HUBOS_BOOT_SESSION_MANAGER) {
      response->bool_result = hubos_root_task_seed_root_session(root_task);
    } else if (response->boot_step == HUBOS_BOOT_CAPABILITY_MANAGER) {
      response->bool_result = hubos_root_task_seed_root_capability(root_task);
    } else if (response->boot_step == HUBOS_BOOT_MEMORY_MANAGER) {
      hubos_id_t memory_id = HUBOS_ID_INVALID;
      response->bool_result = hubos_system_allocate_frame(root_task->system,
                                                          4096,
                                                          0,
                                                          &memory_id) &&
                             hubos_system_share_memory(root_task->system, memory_id) &&
                             hubos_system_reclaim_memory(root_task->system, memory_id);
    } else if (response->boot_step == HUBOS_BOOT_DMA_MANAGER) {
      hubos_id_t dma_resource_id = HUBOS_ID_INVALID;
      response->bool_result = hubos_system_bus_discover(root_task->system,
                                                        HUBOS_BUS_PCIE,
                                                        "resource://boot/dma",
                                                        strlen("resource://boot/dma"),
                                                        HUBOS_RESOURCE_DISCOVERED,
                                                        &dma_resource_id) &&
                             hubos_system_map_dma(root_task->system, dma_resource_id);
    } else if (response->boot_step == HUBOS_BOOT_HUB) {
      response->bool_result = hubos_system_resolve(root_task->system,
                                                   hubos_root_task_registry_resource_name,
                                                   strlen(hubos_root_task_registry_resource_name),
                                                   &descriptor);
    } else if (response->boot_step == HUBOS_BOOT_DRIVER_REGISTRY) {
      response->bool_result = hubos_root_task_seed_driver_registry(root_task);
    } else {
      response->bool_result = true;
    }
    if (response->bool_result &&
        !hubos_system_complete_boot_step(root_task->system, response->boot_step)) {
      response->status = HUBOS_IPC_STATUS_CONFLICT;
      return false;
    }
    response->status = response->bool_result ? HUBOS_IPC_STATUS_OK : HUBOS_IPC_STATUS_CONFLICT;
    return response->bool_result;
    }
  }

  response->status = HUBOS_IPC_STATUS_DENIED;
  return false;
}
