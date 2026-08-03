#ifndef HUBOS_DMA_MANAGER_H
#define HUBOS_DMA_MANAGER_H

#include "hubos/audit.h"

typedef struct {
  hubos_dma_mapping_t *items;
  size_t count;
  size_t capacity;
} hubos_dma_manager_t;

void hubos_dma_manager_init(hubos_dma_manager_t *manager);
void hubos_dma_manager_destroy(hubos_dma_manager_t *manager);

bool hubos_dma_manager_map(hubos_dma_manager_t *manager, hubos_id_t resource_id);
bool hubos_dma_manager_mark_queue_empty(hubos_dma_manager_t *manager, hubos_id_t resource_id);
bool hubos_dma_manager_mark_outstanding_complete(hubos_dma_manager_t *manager,
                                                 hubos_id_t resource_id);
bool hubos_dma_manager_mark_interrupts_drained(hubos_dma_manager_t *manager,
                                               hubos_id_t resource_id);
bool hubos_dma_manager_begin_revoke(hubos_dma_manager_t *manager, hubos_id_t resource_id);
bool hubos_dma_manager_finalize_revoke(hubos_dma_manager_t *manager, hubos_id_t resource_id);
bool hubos_dma_manager_abort(hubos_dma_manager_t *manager, hubos_id_t resource_id);

const hubos_dma_mapping_t *hubos_dma_manager_get(const hubos_dma_manager_t *manager,
                                                 hubos_id_t resource_id);
size_t hubos_dma_manager_count(const hubos_dma_manager_t *manager);

#endif
