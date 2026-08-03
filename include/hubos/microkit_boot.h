#ifndef HUBOS_MICROKIT_BOOT_H
#define HUBOS_MICROKIT_BOOT_H

#include "hubos/microkit_ipc.h"

typedef enum {
  HUBOS_MICROKIT_CHANNEL_ENDPOINT = 1u << 0,
  HUBOS_MICROKIT_CHANNEL_NOTIFICATION = 1u << 1,
  HUBOS_MICROKIT_CHANNEL_SHARED_MEMORY = 1u << 2,
  HUBOS_MICROKIT_CHANNEL_IRQ = 1u << 3,
} hubos_microkit_channel_flags_t;

typedef struct {
  hubos_microkit_component_kind_t kind;
  const char *name;
  unsigned badge;
  size_t startup_order;
  bool bootstrap_only;
  bool restartable;
  unsigned channel_flags;
  bool endpoint_published;
  bool notification_published;
  bool irq_published;
  bool shared_memory_published;
} hubos_microkit_boot_component_t;

typedef struct {
  const hubos_microkit_boot_component_t *components;
  size_t component_count;
} hubos_microkit_boot_manifest_t;

void hubos_microkit_boot_manifest_init(hubos_microkit_boot_manifest_t *manifest);
const hubos_microkit_boot_component_t *hubos_microkit_boot_manifest_get(
  const hubos_microkit_boot_manifest_t *manifest,
  hubos_microkit_component_kind_t kind);
const hubos_microkit_boot_component_t *hubos_microkit_boot_manifest_get_by_badge(
  const hubos_microkit_boot_manifest_t *manifest,
  unsigned badge);
bool hubos_microkit_boot_manifest_validate(const hubos_microkit_boot_manifest_t *manifest,
                                          const hubos_microkit_graph_t *graph,
                                          const hubos_microkit_ipc_layout_t *layout);
size_t hubos_microkit_boot_manifest_publishable_endpoint_count(
  const hubos_microkit_boot_manifest_t *manifest);

#endif
