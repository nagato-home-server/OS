#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
remote_url="${GITHUB_REMOTE_URL:-https://github.com/nagato-home-server/OS.git}"
git_user_name="${GIT_USER_NAME:-nagato-home-server}"
git_user_email="${GIT_USER_EMAIL:-nagato-home-server@users.noreply.github.com}"
phase="${1:-all}"

usage() {
  cat <<'EOF'
Usage: scripts/push-github-import.sh [core|upstream|runtime|all]

Phases:
  core      Initialize repo metadata, commit core sources, push main
  upstream  Commit src/hubos-upstream and push
  runtime   Commit src/runtime-src and push
  all       Run core, then upstream, then runtime
EOF
}

ensure_repo() {
  git -C "$repo_root" rev-parse --is-inside-work-tree >/dev/null 2>&1 || git -C "$repo_root" init
  git -C "$repo_root" config user.name "$git_user_name"
  git -C "$repo_root" config user.email "$git_user_email"

  if git -C "$repo_root" remote get-url origin >/dev/null 2>&1; then
    git -C "$repo_root" remote set-url origin "$remote_url"
  else
    git -C "$repo_root" remote add origin "$remote_url"
  fi

  git -C "$repo_root" branch -M main
}

commit_if_needed() {
  message="$1"
  if ! git -C "$repo_root" diff --cached --quiet; then
    git -C "$repo_root" commit -m "$message"
  else
    printf '%s\n' "No staged changes for: $message"
  fi
}

run_core() {
  ensure_repo

  git -C "$repo_root" add \
    .gitattributes \
    .gitignore \
    CMakeLists.txt \
    Makefile \
    README \
    boards \
    config \
    docs \
    include \
    install.sh \
    microkit-generated \
    qemu-workspace \
    scripts \
    seL4test-manifest \
    tests \
    vm

  git -C "$repo_root" add \
    src/README.bundled-assets.md \
    src/audit.c \
    src/boot.c \
    src/bus_manager.c \
    src/capability_manager.c \
    src/device_server.c \
    src/dma_manager.c \
    src/driver_loader.c \
    src/driver_registry.c \
    src/driver_service.c \
    src/hub.c \
    src/ipc.c \
    src/linux_usbio_backend.c \
    src/memory_manager.c \
    src/microkit_boot.c \
    src/microkit_endpoint.c \
    src/microkit_generated.c \
    src/microkit_graph.c \
    src/microkit_ipc.c \
    src/microkit_kernel_glue.c \
    src/microkit_runtime.c \
    src/model.c \
    src/network_server.c \
    src/qemu_root_task_runtime.c \
    src/resource_registry.c \
    src/root_task.c \
    src/runtime_config.c \
    src/service_endpoints.c \
    src/session_manager.c \
    src/sha256.c \
    src/system.c

  git -C "$repo_root" add \
    src/image-metadata \
    src/rollback-images \
    src/runtime-images \
    src/tool-src

  commit_if_needed "Initial core import"
  git -C "$repo_root" push -u origin main
}

run_upstream() {
  ensure_repo
  git -C "$repo_root" add src/hubos-upstream
  commit_if_needed "Add vendored upstream sources"
  git -C "$repo_root" push
}

run_runtime() {
  ensure_repo
  git -C "$repo_root" add src/runtime-src
  commit_if_needed "Add runtime source bundles"
  git -C "$repo_root" push
}

case "$phase" in
  core)
    run_core
    ;;
  upstream)
    run_upstream
    ;;
  runtime)
    run_runtime
    ;;
  all)
    run_core
    run_upstream
    run_runtime
    ;;
  -h|--help|help)
    usage
    ;;
  *)
    usage >&2
    exit 1
    ;;
esac
