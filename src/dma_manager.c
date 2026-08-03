#include "hubos/dma_manager.h"

#include <stdlib.h>

static hubos_dma_mapping_t *hubos_dma_manager_find_mutable(hubos_dma_manager_t *manager,
                                                           hubos_id_t resource_id) {
  if (manager == NULL || resource_id == HUBOS_ID_INVALID) {
    return NULL;
  }

  for (size_t index = 0; index < manager->count; ++index) {
    if (manager->items[index].resource_id == resource_id) {
      return &manager->items[index];
    }
  }

  return NULL;
}

static const hubos_dma_mapping_t *hubos_dma_manager_find(const hubos_dma_manager_t *manager,
                                                         hubos_id_t resource_id) {
  if (manager == NULL || resource_id == HUBOS_ID_INVALID) {
    return NULL;
  }

  for (size_t index = 0; index < manager->count; ++index) {
    if (manager->items[index].resource_id == resource_id) {
      return &manager->items[index];
    }
  }

  return NULL;
}

static bool hubos_dma_manager_reserve(hubos_dma_manager_t *manager, size_t desired_capacity) {
  if (manager == NULL) {
    return false;
  }

  if (manager->capacity >= desired_capacity) {
    return true;
  }

  size_t new_capacity = manager->capacity == 0 ? 4 : manager->capacity;
  while (new_capacity < desired_capacity) {
    new_capacity *= 2;
  }

  void *new_items = realloc(manager->items, new_capacity * sizeof(*manager->items));
  if (new_items == NULL) {
    return false;
  }

  manager->items = new_items;
  manager->capacity = new_capacity;
  return true;
}

static bool hubos_dma_manager_ensure(hubos_dma_manager_t *manager, hubos_id_t resource_id) {
  hubos_dma_mapping_t *mapping = hubos_dma_manager_find_mutable(manager, resource_id);

  if (mapping != NULL) {
    return true;
  }

  if (!hubos_dma_manager_reserve(manager, manager->count + 1)) {
    return false;
  }

  mapping = &manager->items[manager->count++];
  mapping->resource_id = resource_id;
  mapping->state = HUBOS_DMA_UNMAPPED;
  mapping->queue_empty = false;
  mapping->outstanding_complete = false;
  mapping->interrupts_drained = false;
  return true;
}

void hubos_dma_manager_init(hubos_dma_manager_t *manager) {
  if (manager == NULL) {
    return;
  }

  manager->items = NULL;
  manager->count = 0;
  manager->capacity = 0;
}

void hubos_dma_manager_destroy(hubos_dma_manager_t *manager) {
  if (manager == NULL) {
    return;
  }

  free(manager->items);
  manager->items = NULL;
  manager->count = 0;
  manager->capacity = 0;
}

bool hubos_dma_manager_map(hubos_dma_manager_t *manager, hubos_id_t resource_id) {
  hubos_dma_mapping_t *mapping = NULL;

  if (manager == NULL || resource_id == HUBOS_ID_INVALID) {
    return false;
  }

  if (!hubos_dma_manager_ensure(manager, resource_id)) {
    return false;
  }

  mapping = hubos_dma_manager_find_mutable(manager, resource_id);
  if (mapping == NULL) {
    return false;
  }

  mapping->state = HUBOS_DMA_MAPPING;
  mapping->queue_empty = false;
  mapping->outstanding_complete = false;
  mapping->interrupts_drained = false;
  mapping->state = HUBOS_DMA_ACTIVE;
  return true;
}

bool hubos_dma_manager_mark_queue_empty(hubos_dma_manager_t *manager, hubos_id_t resource_id) {
  hubos_dma_mapping_t *mapping = hubos_dma_manager_find_mutable(manager, resource_id);

  if (mapping == NULL) {
    return false;
  }

  mapping->queue_empty = true;
  return true;
}

bool hubos_dma_manager_mark_outstanding_complete(hubos_dma_manager_t *manager,
                                                 hubos_id_t resource_id) {
  hubos_dma_mapping_t *mapping = hubos_dma_manager_find_mutable(manager, resource_id);

  if (mapping == NULL) {
    return false;
  }

  mapping->outstanding_complete = true;
  return true;
}

bool hubos_dma_manager_mark_interrupts_drained(hubos_dma_manager_t *manager,
                                               hubos_id_t resource_id) {
  hubos_dma_mapping_t *mapping = hubos_dma_manager_find_mutable(manager, resource_id);

  if (mapping == NULL) {
    return false;
  }

  mapping->interrupts_drained = true;
  return true;
}

bool hubos_dma_manager_begin_revoke(hubos_dma_manager_t *manager, hubos_id_t resource_id) {
  hubos_dma_mapping_t *mapping = hubos_dma_manager_find_mutable(manager, resource_id);

  if (mapping == NULL || mapping->state != HUBOS_DMA_ACTIVE) {
    return false;
  }

  mapping->state = HUBOS_DMA_QUIESCING;
  return true;
}

bool hubos_dma_manager_finalize_revoke(hubos_dma_manager_t *manager, hubos_id_t resource_id) {
  hubos_dma_mapping_t *mapping = hubos_dma_manager_find_mutable(manager, resource_id);

  if (mapping == NULL || mapping->state != HUBOS_DMA_QUIESCING) {
    return false;
  }

  if (!mapping->queue_empty || !mapping->outstanding_complete || !mapping->interrupts_drained) {
    return false;
  }

  mapping->state = HUBOS_DMA_REVOKED;
  return true;
}

bool hubos_dma_manager_abort(hubos_dma_manager_t *manager, hubos_id_t resource_id) {
  hubos_dma_mapping_t *mapping = hubos_dma_manager_find_mutable(manager, resource_id);

  if (mapping == NULL) {
    return false;
  }

  if (mapping->state != HUBOS_DMA_ACTIVE && mapping->state != HUBOS_DMA_QUIESCING) {
    return false;
  }

  mapping->state = HUBOS_DMA_ABORTED;
  return true;
}

const hubos_dma_mapping_t *hubos_dma_manager_get(const hubos_dma_manager_t *manager,
                                                 hubos_id_t resource_id) {
  return hubos_dma_manager_find(manager, resource_id);
}

size_t hubos_dma_manager_count(const hubos_dma_manager_t *manager) {
  return manager != NULL ? manager->count : 0;
}
