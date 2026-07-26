#!/bin/bash
# Build the S2M2 inference container
# Usage: ./docker/scripts/build_inference.sh
# Run from the project root (thesis/).

set -e
# Resolve project root: works whether executed or sourced, from any cwd
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${SCRIPT_DIR}"
echo "Building from ${SCRIPT_DIR}"

# Sanity check: verify we're in the project root
if [ ! -d "alg-S2M2" ] || [ ! -d "cpp_ws" ]; then
    echo "ERROR: Could not find alg-S2M2/ or cpp_ws/. Run from project root or use:"
    echo "  bash docker/scripts/build_inference.sh"
    exit 1
fi

docker build \
  -t s2m2-inference:latest \
  -f docker/inference/Dockerfile \
  --build-arg CUDA_VERSION=13.0.3 \
  --build-arg CUDA_SHORT_VERSION=13.0 \
  --build-arg UBUNTU_VERSION=22.04 \
  --build-arg ROS_DISTRO=humble \
  --build-arg OPENCV_VERSION=4.13.0 \
  --build-arg TRT_VERSION=10.13.3.9 \
  "$@" .
