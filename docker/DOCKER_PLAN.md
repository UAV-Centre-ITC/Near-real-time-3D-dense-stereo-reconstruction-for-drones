# Docker Containerization Plan — S2M2 Stereo Vision Pipeline

## Architecture: Two-Container Design

```
┌──────────────────────────┐       shared volume       ┌──────────────────────────────┐
│   s2m2-export            │   ────────────────────>   │   s2m2-inference             │
│                          │   s2m2_models:/models     │                              │
│  Python / Conda (CUDA 12)│   (.engine files)         │  C++ / ROS2 / colcon         │
│  alg-S2M2/ source        │                           │  cpp_ws/ros2_impl/ source    │
│                          │                           │                              │
│  Builds .engine from     │                           │  Loads .engine at runtime    │
│  PyTorch -> ONNX -> TRT  │                           │  Runs stereo pipeline        │
└──────────────────────────┘                           └──────────────────────────────┘
```

---

## Container 1: `s2m2-export` (Python Export)

### Purpose
Export PyTorch S2M2 model → ONNX → TensorRT `.engine` file.

### Base Image
- `nvidia/cuda:13.0.3-cudnn-devel-ubuntu22.04`

### Dependencies

| Source | What | Why |
|---|---|---|
| System (CUDA 13) | `libcuda.so`, kernel driver interface | Required for GPU operations and trtexec |
| Conda env (from environment.yml) | `tensorrt-cu12==10.13.3`, `torch==2.9.1`, `onnx`, etc. | Python TRT bindings + PyTorch export |
| TensorRT Debian 10.13.3 | `trtexec` binary | CLI tool to build .engine from ONNX |
| System apt | `wget`, `git`, `build-essential` | Build tooling |

### Key Design Decision: Python TensorRT API only
- No system TensorRT packages installed — the `tensorrt-cu12` Python package from the conda env provides both the shared libraries and the Python API
- Engine export uses `docker/export/export_tensorrt.py` (pure Python, TensorRT API) instead of calling `trtexec` CLI
- Eliminates dependency on NVIDIA's TensorRT apt repo (which was unreachable from the build machine)
- CUDA 12 `cu12` packages coexist with system CUDA 13 via different SONAMEs

### Build Steps
1. Start from CUDA 13 base image
2. Install system packages (build-essential, wget, curl)
3. Install Miniconda
4. Create conda env from `alg-S2M2/environment.yml` (includes `tensorrt-cu12==10.13.3`)
5. Copy `alg-S2M2/` source code
6. Install the `s2m2` Python package (`pip install -e .`)
7. Copy `docker/export/export_tensorrt.py` (Python TRT export wrapper)

### Entrypoint
- `export [args]` — run Python-based TRT engine export
- `onnx [args]` — generate ONNX first
- `bash` — interactive shell inside conda env

---

## Container 2: `s2m2-inference` (C++ ROS2 Inference)

### Purpose
Run the full stereo inference pipeline: rectify → TRT inference → reproject → transform → filter → publish pointcloud.

### Base Image
- `nvidia/cuda:13.0.3-cudnn-devel-ubuntu22.04`

### Dependencies

| Source | What | Why |
|---|---|---|
| CUDA 13 toolkit | `nvcc`, `cudart`, CUDA headers | Compile `.cu` kernels, link CUDA runtime |
| TensorRT 10.13.3 (Debian) | `libnvinfer.so`, `NvInfer.h`, `nvinfer` | C++ TRT inference API |
| OpenCV 4.13 + contrib | Custom build with CUDA, installed at `/opt/opencv_cuda` | Rectification, GpuMat, cv_bridge |
| ROS2 Humble | `rclcpp`, `sensor_msgs`, `cv_bridge`, `image_transport`, etc. | ROS2 communication |
| System apt | `libeigen3-dev`, `libpcl-dev`, `libpcl-conversions-dev`, `nvml` | Linear algebra, pointcloud, GPU monitoring |
| `cpp_ws/ros2_impl/` source | All ROS2 packages built via colcon | The pipeline itself |

