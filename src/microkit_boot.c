#include "hubos/microkit_boot.h"

static const hubos_microkit_boot_component_t hubos_microkit_boot_components[] = {
  {
    .kind = HUBOS_MICROKIT_COMPONENT_ROOT_TASK,
    .name = "Root Task",
    .badge = 0,
    .startup_order = 0,
    .bootstrap_only = false,
    .restartable = false,
    .channel_flags = HUBOS_MICROKIT_CHANNEL_ENDPOINT,
    .endpoint_published = true,
    .notification_published = false,
    .irq_published = false,
    .shared_memory_published = false,
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY,
    .name = "Resource Registry",
    .badge = 1,
    .startup_order = 1,
    .bootstrap_only = false,
    .restartable = true,
    .channel_flags = HUBOS_MICROKIT_CHANNEL_ENDPOINT,
    .endpoint_published = true,
    .notification_published = false,
    .irq_published = false,
    .shared_memory_published = false,
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER,
    .name = "Capability Manager",
    .badge = 2,
    .startup_order = 2,
    .bootstrap_only = false,
    .restartable = true,
    .channel_flags = HUBOS_MICROKIT_CHANNEL_ENDPOINT,
    .endpoint_published = true,
    .notification_published = false,
    .irq_published = false,
    .shared_memory_published = false,
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER,
    .name = "Session Manager",
    .badge = 3,
    .startup_order = 3,
    .bootstrap_only = false,
    .restartable = true,
    .channel_flags = HUBOS_MICROKIT_CHANNEL_ENDPOINT,
    .endpoint_published = true,
    .notification_published = false,
    .irq_published = false,
    .shared_memory_published = false,
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_MEMORY_MANAGER,
    .name = "Memory Manager",
    .badge = 7,
    .startup_order = 4,
    .bootstrap_only = false,
    .restartable = true,
    .channel_flags = 0,
    .endpoint_published = false,
    .notification_published = false,
    .irq_published = false,
    .shared_memory_published = false,
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_DMA_MANAGER,
    .name = "DMA Manager",
    .badge = 8,
    .startup_order = 5,
    .bootstrap_only = false,
    .restartable = true,
    .channel_flags = HUBOS_MICROKIT_CHANNEL_NOTIFICATION | HUBOS_MICROKIT_CHANNEL_SHARED_MEMORY,
    .endpoint_published = false,
    .notification_published = true,
    .irq_published = false,
    .shared_memory_published = true,
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_HUB,
    .name = "Hub",
    .badge = 4,
    .startup_order = 6,
    .bootstrap_only = false,
    .restartable = true,
    .channel_flags = HUBOS_MICROKIT_CHANNEL_ENDPOINT,
    .endpoint_published = true,
    .notification_published = false,
    .irq_published = false,
    .shared_memory_published = false,
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_DRIVER_REGISTRY,
    .name = "Driver Registry",
    .badge = 9,
    .startup_order = 7,
    .bootstrap_only = false,
    .restartable = true,
    .channel_flags = 0,
    .endpoint_published = false,
    .notification_published = false,
    .irq_published = false,
    .shared_memory_published = false,
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_DRIVER_LOADER,
    .name = "Driver Loader",
    .badge = 10,
    .startup_order = 8,
    .bootstrap_only = false,
    .restartable = true,
    .channel_flags = HUBOS_MICROKIT_CHANNEL_NOTIFICATION,
    .endpoint_published = false,
    .notification_published = true,
    .irq_published = false,
    .shared_memory_published = false,
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_BUS_MANAGERS,
    .name = "Bus Managers",
    .badge = 11,
    .startup_order = 9,
    .bootstrap_only = false,
    .restartable = true,
    .channel_flags = HUBOS_MICROKIT_CHANNEL_NOTIFICATION,
    .endpoint_published = false,
    .notification_published = true,
    .irq_published = false,
    .shared_memory_published = false,
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_DRIVER_SERVICE,
    .name = "Driver Service",
    .badge = 5,
    .startup_order = 10,
    .bootstrap_only = false,
    .restartable = true,
    .channel_flags = HUBOS_MICROKIT_CHANNEL_ENDPOINT | HUBOS_MICROKIT_CHANNEL_NOTIFICATION,
    .endpoint_published = true,
    .notification_published = true,
    .irq_published = false,
    .shared_memory_published = false,
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER,
    .name = "Network Server",
    .badge = 6,
    .startup_order = 11,
    .bootstrap_only = false,
    .restartable = true,
    .channel_flags = HUBOS_MICROKIT_CHANNEL_ENDPOINT | HUBOS_MICROKIT_CHANNEL_NOTIFICATION |
                     HUBOS_MICROKIT_CHANNEL_SHARED_MEMORY,
    .endpoint_published = true,
    .notification_published = true,
    .irq_published = false,
    .shared_memory_published = true,
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER,
    .name = "Device Server",
    .badge = 12,
    .startup_order = 12,
    .bootstrap_only = false,
    .restartable = true,
    .channel_flags = HUBOS_MICROKIT_CHANNEL_ENDPOINT | HUBOS_MICROKIT_CHANNEL_NOTIFICATION |
                     HUBOS_MICROKIT_CHANNEL_IRQ |
                     HUBOS_MICROKIT_CHANNEL_SHARED_MEMORY,
    .endpoint_published = true,
    .notification_published = true,
    .irq_published = true,
    .shared_memory_published = true,
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_STORAGE_SERVER,
    .name = "Storage Server",
    .badge = 13,
    .startup_order = 13,
    .bootstrap_only = false,
    .restartable = true,
    .channel_flags = HUBOS_MICROKIT_CHANNEL_ENDPOINT | HUBOS_MICROKIT_CHANNEL_NOTIFICATION |
                     HUBOS_MICROKIT_CHANNEL_SHARED_MEMORY,
    .endpoint_published = true,
    .notification_published = true,
    .irq_published = false,
    .shared_memory_published = true,
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_DISPLAY_SERVER,
    .name = "Display Server",
    .badge = 14,
    .startup_order = 14,
    .bootstrap_only = false,
    .restartable = true,
    .channel_flags = HUBOS_MICROKIT_CHANNEL_ENDPOINT | HUBOS_MICROKIT_CHANNEL_NOTIFICATION |
                     HUBOS_MICROKIT_CHANNEL_SHARED_MEMORY,
    .endpoint_published = true,
    .notification_published = true,
    .irq_published = false,
    .shared_memory_published = true,
  },
  {
    .kind = HUBOS_MICROKIT_COMPONENT_VM_SERVER,
    .name = "VM Server",
    .badge = 15,
    .startup_order = 15,
    .bootstrap_only = false,
    .restartable = true,
    .channel_flags = HUBOS_MICROKIT_CHANNEL_ENDPOINT |
                     HUBOS_MICROKIT_CHANNEL_NOTIFICATION |
                     HUBOS_MICROKIT_CHANNEL_SHARED_MEMORY,
    .endpoint_published = true,
    .notification_published = true,
    .irq_published = false,
    .shared_memory_published = true,
  },
};

