#include "hubos/microkit_graph.h"

static const hubos_microkit_component_kind_t hubos_resource_registry_dependencies[] = {
  HUBOS_MICROKIT_COMPONENT_ROOT_TASK,
};

static const hubos_microkit_component_kind_t hubos_capability_manager_dependencies[] = {
  HUBOS_MICROKIT_COMPONENT_ROOT_TASK,
  HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY,
};

static const hubos_microkit_component_kind_t hubos_session_manager_dependencies[] = {
  HUBOS_MICROKIT_COMPONENT_ROOT_TASK,
  HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER,
};

static const hubos_microkit_component_kind_t hubos_memory_manager_dependencies[] = {
  HUBOS_MICROKIT_COMPONENT_ROOT_TASK,
  HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY,
};

static const hubos_microkit_component_kind_t hubos_dma_manager_dependencies[] = {
  HUBOS_MICROKIT_COMPONENT_ROOT_TASK,
  HUBOS_MICROKIT_COMPONENT_MEMORY_MANAGER,
  HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY,
};

static const hubos_microkit_component_kind_t hubos_hub_dependencies[] = {
  HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY,
  HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER,
  HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER,
};

static const hubos_microkit_component_kind_t hubos_driver_registry_dependencies[] = {
  HUBOS_MICROKIT_COMPONENT_ROOT_TASK,
  HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY,
};

static const hubos_microkit_component_kind_t hubos_driver_loader_dependencies[] = {
  HUBOS_MICROKIT_COMPONENT_ROOT_TASK,
  HUBOS_MICROKIT_COMPONENT_DRIVER_REGISTRY,
};

static const hubos_microkit_component_kind_t hubos_bus_managers_dependencies[] = {
  HUBOS_MICROKIT_COMPONENT_ROOT_TASK,
  HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY,
  HUBOS_MICROKIT_COMPONENT_DRIVER_REGISTRY,
};

static const hubos_microkit_component_kind_t hubos_driver_service_dependencies[] = {
  HUBOS_MICROKIT_COMPONENT_ROOT_TASK,
  HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER,
  HUBOS_MICROKIT_COMPONENT_DRIVER_REGISTRY,
  HUBOS_MICROKIT_COMPONENT_DRIVER_LOADER,
};

static const hubos_microkit_component_kind_t hubos_network_server_dependencies[] = {
  HUBOS_MICROKIT_COMPONENT_ROOT_TASK,
  HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER,
  HUBOS_MICROKIT_COMPONENT_HUB,
  HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY,
};

static const hubos_microkit_component_kind_t hubos_device_server_dependencies[] = {
  HUBOS_MICROKIT_COMPONENT_ROOT_TASK,
  HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER,
  HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER,
  HUBOS_MICROKIT_COMPONENT_DMA_MANAGER,
  HUBOS_MICROKIT_COMPONENT_BUS_MANAGERS,
};

static const hubos_microkit_component_kind_t hubos_storage_server_dependencies[] = {
  HUBOS_MICROKIT_COMPONENT_ROOT_TASK,
  HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER,
  HUBOS_MICROKIT_COMPONENT_HUB,
  HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY,
};

static const hubos_microkit_component_kind_t hubos_display_server_dependencies[] = {
  HUBOS_MICROKIT_COMPONENT_ROOT_TASK,
  HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER,
  HUBOS_MICROKIT_COMPONENT_HUB,
  HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY,
};

static const hubos_microkit_component_kind_t hubos_vm_server_dependencies[] = {
  HUBOS_MICROKIT_COMPONENT_ROOT_TASK,
  HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER,
  HUBOS_MICROKIT_COMPONENT_MEMORY_MANAGER,
  HUBOS_MICROKIT_COMPONENT_HUB,
  HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER,
  HUBOS_MICROKIT_COMPONENT_STORAGE_SERVER,
  HUBOS_MICROKIT_COMPONENT_DISPLAY_SERVER,
};

