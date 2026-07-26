#!/bin/bash
# Run the S2M2 inference container in the background.
# The container persists across logout (no --rm).
# Enter at any time with:
#   docker exec -it s2m2-inference bash
# Stop with:
#   docker stop s2m2-inference
#
# Usage:
#   ./docker/scripts/run_inference.sh [--extra-args ...]

set -e

CONTAINER_NAME="s2m2-inference"

# Remove any previous container with this name (running or stopped)
if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Removing old container ${CONTAINER_NAME} ..."
    docker stop "${CONTAINER_NAME}" 2>/dev/null || true
    docker rm "${CONTAINER_NAME}" 2>/dev/null || true
fi

docker run --gpus all --network host --ipc=host -d \
  --name "${CONTAINER_NAME}" \
  --user "$(id -u):$(id -g)" \
  -v s2m2_models:/models:ro \
  -v s2m2_data:/data \
  -e ROS_DOMAIN_ID="${ROS_DOMAIN_ID:-0}" \
  -e HOME=/workspace \
  s2m2-inference sleep infinity

echo "Container ${CONTAINER_NAME} started in background."
echo "Enter with: docker exec -it ${CONTAINER_NAME} bash"