void hubos_microkit_boot_manifest_init(hubos_microkit_boot_manifest_t *manifest) {
  if (manifest == NULL) {
    return;
  }

  manifest->components = hubos_microkit_boot_components;
  manifest->component_count = sizeof(hubos_microkit_boot_components) /
                              sizeof(hubos_microkit_boot_components[0]);
}

const hubos_microkit_boot_component_t *hubos_microkit_boot_manifest_get(
  const hubos_microkit_boot_manifest_t *manifest,
  hubos_microkit_component_kind_t kind) {
  if (manifest == NULL || manifest->components == NULL) {
    return NULL;
  }

  for (size_t index = 0; index < manifest->component_count; ++index) {
    if (manifest->components[index].kind == kind) {
      return &manifest->components[index];
    }
  }

  return NULL;
}

const hubos_microkit_boot_component_t *hubos_microkit_boot_manifest_get_by_badge(
  const hubos_microkit_boot_manifest_t *manifest,
  unsigned badge) {
  if (manifest == NULL || manifest->components == NULL) {
    return NULL;
  }

  for (size_t index = 0; index < manifest->component_count; ++index) {
    if (manifest->components[index].badge == badge) {
      return &manifest->components[index];
    }
  }

  return NULL;
}

bool hubos_microkit_boot_manifest_validate(const hubos_microkit_boot_manifest_t *manifest,
                                          const hubos_microkit_graph_t *graph,
                                          const hubos_microkit_ipc_layout_t *layout) {
  size_t published_endpoint_count = 0;

  if (manifest == NULL || graph == NULL || layout == NULL || manifest->components == NULL) {
    return false;
  }

  if (!hubos_microkit_graph_validate(graph) || !hubos_microkit_ipc_layout_validate(layout, graph)) {
    return false;
  }

  for (size_t index = 0; index < manifest->component_count; ++index) {
    const hubos_microkit_boot_component_t *component = &manifest->components[index];
    const hubos_microkit_endpoint_binding_t *binding = NULL;

    if (component->name == NULL || component->name[0] == '\0') {
      return false;
    }
    if (hubos_microkit_graph_get(graph, component->kind) == NULL) {
      return false;
    }
    if (component->endpoint_published !=
        ((component->channel_flags & HUBOS_MICROKIT_CHANNEL_ENDPOINT) != 0)) {
      return false;
    }
    if (component->notification_published !=
        ((component->channel_flags & HUBOS_MICROKIT_CHANNEL_NOTIFICATION) != 0)) {
      return false;
    }
    if (component->irq_published !=
        ((component->channel_flags & HUBOS_MICROKIT_CHANNEL_IRQ) != 0)) {
      return false;
    }
    if (component->shared_memory_published !=
        ((component->channel_flags & HUBOS_MICROKIT_CHANNEL_SHARED_MEMORY) != 0)) {
      return false;
    }

    binding = hubos_microkit_ipc_layout_get_by_badge(layout, component->badge);
    if (component->endpoint_published) {
      if (binding == NULL || !binding->exposed) {
        return false;
      }
      ++published_endpoint_count;
    } else if (binding != NULL) {
      return false;
    }

    for (size_t other_index = index + 1; other_index < manifest->component_count; ++other_index) {
      const hubos_microkit_boot_component_t *other = &manifest->components[other_index];
      if (other->badge == component->badge) {
        return false;
      }
      if (other->startup_order == component->startup_order) {
        return false;
      }
    }
  }

  return published_endpoint_count == layout->binding_count;
}

size_t hubos_microkit_boot_manifest_publishable_endpoint_count(
  const hubos_microkit_boot_manifest_t *manifest) {
  size_t count = 0;

  if (manifest == NULL || manifest->components == NULL) {
    return 0;
  }

  for (size_t index = 0; index < manifest->component_count; ++index) {
    if (manifest->components[index].endpoint_published) {
      ++count;
    }
  }

  return count;
}
