#!/bin/sh

set -eu

transcript_file="${1:-${QEMU_TRANSCRIPT_FILE:-}}"

if [ -z "$transcript_file" ]; then
  echo "Usage: $0 TRANSCRIPT_FILE" >&2
  exit 1
fi

if [ ! -f "$transcript_file" ]; then
  echo "Missing Linux VM transcript: $transcript_file" >&2
  exit 1
fi

require_line() {
  pattern="$1"

  if ! grep -Fq "$pattern" "$transcript_file"; then
    echo "Missing Linux VM boot marker: $pattern" >&2
    exit 1
  fi
}

require_line 'Run /init as init process'
require_line 'with arguments:'
require_line 'with environment:'
require_line 'TERM=linux'

echo "Verified Linux VM transcript at $transcript_file"
