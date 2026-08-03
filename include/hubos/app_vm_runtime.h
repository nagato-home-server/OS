#ifndef HUBOS_APP_VM_RUNTIME_H
#define HUBOS_APP_VM_RUNTIME_H

#include <stddef.h>
#include <string.h>

#include "hubos/app_model.h"
#include "hubos/linux_vm_layout.h"

typedef enum {
  HUBOS_APP_VM_GUEST_CLASS_RUNTIME = 0,
  HUBOS_APP_VM_GUEST_CLASS_FULL,
} hubos_app_vm_guest_class_t;

typedef enum {
  HUBOS_APP_VM_OS_FAMILY_UNKNOWN = 0,
  HUBOS_APP_VM_OS_FAMILY_LINUX,
  HUBOS_APP_VM_OS_FAMILY_BSD,
  HUBOS_APP_VM_OS_FAMILY_UNIKERNEL,
} hubos_app_vm_os_family_t;

typedef struct {
  unsigned memory_mb;
  unsigned vcpus;
  bool virtio_net;
  bool virtio_blk;
  bool vgpu;
} hubos_app_vm_runtime_resources_t;

typedef struct {
  const char *artifact_name;
  const char *sha256_hex;
} hubos_app_vm_runtime_artifact_hash_t;

typedef struct {
  const char *id;
  hubos_app_vm_guest_class_t guest_class;
  hubos_app_vm_os_family_t os_family;
  const char *description;
  const char *version;
  const char *update_policy;
  const char *bundle_path;
  const char *const *source_bundles;
  size_t source_bundle_count;
  const hubos_app_vm_runtime_artifact_hash_t *artifact_hashes;
  size_t artifact_hash_count;
  hubos_linux_vm_artifacts_t artifacts;
  hubos_app_vm_runtime_resources_t resources;
} hubos_app_vm_runtime_profile_t;

typedef struct {
  hubos_id_t app_id;
  const char *runtime_profile_id;
} hubos_app_vm_runtime_assignment_t;

typedef struct {
  const hubos_app_vm_runtime_profile_t *profiles;
  size_t profile_count;
  const char *const *installed_profile_ids;
  size_t installed_profile_count;
  const char *default_profile_id;
  const hubos_app_vm_runtime_assignment_t *app_assignments;
  size_t app_assignment_count;
} hubos_app_vm_runtime_selection_t;

static inline const hubos_app_vm_runtime_profile_t *hubos_app_vm_runtime_catalog_find(
  const hubos_app_vm_runtime_profile_t *profiles,
  size_t profile_count,
  const char *profile_id) {
  if (profiles == NULL || profile_id == NULL) {
    return NULL;
  }

  for (size_t index = 0; index < profile_count; ++index) {
    if (profiles[index].id != NULL && strcmp(profiles[index].id, profile_id) == 0) {
      return &profiles[index];
    }
  }

  return NULL;
}

static inline const hubos_app_vm_runtime_artifact_hash_t *hubos_app_vm_runtime_find_artifact_hash(
  const hubos_app_vm_runtime_profile_t *profile,
  const char *artifact_name) {
  if (profile == NULL || artifact_name == NULL || profile->artifact_hashes == NULL) {
    return NULL;
  }

  for (size_t index = 0; index < profile->artifact_hash_count; ++index) {
    const hubos_app_vm_runtime_artifact_hash_t *artifact_hash = &profile->artifact_hashes[index];
    if (artifact_hash->artifact_name != NULL &&
        strcmp(artifact_hash->artifact_name, artifact_name) == 0) {
      return artifact_hash;
    }
  }

  return NULL;
}

static inline bool hubos_app_vm_runtime_uses_source_bundle(
  const hubos_app_vm_runtime_profile_t *profile,
  const char *source_bundle_name) {
  if (profile == NULL || source_bundle_name == NULL || profile->source_bundles == NULL) {
    return false;
  }

  for (size_t index = 0; index < profile->source_bundle_count; ++index) {
    const char *bundle_name = profile->source_bundles[index];
    if (bundle_name != NULL && strcmp(bundle_name, source_bundle_name) == 0) {
      return true;
    }
  }

  return false;
}

static inline bool hubos_app_vm_runtime_profile_validate(const hubos_app_vm_runtime_profile_t *profile) {
  if (profile == NULL || profile->id == NULL || profile->version == NULL ||
      profile->update_policy == NULL || profile->artifacts.kernel_image == NULL ||
      profile->resources.vcpus == 0 || profile->bundle_path == NULL) {
    return false;
  }

  if (profile->artifact_hash_count > 0 && profile->artifact_hashes == NULL) {
    return false;
  }

  if (profile->source_bundle_count > 0 && profile->source_bundles == NULL) {
    return false;
  }

  return true;
}

static inline bool hubos_app_vm_runtime_selection_is_installed(
  const hubos_app_vm_runtime_selection_t *selection,
  const char *profile_id) {
  if (selection == NULL || profile_id == NULL || selection->installed_profile_ids == NULL) {
    return false;
  }

  for (size_t index = 0; index < selection->installed_profile_count; ++index) {
    const char *installed_id = selection->installed_profile_ids[index];
    if (installed_id != NULL && strcmp(installed_id, profile_id) == 0) {
      return true;
    }
  }

  return false;
}

static inline const char *hubos_app_vm_runtime_selection_profile_id_for_app(
  const hubos_app_vm_runtime_selection_t *selection,
  hubos_id_t app_id) {
  if (selection == NULL) {
    return NULL;
  }

  if (selection->app_assignments != NULL) {
    for (size_t index = 0; index < selection->app_assignment_count; ++index) {
      if (selection->app_assignments[index].app_id == app_id) {
        return selection->app_assignments[index].runtime_profile_id;
      }
    }
  }

  return selection->default_profile_id;
}

static inline const hubos_app_vm_runtime_profile_t *hubos_app_vm_runtime_selection_resolve(
  const hubos_app_vm_runtime_selection_t *selection,
  hubos_id_t app_id) {
  const char *profile_id = NULL;

  if (selection == NULL) {
    return NULL;
  }

  profile_id = hubos_app_vm_runtime_selection_profile_id_for_app(selection, app_id);
  if (profile_id == NULL ||
      !hubos_app_vm_runtime_selection_is_installed(selection, profile_id)) {
    return NULL;
  }

  return hubos_app_vm_runtime_catalog_find(selection->profiles, selection->profile_count, profile_id);
}

#endif
