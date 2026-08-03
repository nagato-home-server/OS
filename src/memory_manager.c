#include "hubos/memory_manager.h"

#include <stdlib.h>

static hubos_memory_object_t *hubos_memory_manager_find_mutable(hubos_memory_manager_t *manager,
                                                                hubos_id_t memory_id) {
  if (manager == NULL || memory_id == HUBOS_ID_INVALID) {
    return NULL;
  }

  for (size_t index = 0; index < manager->count; ++index) {
    if (manager->items[index].id == memory_id) {
      return &manager->items[index];
    }
  }

  return NULL;
}

static const hubos_memory_object_t *hubos_memory_manager_find(const hubos_memory_manager_t *manager,
                                                              hubos_id_t memory_id) {
  if (manager == NULL || memory_id == HUBOS_ID_INVALID) {
    return NULL;
  }

  for (size_t index = 0; index < manager->count; ++index) {
    if (manager->items[index].id == memory_id) {
      return &manager->items[index];
    }
  }

  return NULL;
}

static bool hubos_memory_manager_reserve(hubos_memory_manager_t *manager, size_t desired_capacity) {
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

static bool hubos_memory_manager_append(hubos_memory_manager_t *manager,
                                        hubos_memory_object_kind_t kind,
                                        size_t size,
                                        unsigned numa_node,
                                        hubos_id_t *out_memory_id) {
  hubos_memory_object_t object;

  if (!hubos_memory_manager_reserve(manager, manager->count + 1)) {
    return false;
  }

  object.id = manager->next_id;
  object.kind = kind;
  object.size = size;
  object.numa_node = numa_node;
  object.in_use = true;

  manager->items[manager->count++] = object;
  manager->next_id++;

  if (out_memory_id != NULL) {
    *out_memory_id = object.id;
  }
  return true;
}

void hubos_memory_manager_init(hubos_memory_manager_t *manager) {
  if (manager == NULL) {
    return;
  }

  manager->items = NULL;
  manager->count = 0;
  manager->capacity = 0;
  manager->next_id = 1;
}

void hubos_memory_manager_destroy(hubos_memory_manager_t *manager) {
  if (manager == NULL) {
    return;
  }

  free(manager->items);
  manager->items = NULL;
  manager->count = 0;
  manager->capacity = 0;
  manager->next_id = 1;
}

bool hubos_memory_manager_allocate_frame(hubos_memory_manager_t *manager,
                                         size_t size,
                                         unsigned numa_node,
                                         hubos_id_t *out_memory_id) {
  if (manager == NULL || size == 0) {
    return false;
  }

  return hubos_memory_manager_append(manager,
                                     HUBOS_MEMORY_FRAME,
                                     size,
                                     numa_node,
                                     out_memory_id);
}

bool hubos_memory_manager_allocate_hugepage(hubos_memory_manager_t *manager,
                                            size_t size,
                                            unsigned numa_node,
                                            hubos_id_t *out_memory_id) {
  if (manager == NULL || size == 0) {
    return false;
  }

  return hubos_memory_manager_append(manager,
                                     HUBOS_MEMORY_HUGEPAGE,
                                     size,
                                     numa_node,
                                     out_memory_id);
}

bool hubos_memory_manager_share(hubos_memory_manager_t *manager, hubos_id_t memory_id) {
  hubos_memory_object_t *object = hubos_memory_manager_find_mutable(manager, memory_id);

  if (object == NULL) {
    return false;
  }

  object->kind = HUBOS_MEMORY_SHARED;
  return true;
}

bool hubos_memory_manager_reclaim(hubos_memory_manager_t *manager, hubos_id_t memory_id) {
  hubos_memory_object_t *object = hubos_memory_manager_find_mutable(manager, memory_id);

  if (object == NULL || !object->in_use) {
    return false;
  }

  object->in_use = false;
  object->kind = HUBOS_MEMORY_UNTYPED;
  object->size = 0;
  return true;
}

const hubos_memory_object_t *hubos_memory_manager_get(const hubos_memory_manager_t *manager,
                                                      hubos_id_t memory_id) {
  return hubos_memory_manager_find(manager, memory_id);
}

size_t hubos_memory_manager_count(const hubos_memory_manager_t *manager) {
  return manager != NULL ? manager->count : 0;
}
