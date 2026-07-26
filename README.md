# Near real-time 3D stereo reconstruction from drone imagery and SLAM pose estimations - Branch: fast_foundation

This branch uses another state-of-the-art stereo vision model that came out during the thesis work, from NVIDIA. This branch was created with the sole purpose to test a different optimized model and do a comparison. 

**[Fast-FoundationStereo](https://github.com/NVlabs/Fast-FoundationStereo)** (CVPR 2026) : It achieves strong zero-shot generalization at real-time speeds through knowledge distillation, neural architecture search, and structured pruning of the [FoundationStereo](https://github.com/NVlabs/FoundationStereo) backbone. The model outputs a single dense disparity map without occlusion or confidence auxiliaries.

## Overview

The pipeline consists of:

1. **Keyframe selection** -- incoming drone images + absolute poses (from SLAM) are evaluated; successive frames with sufficient horizontal translation but limited rotation/Z-displacement form a stereo pair.
2. **Stereo rectification** -- the relative rotation/translation between the two keyframes is used to rectify the images so epipolar lines are horizontal.
3. **Image splitting** -- the full-resolution rectified pair is split into 4 overlapping patches for the model's fixed 1024x1248 input size.
4. **Inference** -- each patch is padded to the model's expected resolution, normalized with ImageNet statistics, and run through a single TensorRT engine built from the Fast-FoundationStereo ONNX export.
5. **Reprojection + transformation** -- the disparity map is reprojected to 3D camera-frame points, then transformed to the world coordinate frame using the left camera's SLAM pose.
6. **Point-cloud publishing** -- the world-frame 3D points are published as a ROS2 `Image` (32FC4 encoding) for downstream consumption (point-cloud generation, mapping, etc.).

## Requirements

| Component           | Version / Details                                                      |
| ------------------- | ---------------------------------------------------------------------- |
| CUDA                | 12.x or 13.x                                                           |
| TensorRT            | 10.11.0+ (builds and runtime must match; 10.13.3 tested)              |
| OpenCV              | CUDA-enabled build (`/opt/opencv_cuda`)                                |
| ROS2                | Humble                                                                |
| GPU                 | NVIDIA A40 (or any SM 8.0+ GPU); 2x A40 available on this server      |
| Python              | 3.10+ (for ONNX export only)                                          |

## Repository structure

```
thesis/
  cpp_ws/ros2_impl/
    ffs_inference_cpp_pkg/     # Main stereo inference node (ROS2 + TensorRT)
      config/params.yaml       # Runtime parameters
      include/                 # Headers (ffs_node.hpp, cuda_kernels, etc.)
      launch/                  # Node launch file
      rectification/           # Stereo rectification library
      src/
        ffs_node.cpp           # Main node: keyframe logic, preprocessing, pipeline
        cuda_kernels.cu        # GPU kernels (reprojectTo3D, transform WC)
        image_helpers.cpp      # Disparity saving, visualization
    pointcloud_pkg/            # Point-cloud accumulation/generation node
    stereo_system_bringup_pkg/ # Master launch file for the full stereo pipeline
    gpu_monitoring_pkg/        # GPU utilization monitor node
    updated_build_command.sh   # Colcon build command with CUDA/TensorRT paths
  Fast-FoundationStereo/       # NVIDIA's official repository (ONNX export + plugin)
    scripts/
      make_single_onnx.py      # Export as single ONNX (no plugin, ImageNet norm stripped)
      make_plugin_onnx.py      # Export with FFSGWCVolume TRT plugin node
    cpp/                       # C++ plugin library + end-to-end inference apps
      src/gwc_volume_plugin.cpp
      include/ffs_gwc_plugin.hpp
    weights/                   # Model checkpoints (from NVIDIA)
    docker/                    # Dockerfiles for build environment (optional)
```

## Setup

### 1. Clone the NVIDIA Fast-FoundationStereo repository

```bash
cd /home/s3488977
git clone https://github.com/NVlabs/Fast-FoundationStereo.git
cd Fast-FoundationStereo
```

### 2. Download model weights

Download from the [official Google Drive](https://drive.google.com/drive/folders/1HuTt7UIp7gQsMiDvJwVuWmKpvFzIIMap) and place under `weights/`. The `23-36-37` checkpoint is recommended for the best accuracy-vs-speed trade-off.

```
weights/23-36-37/model_best_bp2_serialize.pth
```

### 3. Set up the build environment (Docker)

The ONNX export and plugin compilation require a Python environment with PyTorch, CUDA runtime, and TensorRT headers. Rather than installing these directly (which can conflict with the server's CUDA/TensorRT versions), we use a slim Docker container based on `docker/dockerfile_fixed`.

This Dockerfile installs PyTorch 2.6, ONNX tooling, and `nvidia-modelopt` inside a CUDA 12.4 base image, **without** installing TensorRT system packages (the engine is built on the host to match its TRT 10.13.3 installation).

#### 3a. Build the Docker image

```bash
cd /home/s3488977/Fast-FoundationStereo
docker build --network host -t ffs -f docker/dockerfile_fixed .
```

#### 3b. Enter the container

Use `docker/updated_run_container.sh` which adds `--env NVIDIA_DISABLE_REQUIRE=1` to bypass NVIDIA driver version checks (the host's driver is newer than what CUDA 12.4 base image expects):

```bash
bash docker/updated_run_container.sh
```

This mounts `/home` into the container, giving access to your home directory.

> **Alternative — conda environment**: If you prefer not to use Docker, create a conda environment with Python 3.10+ and install `torch==2.6.0`, `torchvision==0.21.0`, `onnx`, `onnxruntime-gpu`, and all packages from `requirements.txt`. The engine build still happens on the host.

### 4. Export the plugin ONNX (inside Docker)

TRT 10.13.3 has a known issue where the Cask Pooling Runner crashes on `AveragePool` nodes present in the standard ONNX export. The **plugin ONNX** export (`make_plugin_onnx.py`) works around this by replacing problematic pooling operations with conv-based equivalents and embedding a `FFSGWCVolume` TensorRT plugin node.

From inside the Docker container:

```bash
conda activate my
cd /workspace/Fast-FoundationStereo/scripts

python3 make_plugin_onnx.py \
    --model_dir ../weights/23-36-37/model_best_bp2_serialize.pth \
    --save_path ../my_engine_output \
    --height 1024 --width 1248 \
    --valid_iters 8 --max_disp 192
```

This produces:
- `my_engine_output/fast_foundationstereo_plugin.onnx`
- `my_engine_output/onnx.yaml`

> **Important**: The ONNX model strips ImageNet normalization from the graph. The C++ node handles normalization in `normalizeAndConvertToNCHW()`. The model expects `(img / 255.0 - mean) / std` with `mean=[0.485, 0.456, 0.406]` and `std=[0.229, 0.224, 0.225]`.

### 5. Build the FFSGWCVolume plugin and TensorRT engine

The following two steps run **on the host** (outside Docker), since the host has TensorRT 10.13.3 installed directly and `trtexec` available in PATH.

#### 5a. Compile the plugin shared library

```bash
cd /home/s3488977/Fast-FoundationStereo/cpp
cmake -B build
cmake --build build -j
```

This compiles `build/libffs_gwc_plugin.so` — needed at both engine build time and runtime.

#### 5b. Build the TensorRT engine with trtexec

```bash
trtexec --onnx=/home/s3488977/Fast-FoundationStereo/my_engine_output/fast_foundationstereo_plugin.onnx \
        --saveEngine=/home/s3488977/Fast-FoundationStereo/my_engine_output/fast_foundationstereo.engine \
        --fp16 \
        --plugins=/home/s3488977/Fast-FoundationStereo/cpp/build/libffs_gwc_plugin.so
```

> **Note**: If `trtexec` is not found, source your TensorRT environment first. Common locations: `/usr/src/tensorrt/bin/trtexec`, `/usr/local/cuda/bin/trtexec`.

#### 5c. Engine output

After successful build, `my_engine_output/` should contain:

```
my_engine_output/
  fast_foundationstereo_plugin.onnx   (intermediate, not needed at runtime)
  fast_foundationstereo.engine        (the TRT engine)
  onnx.yaml                           (config: image size, max_disp, etc.)
```

### 6. System dependencies (ROS2 + CUDA + OpenCV)

This step is already done on the server. If setting up from scratch:

- ROS2 Humble: follow [official instructions](https://docs.ros.org/en/humble/Installation.html)
- CUDA 12.x/13.x: installed via system package manager
- TensorRT 10.13.3: installed in `/usr/lib/x86_64-linux-gnu/`
- OpenCV with CUDA: built from source into `/opt/opencv_cuda/`
- Install `ros-humble-cv-bridge`, `ros-humble-image-transport`, `ros-humble-image-transport-plugins`

### 7. Build the ROS2 workspace

```bash
cd /home/s3488977/thesis/cpp_ws/ros2_impl
source /opt/ros/humble/setup.bash
bash updated_build_command.sh
```

This builds all packages: `ffs_inference_cpp_pkg`, `pointcloud_pkg`, `stereo_system_bringup_pkg`, `gpu_monitoring_pkg`.

### 8. Configure parameters

Edit `ffs_inference_cpp_pkg/config/params.yaml`:

```yaml
ffs_node:
  ros__parameters:
    engine_filepath: "/home/s3488977/Fast-FoundationStereo/my_engine_output/fast_foundationstereo.engine"
    plugin_filepath: "/home/s3488977/Fast-FoundationStereo/cpp/build/libffs_gwc_plugin.so"
    image_topic: "/keyframe_image"
    absolute_pose_topic: "/keyframe_pose"
    min_keyframe_movement: 7.0      # min horizontal translation (meters) for stereo pair
    max_keyframe_angular: 0.15      # max rotation angle (radians) before reset
    max_keyframe_Z: 0.35            # max vertical displacement (meters) before reset
    model_input_height: 1024
    model_input_width: 1248
    save_rectified_images: true
    use_sim_time: true
```

| Parameter                  | Description                                                          |
| -------------------------- | -------------------------------------------------------------------- |
| `engine_filepath`          | Path to the TensorRT engine file                                     |
| `plugin_filepath`          | Path to `libffs_gwc_plugin.so`                                       |
| `image_topic`              | ROS2 topic for incoming keyframe images (raw, not compressed)        |
| `absolute_pose_topic`      | ROS2 topic for SLAM absolute poses (PoseStamped)                     |
| `min_keyframe_movement`    | Minimum forward (X) translation to trigger a stereo pair             |
| `max_keyframe_angular`     | Maximum angular change before keyframe reset                         |
| `max_keyframe_Z`           | Maximum Z (up/down) displacement before keyframe reset               |
| `model_input_height/width` | Model input resolution (must be divisible by 32)                     |
| `save_rectified_images`    | Save rectified image pairs to disk for debugging                     |
| `use_sim_time`             | Use ROS2 simulation time (set to `true` for rosbag playback)         |

### 9. Camera calibration

The camera intrinsics and distortion coefficients are hardcoded in `ffs_inference_cpp_pkg/include/cam_params_rosbag.hpp`. Update these values to match your specific camera before building. The current values are for a Pix4D dataset.

## Running

### Full pipeline launch

```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 launch stereo_system_bringup_pkg stereo_system.launch.py
```

This starts:
- `ffs_node` -- the Fast-FoundationStereo inference node
- `pointcloud_generator` -- accumulates 3D points into colored point clouds
- `gpu_monitoring_node` -- publishes GPU utilization metrics

### Running with rosbag playback

```bash
# Terminal 1: launch the stereo pipeline
ros2 launch stereo_system_bringup_pkg stereo_system.launch.py

# Terminal 2: play back recorded data
ros2 bag play <path_to_bag> --clock
```

The node subscribes to `/keyframe_image` (sensor_msgs/Image, raw encoding) and `/keyframe_pose` (geometry_msgs/PoseStamped), and publishes:
- `/stereo/points3d` -- world-frame 3D points (32FC4 encoding: X, Y, Z, 1)
- `/stereo/left/processed` -- processed left image (for pointcloud coloring)

## Key differences from the S2M2 branch

- **Single output**: FFS outputs only a disparity map. There is no occlusion mask or confidence map, so the occlusion/confidence-based point filtering used in S2M2 is not available. All valid disparity pixels are reprojected.
- **ImageNet normalization**: FFS requires pre-normalized float32 inputs. The C++ node applies `(pixel/255 - mean) / std` with ImageNet statistics in `normalizeAndConvertToNCHW()`.
- **Plugin engine**: FFS uses the `FFSGWCVolume` TensorRT plugin (compiled from `cpp/` in the NVIDIA repo) instead of the S2M2's plain ONNX+trtexec export. The `libffs_gwc_plugin.so` must be loaded at runtime via `dlopen()`.
- **Tensor names**: The engine uses tensor names `"left"`, `"right"`, `"disp"` (matching the plugin ONNX export's `input_names`/`output_names`).
- **Image encoding**: Input images are converted to `CV_32FC3` immediately in `setLeftRightImages()` (S2M2 kept them as `rgb8` uint8).
- **4-patch inference**: Full-resolution images are split into 4 patches, each processed independently through the fixed-resolution model, then recombined during reprojection.

## Troubleshooting

### Engine build crashes on AveragePool_1

This is a known TensorRT 10.13 bug. Use the plugin ONNX export (`make_plugin_onnx.py`) instead of `make_single_onnx.py`. The plugin ONNX replaces problematic pooling operations with conv-based equivalents.

### Permission denied on output files from Docker

If files in the NVIDIA repo's `output/` directory are owned by root (from the Docker container), write engine/model files to your home directory instead, then update `params.yaml` paths accordingly.

### trtexec not found

Locate `trtexec` by sourcing your TensorRT environment or use the full path:

```bash
# Check common locations:
ls /usr/src/tensorrt/bin/trtexec
ls /usr/local/TensorRT-*/bin/trtexec
which trtexec

# Or find it:
find /usr -name trtexec 2>/dev/null
```

### Plugin fails to load at runtime

Error: `Failed to load FFSGWCVolume plugin`. Verify:
1. `plugin_filepath` in `params.yaml` points to the correct `.so` file
2. The `.so` was compiled for the same architecture as the host
3. `libffs_gwc_plugin.so` is linked against the correct TensorRT version

## Acknowledgements

This project uses **[Fast-FoundationStereo](https://github.com/NVlabs/Fast-FoundationStereo)** by Bowen Wen, Shaurya Dewan, and Stan Birchfield (NVIDIA, CVPR 2026).

```bibtex
@article{wen2026fastfoundationstereo,
  title={{Fast-FoundationStereo}: Real-Time Zero-Shot Stereo Matching},
  author={Bowen Wen and Shaurya Dewan and Stan Birchfield},
  journal={CVPR},
  year={2026}
}
```
