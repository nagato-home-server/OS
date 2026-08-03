#include "hubos/runtime_config.h"

static const char *const hubos_runtime_config_linux_dev_source_bundles[] = {
  "linux-kernel",
  "buildroot",
};

static const hubos_app_vm_runtime_artifact_hash_t hubos_runtime_config_linux_dev_artifact_hashes[] = {
  { "kernel.elf", "2602c46fd19da5ac16986db9fd8d90227f310b1e5d04118d03873c5a11ba3d68" },
  { "initramfs.cpio.gz", "bcbd23fd825ef20aa5116085e21223614017513459938fbc4ba203396fe358e3" },
  { "rootfs.img", "d19aac3c1ff4e04231c3d86394c8b11c26fb359e7a59e9ecfd0402ac032cbb2d" },
};

static const char *const hubos_runtime_config_mini_bsd_service_source_bundles[] = {
  "freebsd-src",
  "freebsd-ports",
};

static const hubos_app_vm_runtime_artifact_hash_t
  hubos_runtime_config_mini_bsd_service_artifact_hashes[] = {
    { "kernel.elf", "e04e21b3c900625ad85ebad1b4e64484f11321d377064c1a7d43e158e055e316" },
    { "rootfs.img", "f641054e7467e27ff1a3565bf908f95e43eec70c28fab61d9dee266d0ecb9cb6" },
  };

static const hubos_app_vm_runtime_artifact_hash_t
  hubos_runtime_config_unikernel_net_artifact_hashes[] = {
    { "app.elf", "05b6e158ec5b8dc1a8d1a1c493d4b401ffd9328102b15593426086500d77ae8a" },
  };

static const hubos_app_vm_runtime_profile_t hubos_runtime_config_profiles_data[] = {
  {
    "linux-dev",
    HUBOS_APP_VM_GUEST_CLASS_FULL,
    HUBOS_APP_VM_OS_FAMILY_LINUX,
    "General-purpose Linux guest for broad app compatibility.",
    "1.0.0",
    "guest-managed",
    "src/runtime-images/linux-dev/1.0.0",
    hubos_runtime_config_linux_dev_source_bundles,
    sizeof(hubos_runtime_config_linux_dev_source_bundles) /
      sizeof(hubos_runtime_config_linux_dev_source_bundles[0]),
    hubos_runtime_config_linux_dev_artifact_hashes,
    sizeof(hubos_runtime_config_linux_dev_artifact_hashes) /
      sizeof(hubos_runtime_config_linux_dev_artifact_hashes[0]),
    {
      "src/runtime-images/linux-dev/1.0.0/kernel.elf",
      "src/runtime-images/linux-dev/1.0.0/initramfs.cpio.gz",
      "src/runtime-images/linux-dev/1.0.0/rootfs.img",
      NULL,
      "console=ttyS0 root=/dev/vda rw",
    },
    { 1024, 2, true, true, false },
  },
  {
    "mini-bsd-service",
    HUBOS_APP_VM_GUEST_CLASS_RUNTIME,
    HUBOS_APP_VM_OS_FAMILY_BSD,
    "Smaller BSD-style runtime for service-focused app VMs.",
    "1.0.0",
    "guest-managed",
    "src/runtime-images/mini-bsd-service/1.0.0",
    hubos_runtime_config_mini_bsd_service_source_bundles,
    sizeof(hubos_runtime_config_mini_bsd_service_source_bundles) /
      sizeof(hubos_runtime_config_mini_bsd_service_source_bundles[0]),
    hubos_runtime_config_mini_bsd_service_artifact_hashes,
    sizeof(hubos_runtime_config_mini_bsd_service_artifact_hashes) /
      sizeof(hubos_runtime_config_mini_bsd_service_artifact_hashes[0]),
    {
      "src/runtime-images/mini-bsd-service/1.0.0/kernel.elf",
      NULL,
      "src/runtime-images/mini-bsd-service/1.0.0/rootfs.img",
      NULL,
      "console=ttyS0",
    },
    { 256, 1, true, false, false },
  },
  {
    "unikernel-net",
    HUBOS_APP_VM_GUEST_CLASS_RUNTIME,
    HUBOS_APP_VM_OS_FAMILY_UNIKERNEL,
    "Lean runtime for network-oriented app appliances.",
    "1.0.0",
    "guest-managed",
    "src/runtime-images/unikernel-net/1.0.0",
    NULL,
    0,
    hubos_runtime_config_unikernel_net_artifact_hashes,
    sizeof(hubos_runtime_config_unikernel_net_artifact_hashes) /
      sizeof(hubos_runtime_config_unikernel_net_artifact_hashes[0]),
    {
      "src/runtime-images/unikernel-net/1.0.0/app.elf",
      NULL,
      NULL,
      NULL,
      "",
    },
    { 128, 1, true, false, false },
  },
};

static const char *const hubos_runtime_config_installed_profile_ids[] = {
  "linux-dev",
  "mini-bsd-service",
};

static const hubos_app_vm_runtime_selection_t hubos_runtime_config_default_selection_data = {
  hubos_runtime_config_profiles_data,
  sizeof(hubos_runtime_config_profiles_data) / sizeof(hubos_runtime_config_profiles_data[0]),
  hubos_runtime_config_installed_profile_ids,
  sizeof(hubos_runtime_config_installed_profile_ids) /
    sizeof(hubos_runtime_config_installed_profile_ids[0]),
  "linux-dev",
  NULL,
  0,
};

const hubos_app_vm_runtime_profile_t *hubos_runtime_config_profiles(size_t *out_count) {
  if (out_count != NULL) {
    *out_count = sizeof(hubos_runtime_config_profiles_data) /
                 sizeof(hubos_runtime_config_profiles_data[0]);
  }

  return hubos_runtime_config_profiles_data;
}

const hubos_app_vm_runtime_selection_t *hubos_runtime_config_default_selection(void) {
  return &hubos_runtime_config_default_selection_data;
}

const hubos_app_vm_runtime_profile_t *hubos_runtime_config_default_profile(void) {
  return hubos_app_vm_runtime_selection_resolve(&hubos_runtime_config_default_selection_data,
                                                HUBOS_ID_INVALID);
}
