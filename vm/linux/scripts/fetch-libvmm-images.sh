#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/../../../" && pwd)"
arch="${LIBVMM_GUEST_ARCH:-x86_64}"
out_dir="${LIBVMM_IMAGE_OUT:-$repo_root/build/vm/linux/libvmm-$arch}"

mkdir -p "$out_dir"

download() {
  url="$1"
  output="$2"

  if [ -f "$output" ]; then
    return 0
  fi

  curl -L --fail --retry 3 --retry-delay 1 "$url" -o "$output"
}

case "$arch" in
  x86_64)
    linux_name="${LIBVMM_LINUX_NAME:-be4206493bcc7234a8713319b7c6280fa04f9c5a-bzImage}"
    initrd_name="${LIBVMM_INITRD_NAME:-d887a642236a92610a9537ab9f4a4aa1a966ad3a-rootfs.cpio.gz}"
    linux_url="https://trustworthy.systems/Downloads/libvmm/images/${linux_name}.tar.gz"
    initrd_url="https://trustworthy.systems/Downloads/libvmm/images/${initrd_name}.tar.gz"
    linux_tar="$out_dir/${linux_name}.tar.gz"
    initrd_tar="$out_dir/${initrd_name}.tar.gz"

    download "$linux_url" "$linux_tar"
    download "$initrd_url" "$initrd_tar"

    tar -xf "$linux_tar" -C "$out_dir"
    tar -xf "$initrd_tar" -C "$out_dir"

    linux_dir="$out_dir/$linux_name"
    initrd_dir="$out_dir/$initrd_name"

    if [ -f "$linux_dir/bzImage" ]; then
      cp "$linux_dir/bzImage" "$out_dir/bzImage"
    elif [ -f "$linux_dir/linux" ]; then
      cp "$linux_dir/linux" "$out_dir/bzImage"
    fi

    if [ -f "$initrd_dir/rootfs.cpio.gz" ]; then
      cp "$initrd_dir/rootfs.cpio.gz" "$out_dir/rootfs.cpio.gz"
    fi
    ;;
  aarch64)
    linux_name="${LIBVMM_LINUX_NAME:-85000f3f42a882e4476e57003d53f2bbec8262b0-linux}"
    initrd_name="${LIBVMM_INITRD_NAME:-6dcd1debf64e6d69b178cd0f46b8c4ae7cebe2a5-rootfs.cpio.gz}"
    linux_url="https://trustworthy.systems/Downloads/libvmm/images/${linux_name}.tar.gz"
    initrd_url="https://trustworthy.systems/Downloads/libvmm/images/${initrd_name}.tar.gz"
    linux_tar="$out_dir/${linux_name}.tar.gz"
    initrd_tar="$out_dir/${initrd_name}.tar.gz"

    download "$linux_url" "$linux_tar"
    download "$initrd_url" "$initrd_tar"

    tar -xf "$linux_tar" -C "$out_dir"
    tar -xf "$initrd_tar" -C "$out_dir"

    linux_dir="$out_dir/$linux_name"
    initrd_dir="$out_dir/$initrd_name"

    if [ -f "$linux_dir/linux" ]; then
      cp "$linux_dir/linux" "$out_dir/linux"
    elif [ -f "$linux_dir/Image" ]; then
      cp "$linux_dir/Image" "$out_dir/linux"
    fi

    if [ -f "$initrd_dir/rootfs.cpio.gz" ]; then
      cp "$initrd_dir/rootfs.cpio.gz" "$out_dir/rootfs.cpio.gz"
    fi
    ;;
  *)
    echo "Unsupported LIBVMM_GUEST_ARCH: $arch" >&2
    exit 1
    ;;
esac

printf '%s\n' "$out_dir"
