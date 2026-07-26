# s2m2-inference — C++/ROS2 stereo inference pipeline
# Runs: rectify → TRT inference → reproject → transform → filter → publish
#
# Build:
#   docker build -t s2m2-inference:latest -f docker/inference/Dockerfile .
#
# Usage:
#   docker run --gpus all --network host \
#     -v s2m2_models:/models:ro \
#     s2m2-inference

ARG CUDA_VERSION=13.0.3
ARG UBUNTU_VERSION=22.04
ARG OPENCV_VERSION=4.13.0
ARG ROS_DISTRO=humble
ARG TRT_VERSION=10.13.3.9

# ═══════════════════════════════════════════════════════════════════════
# STAGE 1: OpenCV with CUDA
# ═══════════════════════════════════════════════════════════════════════
FROM nvidia/cuda:${CUDA_VERSION}-cudnn-devel-ubuntu${UBUNTU_VERSION} AS opencv-builder

ARG OPENCV_VERSION
ARG UBUNTU_VERSION

SHELL ["/bin/bash", "-c"]
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# Build dependencies for OpenCV
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    wget \
    unzip \
    pkg-config \
    libjpeg-dev \
    libpng-dev \
    libtiff-dev \
    libavcodec-dev \
    libavformat-dev \
    libswscale-dev \
    libv4l-dev \
    libxvidcore-dev \
    libx264-dev \
    libgtk-3-dev \
    libatlas-base-dev \
    gfortran \
    python3-dev \
    python3-numpy \
    libtbb2 \
    libtbb-dev \
    libdc1394-dev \
    && rm -rf /var/lib/apt/lists/*

# Download OpenCV + contrib source
WORKDIR /tmp/opencv_build
RUN wget -q https://github.com/opencv/opencv/archive/${OPENCV_VERSION}.zip \
    -O opencv.zip && \
    unzip -q opencv.zip && \
    wget -q https://github.com/opencv/opencv_contrib/archive/${OPENCV_VERSION}.zip \
    -O opencv_contrib.zip && \
    unzip -q opencv_contrib.zip && \
    rm -f opencv.zip opencv_contrib.zip

# Build OpenCV with CUDA
RUN mkdir -p opencv-${OPENCV_VERSION}/build && \
    cd opencv-${OPENCV_VERSION}/build && \
    cmake \
    -DCMAKE_BUILD_TYPE=RELEASE \
    -DCMAKE_INSTALL_PREFIX=/opt/opencv_cuda \
    -DOPENCV_EXTRA_MODULES_PATH=../../opencv_contrib-${OPENCV_VERSION}/modules \
    -DWITH_CUDA=ON \
    -DWITH_CUDNN=ON \
    -DWITH_CUBLAS=ON \
    -DWITH_TBB=ON \
    -DWITH_QT=OFF \
    -DWITH_OPENGL=ON \
    -DOPENCV_DNN_CUDA=ON \
    -DENABLE_FAST_MATH=ON \
    -DCUDA_FAST_MATH=ON \
    -DCUDA_ARCH_BIN=8.6 \
    -DCUDA_ARCH_PTX=8.6 \
    -DOPENCV_GENERATE_PKGCONFIG=ON \
    -DOPENCV_ENABLE_NONFREE=ON \
    -DBUILD_EXAMPLES=OFF \
    -DWITH_GTK=ON \
    .. && \
    make -j$(nproc) && \
    make install && \
    rm -rf /tmp/opencv_build

# ═══════════════════════════════════════════════════════════════════════
# STAGE 2: Final inference image
# ═══════════════════════════════════════════════════════════════════════
FROM nvidia/cuda:${CUDA_VERSION}-cudnn-devel-ubuntu${UBUNTU_VERSION}

ARG CUDA_VERSION
ARG CUDA_SHORT_VERSION=13.0
ARG ROS_DISTRO
ARG TRT_VERSION=10.13.3.9

SHELL ["/bin/bash", "-c"]
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC
ENV ROS_DISTRO=${ROS_DISTRO}

# ── System dependencies ─────────────────────────────────────────────────
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    curl \
    wget \
    git \
    gnupg \
    lsb-release \
    software-properties-common \
    ca-certificates \
    python3-pip \
    python3-dev \
    python3-venv \
    python3-numpy \
    libgtk-3-dev \
    && rm -rf /var/lib/apt/lists/*

# ── System libraries (Eigen3, PCL, NVML) ──────────────────────────────
# NVML library is a stub at build time (real lib loaded from driver at runtime).
# The CUDA toolkit ships libnvidia-ml.so in stubs/ for this purpose.
RUN apt-get update && apt-get install -y --no-install-recommends \
    libeigen3-dev \
    libpcl-dev \
    && rm -rf /var/lib/apt/lists/* && \
    # Create NVML stub for linking (gpu_monitoring_pkg needs it)
    if [ -f /usr/local/cuda-${CUDA_SHORT_VERSION}/lib64/stubs/libnvidia-ml.so ]; then \
        ln -sf /usr/local/cuda-${CUDA_SHORT_VERSION}/lib64/stubs/libnvidia-ml.so \
            /usr/lib/x86_64-linux-gnu/libnvidia-ml.so; \
    fi

# ── ROS2 Humble ────────────────────────────────────────────────────────
RUN curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key \
    -o /usr/share/keyrings/ros-archive-keyring.gpg && \
    echo "deb [signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] \
    http://packages.ros.org/ros2/ubuntu $(lsb_release -cs) main" \
    > /etc/apt/sources.list.d/ros2.list

# ros-base includes rclcpp, sensor_msgs, geometry_msgs, etc.
# cv_bridge and image_geometry NOT installed from apt —
# the workspace's vision_opencv fork provides them, linked against
# the custom OpenCV at /opt/opencv_cuda.
RUN apt-get update && apt-get install -y --no-install-recommends \
    ros-${ROS_DISTRO}-ros-base \
    ros-${ROS_DISTRO}-image-transport \
    ros-${ROS_DISTRO}-message-filters \
    ros-${ROS_DISTRO}-tf2-eigen \
    ros-${ROS_DISTRO}-pcl-conversions \
    ros-${ROS_DISTRO}-pcl-msgs \
    ros-${ROS_DISTRO}-ament-cmake \
    ros-${ROS_DISTRO}-rosidl-default-generators \
    ros-${ROS_DISTRO}-rosidl-default-runtime \
    python3-colcon-common-extensions \
    && rm -rf /var/lib/apt/lists/*

# ROS2 is sourced by the entrypoint already; skip bashrc to avoid duplicate

# ── TensorRT C++ ───────────────────────────────────────────────────────
# TensorRT 10.13.3.9 installed from NVIDIA's CDN tar archive.
# (The apt repo was unreachable from this build environment.)
# The tar includes headers, libraries, and trtexec.

ARG TRT_MAJOR=10.13.3
ARG TRT_FULL=${TRT_VERSION}
ARG TRT_TAR=TensorRT-${TRT_FULL}.Linux.x86_64-gnu.cuda-13.0.tar.gz
ARG TRT_URL=https://developer.download.nvidia.com/compute/machine-learning/tensorrt/${TRT_MAJOR}/tars/${TRT_TAR}

RUN echo "Downloading TensorRT from ${TRT_URL} ..." && \
    curl -fSL "${TRT_URL}" -o /tmp/trt.tar.gz --retry 3 --retry-delay 5 && \
    tar xzf /tmp/trt.tar.gz -C /opt/ && \
    rm -f /tmp/trt.tar.gz && \
    ln -sf /opt/TensorRT-${TRT_FULL} /opt/tensorrt && \
    # Copy libraries to system path and update linker cache
    cp -P /opt/tensorrt/lib/*.so* /usr/lib/x86_64-linux-gnu/ && \
    cp -P /opt/tensorrt/lib/*.so* /usr/local/cuda-${CUDA_SHORT_VERSION}/lib64/ 2>/dev/null || true && \
    # Copy headers to system include path
    cp -r /opt/tensorrt/include/* /usr/include/x86_64-linux-gnu/ && \
    cp /opt/tensorrt/include/NvInfer.h /usr/include/ && \
    ldconfig && \
    # Make trtexec available
    ln -sf /opt/tensorrt/bin/trtexec /usr/local/bin/trtexec 2>/dev/null || true

# ── OpenCV (from Stage 1) ──────────────────────────────────────────────
COPY --from=opencv-builder /opt/opencv_cuda /opt/opencv_cuda

# ── ROS2 workspace source (excludes host build artifacts) ──────────────
WORKDIR /workspace
COPY cpp_ws/ros2_impl /workspace/ros2_impl
# Ensure no stale build artifacts from the host leaked through
RUN rm -rf /workspace/ros2_impl/install /workspace/ros2_impl/build /workspace/ros2_impl/log 2>/dev/null; true

# Copy container-specific path overrides alongside the native params.yaml.
# The launch file merges this file after the native one, overriding only
# filesystem paths (/models/, /data/) while inheriting all other parameters.
COPY docker/inference/config/params_docker.yaml /workspace/ros2_impl/s2m2_inference_cpp_pkg/config/
# Make workspace writable so any UID can build and run ROS2 inside the container.
# The non-root user (host UID via --user) needs to create build/install/log dirs
# and ~/.ros/log for the ROS2 launch logger.
RUN chmod -R a+w /workspace

# ── Convenience scripts (placed on PATH) ────────────────────────────────
# After entering the container, just run:
#   build   — colcon build the workspace with all flags
#   launch  — source and launch the full stereo pipeline
COPY docker/inference/bin/ /usr/local/bin/
RUN chmod +x /usr/local/bin/build /usr/local/bin/launch

# ── Entrypoint ──────────────────────────────────────────────────────────
COPY docker/inference/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
CMD ["bash"]
