#!/bin/sh

set -eu

transcript_file="${1:-${QEMU_TRANSCRIPT_FILE:-}}"
expect_linux_vm="${HUBOS_EXPECT_LINUX_VM:-0}"

if [ -z "$transcript_file" ]; then
  echo "Usage: $0 TRANSCRIPT_FILE" >&2
  exit 1
fi

if [ ! -f "$transcript_file" ]; then
  echo "Missing HubOS QEMU transcript: $transcript_file" >&2
  exit 1
fi

require_line() {
  pattern="$1"

  if ! grep -Fq "$pattern" "$transcript_file"; then
    echo "Missing HubOS boot marker: $pattern" >&2
    exit 1
  fi
}

require_line 'Booting all finished, dropped to user space'
require_line 'MON|INFO: Microkit Monitor started!'
require_line 'VM Server: entering init'
require_line 'VM Server: runtime profile=linux-dev'
require_line 'VM Server: boot complete'
require_line 'Linux VM: control-plane startup confirmed'
require_line 'Root Task: entering init'
require_line 'Root Task: init complete'
require_line 'Resource Registry: init complete'
require_line 'Capability Manager: init complete'
require_line 'Session Manager: init complete'
require_line 'Hub: init complete'
require_line 'Memory Manager: init complete'
require_line 'DMA Manager: init complete'
require_line 'Driver Registry: init complete'
require_line 'Driver Loader: init complete'
require_line 'Driver Service: init complete'
require_line 'Network Server: init complete'
require_line 'Device Server: init'

if [ "$expect_linux_vm" = "1" ]; then
  require_line 'Linux VM: Buildroot guest profile selected'
fi

echo "Verified HubOS QEMU transcript at $transcript_file"
