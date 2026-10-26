#!/bin/sh
# Downloads the whisper tiny.en model into this folder. Configuring with
# CMake does the same, so this is only for fetching it by hand.
dir="$(cd "$(dirname "$0")" && pwd)"
exec "$dir/../third_party/whisper.cpp/models/download-ggml-model.sh" base.en "$dir"
