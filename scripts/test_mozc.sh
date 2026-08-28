#!/usr/bin/env bash
#
# scripts/test_mozc.sh
# Run Mozc Kana-Kanji Conversion & TIP Unit Test Suite
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${ROOT_DIR}"
make test-mozc