### OpenCV Build (matches user's existing command)
```bash
cmake -DCMAKE_BUILD_TYPE=RELEASE \
  -DCMAKE_INSTALL_PREFIX=/opt/opencv_cuda \
  -DOPENCV_EXTRA_MODULES_PATH=../../opencv_contrib-4.13.0/modules \
  -DWITH_CUDA=ON -DWITH_CUDNN=ON -DWITH_CUBLAS=ON \
  -DWITH_TBB=ON -DWITH_QT=OFF -DWITH_OPENGL=ON \
  -DOPENCV_DNN_CUDA=ON -DENABLE_FAST_MATH=ON -DCUDA_FAST_MATH=ON \
  -DCUDA_ARCH_BIN=8.6 -DCUDA_ARCH_PTX=8.6 \
  -DOPENCV_GENERATE_PKGCONFIG=ON -DOPENCV_ENABLE_NONFREE=ON \
  -DBUILD_EXAMPLES=OFF ..
```
> **Note:** `-DWITH_QT=OFF` in Docker (no display server); adjust if you need GUI features.

### Build Steps
1. Start from CUDA 13 base image
2. Install ROS2 Humble (apt)
3. Install system deps (eigen3, pcl, nvml, build tools)
4. Install TensorRT 10.13.3 Debian packages
5. Build OpenCV 4.13 with CUDA → `/opt/opencv_cuda`
6. Copy `cpp_ws/ros2_impl/` workspace
7. Build with colcon:
   ```bash
   colcon build --symlink-install \
     --allow-overriding cv_bridge image_geometry my_custom_interfaces \
     --packages-ignore opencv_tests \
     --cmake-args \
       -DOpenCV_DIR=/opt/opencv_cuda/lib/cmake/opencv4 \
       -DCUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda-13 \
       -DCMAKE_CUDA_COMPILER=/usr/local/cuda-13/bin/nvcc
   ```

### Entrypoint
- `source /workspace/install/setup.bash && ros2 launch stereo_system_bringup_pkg stereo_system.launch.py`

---

## Shared Volume: `s2m2_models`

| Property | Value |
|---|---|
| Volume name | `s2m2_models` |
| Mount in export | `/workspace/alg-S2M2/weights/trt_save/` |
| Mount in inference | `/models/` |
| Contents | `.engine` files (e.g., `S2M2_S_1248_1024_fp16.engine`) |

The inference container's `config/params.yaml` must be updated so `engine_filepath` points to `/models/S2M2_S_1248_1024_fp16.engine`.

---

## Execution Plan

### Prerequisites (on the GPU machine)

```bash
# Clone the repository (or copy it to the GPU machine)
git clone <repo-url> thesis
cd thesis
git checkout dockerized_ros2impl
```

### Step 1: Build the Export Container

```bash
# Using the helper script:
bash docker/scripts/build_export.sh

# Or manually:
docker build -t s2m2-export:latest \
  --build-arg CUDA_VERSION=13.0.3 \
  --build-arg UBUNTU_VERSION=22.04 \
  -f docker/export/Dockerfile .
```

**Estimated time:** ~15-20 min (conda env creation is the bulk).
**GPU required:** No (export needs GPU only at runtime).

### Step 2: Build the Inference Container

```bash
# Using the helper script:
bash docker/scripts/build_inference.sh

# Or manually:
docker build -t s2m2-inference:latest \
  --build-arg CUDA_VERSION=13.0.3 \
  --build-arg UBUNTU_VERSION=22.04 \
  --build-arg ROS_DISTRO=humble \
  --build-arg OPENCV_VERSION=4.13.0 \
  --build-arg TRT_VERSION=10.13.3.9 \
  -f docker/inference/Dockerfile .
```

