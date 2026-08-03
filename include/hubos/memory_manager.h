#ifndef HUBOS_MEMORY_MANAGER_H
#define HUBOS_MEMORY_MANAGER_H

#include "hubos/model.h"

typedef enum {
  HUBOS_MEMORY_UNTYPED = 0,
  HUBOS_MEMORY_FRAME,
  HUBOS_MEMORY_SHARED,
  HUBOS_MEMORY_HUGEPAGE,
} hubos_memory_object_kind_t;

typedef struct {
  hubos_id_t id;
  hubos_memory_object_kind_t kind;
  size_t size;
  unsigned numa_node;
  bool in_use;
} hubos_memory_object_t;

typedef struct {
  hubos_memory_object_t *items;
  size_t count;
  size_t capacity;
  hubos_id_t next_id;
} hubos_memory_manager_t;

void hubos_memory_manager_init(hubos_memory_manager_t *manager);
void hubos_memory_manager_destroy(hubos_memory_manager_t *manager);

bool hubos_memory_manager_allocate_frame(hubos_memory_manager_t *manager,
                                         size_t size,
                                         unsigned numa_node,
                                         hubos_id_t *out_memory_id);

bool hubos_memory_manager_allocate_hugepage(hubos_memory_manager_t *manager,
                                            size_t size,
                                            unsigned numa_node,
                                            hubos_id_t *out_memory_id);

bool hubos_memory_manager_share(hubos_memory_manager_t *manager,
                                hubos_id_t memory_id);
bool hubos_memory_manager_reclaim(hubos_memory_manager_t *manager,
                                  hubos_id_t memory_id);

const hubos_memory_object_t *hubos_memory_manager_get(const hubos_memory_manager_t *manager,
                                                      hubos_id_t memory_id);
size_t hubos_memory_manager_count(const hubos_memory_manager_t *manager);

#endif