static const hubos_microkit_component_t hubos_microkit_components[] = {
  {
    .kind = HUBOS_MICROKIT_COMPONENT_ROOT_TASK,
    .name = "Root Task",
    .phase = HUBOS_MICROKIT_PHASE_BOOTSTRAP,
    .bootstrap_only = false,
    .restartable = false,
    .dependencies = NULL,
    .dependency_count = 0,
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY,
    .name = "Resource Registry",
    .phase = HUBOS_MICROKIT_PHASE_CORE_SERVICES,
    .bootstrap_only = false,
    .restartable = true,
    .dependencies = hubos_resource_registry_dependencies,
    .dependency_count = sizeof(hubos_resource_registry_dependencies) /
                        sizeof(hubos_resource_registry_dependencies[0]),
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER,
    .name = "Capability Manager",
    .phase = HUBOS_MICROKIT_PHASE_CORE_SERVICES,
    .bootstrap_only = false,
    .restartable = true,
    .dependencies = hubos_capability_manager_dependencies,
    .dependency_count = sizeof(hubos_capability_manager_dependencies) /
                        sizeof(hubos_capability_manager_dependencies[0]),
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER,
    .name = "Session Manager",
    .phase = HUBOS_MICROKIT_PHASE_CORE_SERVICES,
    .bootstrap_only = false,
    .restartable = true,
    .dependencies = hubos_session_manager_dependencies,
    .dependency_count = sizeof(hubos_session_manager_dependencies) /
                        sizeof(hubos_session_manager_dependencies[0]),
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_MEMORY_MANAGER,
    .name = "Memory Manager",
    .phase = HUBOS_MICROKIT_PHASE_CORE_SERVICES,
    .bootstrap_only = false,
    .restartable = true,
    .dependencies = hubos_memory_manager_dependencies,
    .dependency_count = sizeof(hubos_memory_manager_dependencies) /
                        sizeof(hubos_memory_manager_dependencies[0]),
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_DMA_MANAGER,
    .name = "DMA Manager",
    .phase = HUBOS_MICROKIT_PHASE_SUPPORT_SERVICES,
    .bootstrap_only = false,
    .restartable = true,
    .dependencies = hubos_dma_manager_dependencies,
    .dependency_count = sizeof(hubos_dma_manager_dependencies) /
                        sizeof(hubos_dma_manager_dependencies[0]),
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_HUB,
    .name = "Hub",
    .phase = HUBOS_MICROKIT_PHASE_CORE_SERVICES,
    .bootstrap_only = false,
    .restartable = true,
    .dependencies = hubos_hub_dependencies,
    .dependency_count = sizeof(hubos_hub_dependencies) /
                        sizeof(hubos_hub_dependencies[0]),
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_DRIVER_REGISTRY,
    .name = "Driver Registry",
    .phase = HUBOS_MICROKIT_PHASE_SUPPORT_SERVICES,
    .bootstrap_only = false,
    .restartable = true,
    .dependencies = hubos_driver_registry_dependencies,
    .dependency_count = sizeof(hubos_driver_registry_dependencies) /
                        sizeof(hubos_driver_registry_dependencies[0]),
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_DRIVER_LOADER,
    .name = "Driver Loader",
    .phase = HUBOS_MICROKIT_PHASE_SUPPORT_SERVICES,
    .bootstrap_only = false,
    .restartable = true,
    .dependencies = hubos_driver_loader_dependencies,
    .dependency_count = sizeof(hubos_driver_loader_dependencies) /
                        sizeof(hubos_driver_loader_dependencies[0]),
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_BUS_MANAGERS,
    .name = "Bus Managers",
    .phase = HUBOS_MICROKIT_PHASE_SUPPORT_SERVICES,
    .bootstrap_only = false,
    .restartable = true,
    .dependencies = hubos_bus_managers_dependencies,
    .dependency_count = sizeof(hubos_bus_managers_dependencies) /
                        sizeof(hubos_bus_managers_dependencies[0]),
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_DRIVER_SERVICE,
    .name = "Driver Service",
    .phase = HUBOS_MICROKIT_PHASE_SUPPORT_SERVICES,
    .bootstrap_only = false,
    .restartable = true,
    .dependencies = hubos_driver_service_dependencies,
    .dependency_count = sizeof(hubos_driver_service_dependencies) /
                        sizeof(hubos_driver_service_dependencies[0]),
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER,
    .name = "Network Server",
    .phase = HUBOS_MICROKIT_PHASE_DEVICE_SERVICES,
    .bootstrap_only = false,
    .restartable = true,
    .dependencies = hubos_network_server_dependencies,
    .dependency_count = sizeof(hubos_network_server_dependencies) /
                        sizeof(hubos_network_server_dependencies[0]),
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER,
    .name = "Device Server",
    .phase = HUBOS_MICROKIT_PHASE_DEVICE_SERVICES,
    .bootstrap_only = false,
    .restartable = true,
    .dependencies = hubos_device_server_dependencies,
    .dependency_count = sizeof(hubos_device_server_dependencies) /
                        sizeof(hubos_device_server_dependencies[0]),
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_STORAGE_SERVER,
    .name = "Storage Server",
    .phase = HUBOS_MICROKIT_PHASE_DEVICE_SERVICES,
    .bootstrap_only = false,
    .restartable = true,
    .dependencies = hubos_storage_server_dependencies,
    .dependency_count = sizeof(hubos_storage_server_dependencies) /
                        sizeof(hubos_storage_server_dependencies[0]),
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_DISPLAY_SERVER,
    .name = "Display Server",
    .phase = HUBOS_MICROKIT_PHASE_DEVICE_SERVICES,
    .bootstrap_only = false,
    .restartable = true,
    .dependencies = hubos_display_server_dependencies,
    .dependency_count = sizeof(hubos_display_server_dependencies) /
                        sizeof(hubos_display_server_dependencies[0]),
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_VM_SERVER,
    .name = "VM Server",
    .phase = HUBOS_MICROKIT_PHASE_CONSUMERS,
    .bootstrap_only = false,
    .restartable = true,
    .dependencies = hubos_vm_server_dependencies,
    .dependency_count = sizeof(hubos_vm_server_dependencies) /
                        sizeof(hubos_vm_server_dependencies[0]),
  },
};

