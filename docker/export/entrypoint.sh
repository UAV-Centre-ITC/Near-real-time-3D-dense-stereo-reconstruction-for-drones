#!/bin/bash
set -e

source $(conda info --base)/etc/profile.d/conda.sh
conda activate s2m2

# If called with a typical subcommand, dispatch as before.
# Otherwise exec whatever was passed (e.g., "sleep infinity" for background mode).
cmd="${1:-}"

case "$cmd" in
    export|onnx|--help|-h)
        if [[ "$cmd" == "--help" ]] || [[ "$cmd" == "-h" ]]; then
            exec python /workspace/export_tensorrt.py --help
        fi
        shift
        exec python /workspace/export_tensorrt.py "$@"
        ;;
    bash)
        exec bash
        ;;
    "")
        # No arguments → default to export
        exec python /workspace/export_tensorrt.py
        ;;
    *)
        # Pass through (e.g., "sleep infinity" from -d mode)
        exec "$@"
        ;;
esac
