#ifndef HUBOS_MICROKIT_GRAPH_H
#define HUBOS_MICROKIT_GRAPH_H

#include "hubos/model.h"

typedef enum {
  HUBOS_MICROKIT_COMPONENT_ROOT_TASK = 0,
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
  HUBOS_MICROKIT_COMPONENT_COUNT,
} hubos_microkit_component_kind_t;

typedef enum {
  HUBOS_MICROKIT_PHASE_BOOTSTRAP = 0,
  HUBOS_MICROKIT_PHASE_CORE_SERVICES,
  HUBOS_MICROKIT_PHASE_SUPPORT_SERVICES,
  HUBOS_MICROKIT_PHASE_DEVICE_SERVICES,
  HUBOS_MICROKIT_PHASE_CONSUMERS,
} hubos_microkit_phase_t;

typedef struct {
  hubos_microkit_component_kind_t kind;
  const char *name;
  hubos_microkit_phase_t phase;
  bool bootstrap_only;
  bool restartable;
  const hubos_microkit_component_kind_t *dependencies;
  size_t dependency_count;
} hubos_microkit_component_t;

typedef struct {
  const hubos_microkit_component_t *components;
  size_t component_count;
} hubos_microkit_graph_t;

void hubos_microkit_graph_init(hubos_microkit_graph_t *graph);
void hubos_microkit_graph_destroy(hubos_microkit_graph_t *graph);

const hubos_microkit_component_t *hubos_microkit_graph_get(
  const hubos_microkit_graph_t *graph,
  hubos_microkit_component_kind_t kind);

bool hubos_microkit_graph_validate(const hubos_microkit_graph_t *graph);

size_t hubos_microkit_graph_boot_order(const hubos_microkit_graph_t *graph,
                                       hubos_microkit_component_kind_t *out_order,
                                       size_t order_capacity);

const char *hubos_microkit_component_name(hubos_microkit_component_kind_t kind);

#endif
