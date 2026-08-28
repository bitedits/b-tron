#!/usr/bin/env bash
# ==============================================================================
# B-TRON T-Editor UI & Internal Functions Test Runner
# ==============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${ROOT_DIR}"

echo "=========================================================="
echo " Building and Running B-TRON T-Editor Unit Test Suite..."
echo "=========================================================="

make test-editor

echo "All T-Editor UI internal tests passed successfully."
