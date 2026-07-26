# s2m2-export — Python conda environment for S2M2 model export
# Builds: PyTorch → ONNX → TensorRT .engine file
#
# No system TensorRT needed — uses tensorrt-cu12 Python package
# from the conda environment (includes both bindings and libs).
#
# Build:
#   docker build -t s2m2-export:latest -f docker/export/Dockerfile .
#
# Usage:
#   # Export an engine:
#   docker run --gpus all -v s2m2_models:/workspace/alg-S2M2/weights/trt_save \
#     s2m2-export --model_type S --img_width 1248 --img_height 1024 --precision fp16
#
#   # Generate ONNX first:
#   docker run --gpus all -v s2m2_models:/workspace/alg-S2M2/weights/trt_save \
#     s2m2-export onnx --model_type S --img_width 1248 --img_height 1024
#
#   # Interactive shell:
#   docker run --gpus all -it --rm s2m2-export bash

ARG CUDA_VERSION=13.0.3
ARG UBUNTU_VERSION=22.04

FROM nvidia/cuda:${CUDA_VERSION}-cudnn-devel-ubuntu${UBUNTU_VERSION}

SHELL ["/bin/bash", "-c"]

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# ── System dependencies ─────────────────────────────────────────────────
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    wget \
    curl \
    ca-certificates \
    libgl1 \
    libglib2.0-0 \
    && rm -rf /var/lib/apt/lists/*

# ── Miniconda ───────────────────────────────────────────────────────────
ENV CONDA_DIR=/opt/conda
ENV PATH=${CONDA_DIR}/bin:${PATH}

RUN wget -q https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh \
    -O /tmp/miniconda.sh && \
    bash /tmp/miniconda.sh -b -p ${CONDA_DIR} && \
    rm /tmp/miniconda.sh && \
    conda config --set auto_update_conda false && \
    conda clean -afy

# ── Conda environment ──────────────────────────────────────────────────
# The environment.yml includes tensorrt-cu12==10.13.3 (Python bindings + libs).
WORKDIR /workspace

COPY alg-S2M2/environment.yml /workspace/environment.yml

RUN conda tos accept --override-channels --channel https://repo.anaconda.com/pkgs/main 2>/dev/null || true && \
    conda tos accept --override-channels --channel https://repo.anaconda.com/pkgs/r 2>/dev/null || true && \
    conda env create -f /workspace/environment.yml && \
    conda clean -afy

RUN conda init bash && \
    echo "conda activate s2m2" >> ~/.bashrc

# ── alg-S2M2 source code ──────────────────────────────────────────────
COPY alg-S2M2 /workspace/alg-S2M2

RUN source $(conda info --base)/etc/profile.d/conda.sh && \
    conda activate s2m2 && \
    cd /workspace/alg-S2M2 && \
    pip install -e .

# ── Docker export script ───────────────────────────────────────────────
# Replaces trtexec CLI with pure Python TensorRT API (avoids system TRT).
COPY docker/export/export_tensorrt.py /workspace/export_tensorrt.py

# ── Entrypoint ──────────────────────────────────────────────────────────
COPY docker/export/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
CMD ["--help"]