static size_t hubos_microkit_component_rank(hubos_microkit_component_kind_t kind) {
  switch (kind) {
  case HUBOS_MICROKIT_COMPONENT_ROOT_TASK:
    return 0;
  case HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY:
    return 1;
  case HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER:
    return 2;
  case HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER:
    return 3;
  case HUBOS_MICROKIT_COMPONENT_MEMORY_MANAGER:
    return 4;
  case HUBOS_MICROKIT_COMPONENT_DMA_MANAGER:
    return 5;
  case HUBOS_MICROKIT_COMPONENT_HUB:
    return 6;
  case HUBOS_MICROKIT_COMPONENT_DRIVER_REGISTRY:
    return 7;
  case HUBOS_MICROKIT_COMPONENT_DRIVER_LOADER:
    return 8;
  case HUBOS_MICROKIT_COMPONENT_BUS_MANAGERS:
    return 9;
  case HUBOS_MICROKIT_COMPONENT_DRIVER_SERVICE:
    return 10;
  case HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER:
    return 11;
  case HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER:
    return 12;
  case HUBOS_MICROKIT_COMPONENT_STORAGE_SERVER:
    return 13;
  case HUBOS_MICROKIT_COMPONENT_DISPLAY_SERVER:
    return 14;
  case HUBOS_MICROKIT_COMPONENT_VM_SERVER:
    return 15;
  case HUBOS_MICROKIT_COMPONENT_COUNT:
    break;
  }

  return HUBOS_MICROKIT_COMPONENT_COUNT;
}

static bool hubos_microkit_graph_has_acyclic_dependencies(const hubos_microkit_graph_t *graph) {
  if (graph == NULL || graph->components == NULL) {
    return false;
  }

  for (size_t index = 0; index < graph->component_count; ++index) {
    const hubos_microkit_component_t *component = &graph->components[index];
    size_t component_rank = hubos_microkit_component_rank(component->kind);

    if (component->kind == HUBOS_MICROKIT_COMPONENT_ROOT_TASK &&
        component->dependency_count != 0) {
      return false;
    }

    if (component_rank == HUBOS_MICROKIT_COMPONENT_COUNT) {
      return false;
    }

    for (size_t dep_index = 0; dep_index < component->dependency_count; ++dep_index) {
      hubos_microkit_component_kind_t dependency = component->dependencies[dep_index];
      const hubos_microkit_component_t *dependency_component = hubos_microkit_graph_get(graph,
                                                                                        dependency);
      if (dependency_component == NULL) {
        return false;
      }
      if (hubos_microkit_component_rank(dependency) >= component_rank) {
        return false;
      }
    }
  }

  return true;
}