**Estimated time:** ~45-60 min (OpenCV from source + ROS2 packages).
**GPU required:** No (build uses nvcc but doesn't need a GPU).

### Step 3: Create the Shared Volume

```bash
docker volume create s2m2_models
docker volume create s2m2_data   # optional, for profiling/rectified outputs
```

### Step 4a: Generate ONNX model

```bash
docker run --gpus all \
  -v s2m2_models:/workspace/alg-S2M2/weights/trt_save \
  s2m2-export onnx \
  --model_type S --img_width 1248 --img_height 1024
```

**GPU required:** Yes (runs PyTorch inference to trace the model).

### Step 4b: Build TensorRT Engine

```bash
docker run --gpus all \
  -v s2m2_models:/workspace/alg-S2M2/weights/trt_save \
  s2m2-export export \
  --model_type S --img_width 1248 --img_height 1024 --precision fp16
```

**GPU required:** Yes (TensorRT engine building needs GPU).
**Estimated time:** ~5-10 min depending on model size and GPU.

```bash
# Verify the engine was created:
docker run --rm -v s2m2_models:/models alpine ls -la /models/
# Expected: S2M2_S_1248_1024_fp16.engine
```

### Step 5: Run Inference

```bash
# Using docker-compose (recommended):
docker compose -f docker/docker-compose.yml up inference

# Or manually:
docker run --gpus all --network host \
  -v s2m2_models:/models:ro \
  -v s2m2_data:/data \
  -e ROS_DOMAIN_ID=0 \
  s2m2-inference
```

The container will:
1. Source ROS2 and colcon setup
2. Launch `stereo_system_bringup_pkg` which starts:
   - `s2m2_node` (main inference pipeline)
   - `pointcloud_generator` (converts to PointCloud2)
   - `gpu_monitoring_node` (NVML metrics)

### Debugging / Testing

```bash
# Interactive shell in export container:
docker run --gpus all -it --rm \
  -v s2m2_models:/workspace/alg-S2M2/weights/trt_save \
  s2m2-export bash

# Interactive shell in inference container:
docker run --gpus all -it --rm --network host \
  -v s2m2_models:/models:ro \
  s2m2-inference bash

# Test the inference node in GDB:
docker run --gpus all --network host \
  -v s2m2_models:/models:ro \
  s2m2-inference launch gdb_debug_s2m2_node:=true
```

### Maintenance: Rebuilding on Source Changes

```bash
# If only workspace code changes (no OpenCV/ROS changes):
docker build -t s2m2-inference:latest \
  -f docker/inference/Dockerfile . \
  --target final  # Not needed since there's only one target

# Actually, Docker caches layers. Just re-run the build:
# - OpenCV layer is cached (stage 1)
# - ROS2 layer is cached (stage 2)
# - Only the "COPY workspace + colcon build" layer is re-executed
bash docker/scripts/build_inference.sh
```

### File Transfer (if building on a different machine)

If the GPU machine doesn't have direct access to the git repository:

```bash
# On dev machine — create a source tarball:
cd /home/geo/thesis
tar czf thesis_docker.tar.gz \
  alg-S2M2/environment.yml alg-S2M2/demo alg-S2M2/pyproject.toml alg-S2M2/src \
  cpp_ws/ros2_impl \
  docker/

# Copy to GPU machine and extract, then run the build steps above.
```

---

## Open Issues / Risks

| Risk | Mitigation |
|---|---|
| **CUDA 13 base image** — confirmed available as `nvidia/cuda:13.0.3-cudnn-devel-ubuntu22.04` | Use verified tag |
| **OpenCV CMAKE_CUDA_COMPILER detection for CUDA 13** — OpenCV 4.13's CMake may not auto-detect CUDA 13 | Verified working — user's host build succeeds with same flags. Use `-DCUDA_TOOLKIT_ROOT_DIR` and `-DCMAKE_CUDA_COMPILER` explicitly |
| **cv_bridge OpenCV linkage** — ROS2's cv_bridge links system OpenCV by default | Use `--allow-overriding cv_bridge image_geometry` in colcon build to force linkage against `/opt/opencv_cuda` |
| **TensorRT cu12 + CUDA 13 coexistence** — System CUDA 13 vs conda CUDA 12 libs | Verified by existing host setup; libs have different SONAMEs so no symbol conflict |
| **QT off in Docker** — `-DWITH_QT=OFF` removes highgui window display | Inference is headless; if visualization is needed, use X11 forwarding or offscreen rendering |
| **trtexec not available in Docker** — NVIDIA TensorRT apt repo unreachable | Replaced with pure Python export script using `tensorrt` Python API (from conda env) |
