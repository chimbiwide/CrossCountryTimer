#!/usr/bin/env bash
set -euo pipefail

dir="$(cd "$(dirname "$0")" && pwd)/models"
mkdir -p "$dir"

download() {
    local url="$1" file="$2" sha256="$3"
    local path="$dir/$file"

    if [ -f "$path" ] && echo "$sha256  $path" | sha256sum --check --status; then
        echo "$file is already there"
        return
    fi

    echo "Downloading $file"
    if command -v curl >/dev/null; then
        curl -L --fail --progress-bar -o "$path.part" "$url/$file"
    else
        wget -q --show-progress -O "$path.part" "$url/$file"
    fi

    if ! echo "$sha256  $path.part" | sha256sum --check --status; then
        rm -f "$path.part"
        echo "$file does not match its SHA256, so it was deleted" >&2
        exit 1
    fi
    mv "$path.part" "$path"
}

download https://huggingface.co/mradermacher/GLM-OCR-GGUF/resolve/main \
    GLM-OCR.Q4_K_M.gguf bb27f2451f0f1b13491e04e0664cd946438a4677c32acf8ecfef79f68e862de1
download https://huggingface.co/ggml-org/GLM-OCR-GGUF/resolve/main \
    mmproj-GLM-OCR-Q8_0.gguf 9c4b58e33e316ed142eb5dcb41abec3844d3e6e5dc361ffb782c3fa9d175141f
