#!/usr/bin/env bash
#
# scripts/third_parties.sh
# Fetch and setup third-party dependencies for B-TRON TIP / IME (Google Mozc)
# Target Version: 3.34.6239 (Stable)
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
THIRD_PARTY_DIR="${ROOT_DIR}/third_party"
MOZC_DIR="${THIRD_PARTY_DIR}/mozc"
MOZC_TAG="3.34.6239"
MOZC_REPO="https://github.com/google/mozc.git"

echo "=========================================================="
echo " [B-TRON TIP] Fetching Third-Party Dependencies"
echo " Package : Google Mozc (${MOZC_TAG})"
echo " Repo    : ${MOZC_REPO}"
echo " Dest    : ${MOZC_DIR}"
echo "=========================================================="

mkdir -p "${THIRD_PARTY_DIR}"

if [ -d "${MOZC_DIR}/src/dictionary" ] && [ -d "${MOZC_DIR}/src/converter" ]; then
    echo "[INFO] Mozc tree already exists and is validated."
else
    echo "[INFO] Fetching Mozc tag ${MOZC_TAG} (shallow clone)..."
    rm -rf "${MOZC_DIR}"
    git clone --depth 1 --branch "${MOZC_TAG}" "${MOZC_REPO}" "${MOZC_DIR}"
fi

echo "[INFO] Validating Mozc tree structure..."
test -d "${MOZC_DIR}/src/dictionary" || { echo "[ERROR] Missing dictionary directory"; exit 1; }
test -d "${MOZC_DIR}/src/converter" || { echo "[ERROR] Missing converter directory"; exit 1; }
test -d "${MOZC_DIR}/src/composer" || { echo "[ERROR] Missing composer directory"; exit 1; }
test -d "${MOZC_DIR}/src/data" || { echo "[ERROR] Missing data directory"; exit 1; }

echo "=========================================================="
echo " [B-TRON TIP] Mozc ${MOZC_TAG} successfully fetched and verified!"
echo "=========================================================="
