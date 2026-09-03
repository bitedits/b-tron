#!/usr/bin/env bash
#
# scripts/fetch_tibetan_refs.sh
# Fetch and setup third-party reference sources for B-TRON TIP / Tibetan IME, Dictionaries & Fonts
# Canonical Reference Engines:
#  - BDRC / OpenPecha Botok (Tibetan NLP & Trie Dictionary Engine)
#  - BDRC pyewts (Extended Wylie Transliteration Scheme)
#  - Rangjung Yeshe / Ives Waldo / Hopkins Tibetan-English Dictionaries
#  - Jomolhari & Tibetan OpenType Fonts (by Christopher Fynn / BDRC)
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
THIRD_PARTY_DIR="${ROOT_DIR}/third_party"
TIBETAN_DIR="${THIRD_PARTY_DIR}/tibetan"
FONTS_DIR="${TIBETAN_DIR}/fonts"

echo "=========================================================="
echo " [B-TRON TIP] Fetching Tibetan Third-Party Reference Sources"
echo " Dest    : ${TIBETAN_DIR}"
echo "=========================================================="

mkdir -p "${TIBETAN_DIR}"
mkdir -p "${FONTS_DIR}"

# 1. BDRC pyewts
PYEWTS_DIR="${TIBETAN_DIR}/pyewts"
if [ -d "${PYEWTS_DIR}/.git" ]; then
    echo "[INFO] pyewts already fetched."
else
    echo "[INFO] Fetching BDRC pyewts (shallow clone)..."
    git clone --depth 1 https://github.com/BuddhistDigitalResourceCenter/pyewts.git "${PYEWTS_DIR}" 2>/dev/null || echo "[WARN] pyewts remote clone skipped or offline."
fi

# 2. OpenPecha Botok
BOTOK_DIR="${TIBETAN_DIR}/botok"
if [ -d "${BOTOK_DIR}/.git" ]; then
    echo "[INFO] botok already fetched."
else
    echo "[INFO] Fetching OpenPecha botok (shallow clone)..."
    git clone --depth 1 https://github.com/OpenPecha/botok.git "${BOTOK_DIR}" 2>/dev/null || echo "[WARN] botok remote clone skipped or offline."
fi

# 3. Esukhia / Rangjung Yeshe Tibetan-English Dictionaries
DICT_DIR="${TIBETAN_DIR}/dictionaries"
if [ -d "${DICT_DIR}/.git" ]; then
    echo "[INFO] Tibetan-English dictionaries already fetched."
else
    echo "[INFO] Fetching Esukhia / Rangjung Yeshe Tibetan Dictionaries (shallow clone)..."
    git clone --depth 1 https://github.com/esukhia/dictionaries.git "${DICT_DIR}" 2>/dev/null || echo "[WARN] dictionaries remote clone skipped or offline."
fi

# 4. Jomolhari Tibetan Canonical Fonts
if [ -f "${FONTS_DIR}/Jomolhari.ttf" ]; then
    echo "[INFO] Jomolhari font already fetched."
else
    echo "[INFO] Fetching Jomolhari OpenType font..."
    curl -s -L "https://raw.githubusercontent.com/OpenPecha/tibetan-fonts/master/fonts/Jomolhari.ttf" -o "${FONTS_DIR}/Jomolhari.ttf" 2>/dev/null || echo "[WARN] Jomolhari download skipped or offline."
fi

echo "=========================================================="
echo " [B-TRON TIP] Tibetan Reference Sources & Fonts Complete!"
echo "=========================================================="