static bool hubos_microkit_graph_has_unique_components(const hubos_microkit_graph_t *graph) {
  if (graph == NULL || graph->components == NULL) {
    return false;
  }

  for (size_t index = 0; index < graph->component_count; ++index) {
    const hubos_microkit_component_t *component = &graph->components[index];
    if (hubos_microkit_graph_get(graph, component->kind) == NULL) {
      return false;
    }

    for (size_t other_index = index + 1; other_index < graph->component_count; ++other_index) {
      if (graph->components[other_index].kind == component->kind) {
        return false;
      }
    }
  }

  return true;
}

void hubos_microkit_graph_init(hubos_microkit_graph_t *graph) {
  if (graph == NULL) {
    return;
  }

  graph->components = hubos_microkit_components;
  graph->component_count = sizeof(hubos_microkit_components) / sizeof(hubos_microkit_components[0]);
}

void hubos_microkit_graph_destroy(hubos_microkit_graph_t *graph) {
  if (graph == NULL) {
    return;
  }

  graph->components = NULL;
  graph->component_count = 0;
}

const hubos_microkit_component_t *hubos_microkit_graph_get(
  const hubos_microkit_graph_t *graph,
  hubos_microkit_component_kind_t kind) {
  if (graph == NULL || graph->components == NULL) {
    return NULL;
  }

  for (size_t index = 0; index < graph->component_count; ++index) {
    if (graph->components[index].kind == kind) {
      return &graph->components[index];
    }
  }

  return NULL;
}

bool hubos_microkit_graph_validate(const hubos_microkit_graph_t *graph) {
  if (graph == NULL || graph->components == NULL || graph->component_count == 0) {
    return false;
  }

  if (hubos_microkit_graph_get(graph, HUBOS_MICROKIT_COMPONENT_ROOT_TASK) == NULL) {
    return false;
  }

  for (hubos_microkit_component_kind_t kind = 0; kind < HUBOS_MICROKIT_COMPONENT_COUNT; ++kind) {
    if (hubos_microkit_graph_get(graph, kind) == NULL) {
      return false;
    }
  }

  if (!hubos_microkit_graph_has_unique_components(graph)) {
    return false;
  }

  return hubos_microkit_graph_has_acyclic_dependencies(graph);
}

size_t hubos_microkit_graph_boot_order(const hubos_microkit_graph_t *graph,
                                       hubos_microkit_component_kind_t *out_order,
                                       size_t order_capacity) {
  static const hubos_microkit_component_kind_t fixed_order[] = {
    HUBOS_MICROKIT_COMPONENT_ROOT_TASK,
    HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY,
    HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER,
    HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER,
    HUBOS_MICROKIT_COMPONENT_MEMORY_MANAGER,
    HUBOS_MICROKIT_COMPONENT_DMA_MANAGER,
    HUBOS_MICROKIT_COMPONENT_HUB,
    HUBOS_MICROKIT_COMPONENT_DRIVER_REGISTRY,
    HUBOS_MICROKIT_COMPONENT_DRIVER_LOADER,
    HUBOS_MICROKIT_COMPONENT_BUS_MANAGERS,
    HUBOS_MICROKIT_COMPONENT_DRIVER_SERVICE,
    HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER,
    HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER,
    HUBOS_MICROKIT_COMPONENT_STORAGE_SERVER,
    HUBOS_MICROKIT_COMPONENT_DISPLAY_SERVER,
    HUBOS_MICROKIT_COMPONENT_VM_SERVER,
  };
  size_t count = sizeof(fixed_order) / sizeof(fixed_order[0]);

  if (!hubos_microkit_graph_validate(graph)) {
    return 0;
  }

  if (out_order != NULL) {
    size_t limit = order_capacity < count ? order_capacity : count;
    for (size_t index = 0; index < limit; ++index) {
      out_order[index] = fixed_order[index];
    }
  }

  return count;
}

const char *hubos_microkit_component_name(hubos_microkit_component_kind_t kind) {
  const hubos_microkit_component_t *component = hubos_microkit_graph_get(
    &(hubos_microkit_graph_t) {
      .components = hubos_microkit_components,
      .component_count = sizeof(hubos_microkit_components) / sizeof(hubos_microkit_components[0]),
    },
    kind);

  return component != NULL ? component->name : NULL;
}
