#!/bin/bash
# Run the S2M2 export container in the background.
# The container persists across logout (no --rm).
# Enter at any time with:
#   docker exec -it s2m2-export bash
# Stop with:
#   docker stop s2m2-export
#
# Usage:
#   ./docker/scripts/run_export.sh [--extra-args ...]

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

WEIGHTS_DIR="${SCRIPT_DIR}/alg-S2M2/weights/pretrain_weights"
CONTAINER_NAME="s2m2-export"

# Remove any previous container with this name (running or stopped)
if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Removing old container ${CONTAINER_NAME} ..."
    docker stop "${CONTAINER_NAME}" 2>/dev/null || true
    docker rm "${CONTAINER_NAME}" 2>/dev/null || true
fi

if [ ! -d "${WEIGHTS_DIR}" ]; then
    echo "WARNING: Pretrained weights not found at ${WEIGHTS_DIR}"
    echo "Mount your weights directory with -v /path/to/weights:/workspace/alg-S2M2/weights/pretrain_weights:ro"
    echo ""
fi

docker run --gpus all -d \
  --name "${CONTAINER_NAME}" \
  -v "${WEIGHTS_DIR}:/workspace/alg-S2M2/weights/pretrain_weights:ro" \
  -v s2m2_models:/workspace/alg-S2M2/weights/trt_save \
  s2m2-export sleep infinity

echo "Container ${CONTAINER_NAME} started in background."
echo "Enter with: docker exec -it ${CONTAINER_NAME} bash"
