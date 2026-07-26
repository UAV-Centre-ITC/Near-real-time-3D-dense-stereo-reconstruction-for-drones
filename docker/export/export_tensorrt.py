#!/usr/bin/env python3
"""
Export TensorRT engine from PyTorch model in one step.
1. Loads the PyTorch model from pretrained weights
2. Exports to ONNX (using synthetic input tensors — no sample images needed)
3. Builds TensorRT engine from the ONNX

Usage:
  python export_tensorrt.py \
    --model_type S --img_width 1248 --img_height 1024 --precision fp16 \
    --weights_dir /workspace/alg-S2M2/weights/pretrain_weights
"""

import argparse
import os
import sys

os.environ["PYTORCH_CUDA_ALLOC_CONF"] = "expandable_segments:True"

import numpy as np
import torch
import torch._dynamo
import tensorrt as trt

# Add the alg-S2M2 package to the path
sys.path.insert(0, "/workspace/alg-S2M2")
from s2m2.core.utils.model_utils import load_model
from s2m2.tools.export_model import export_onnx

TRT_LOGGER = trt.Logger(trt.Logger.WARNING)


def build_trt_engine(onnx_path, engine_path, precision):
    """Build a TensorRT engine from an ONNX file using the Python API."""
    builder = trt.Builder(TRT_LOGGER)
    network = builder.create_network(
        1 << int(trt.NetworkDefinitionCreationFlag.EXPLICIT_BATCH)
    )
    parser = trt.OnnxParser(network, TRT_LOGGER)

    with open(onnx_path, "rb") as f:
        if not parser.parse(f.read()):
            for i in range(parser.num_errors):
                print(f"ONNX parse error: {parser.get_error(i)}")
            raise RuntimeError("Failed to parse ONNX file")

    config = builder.create_builder_config()
    config.set_memory_pool_limit(trt.MemoryPoolType.WORKSPACE, 8 << 30)  # 8 GiB

    if precision == "fp16":
        config.set_flag(trt.BuilderFlag.FP16)
    elif precision == "tf32":
        config.set_flag(trt.BuilderFlag.TF32)
    elif precision == "fp32":
        config.clear_flag(trt.BuilderFlag.TF32)

    profile = builder.create_optimization_profile()
    for i in range(network.num_inputs):
        tensor = network.get_input(i)
        shape = tensor.shape
        profile.set_shape(tensor.name, shape, shape, shape)
    config.add_optimization_profile(profile)

    serialized = builder.build_serialized_network(network, config)
    if serialized is None:
        raise RuntimeError("Failed to build TensorRT engine")

    with open(engine_path, "wb") as f:
        f.write(serialized)
    print(f"Engine saved to {engine_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Export S2M2 model to TensorRT engine (ONNX + TRT in one step)"
    )
    parser.add_argument("--model_type", type=str, default="S")
    parser.add_argument("--img_width", type=int, default=1248)
    parser.add_argument("--img_height", type=int, default=1024)
    parser.add_argument(
        "--precision", type=str, choices=["fp16", "tf32", "fp32"], default="fp16"
    )
    parser.add_argument(
        "--weights_dir",
        type=str,
        default="/workspace/alg-S2M2/weights/pretrain_weights",
        help="Directory containing pretrained model weights",
    )
    parser.add_argument(
        "--output_dir",
        type=str,
        default="/workspace/alg-S2M2/weights",
        help="Base output directory for ONNX and TRT files",
    )
    parser.add_argument("--num_refine", type=int, default=3)
    parser.add_argument("--allow_negative", action="store_true")
    args = parser.parse_args()

    device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
    torch._dynamo.config.verbose = True
    torch.backends.cudnn.benchmark = True
    torch.manual_seed(0)
    torch.cuda.manual_seed(0)
    np.random.seed(0)

    os.makedirs(os.path.join(args.output_dir, "onnx_save"), exist_ok=True)
    os.makedirs(os.path.join(args.output_dir, "trt_save"), exist_ok=True)

    # Determine torch major/minor for filename
    torch_ver = torch.__version__
    torch_ver_short = f"{torch_ver[0]}{torch_ver[2]}"

    onnx_filename = (
        f"S2M2_{args.model_type}_{args.img_width}_{args.img_height}"
        f"_v2_torch{torch_ver_short}.onnx"
    )
    trt_filename = (
        f"S2M2_{args.model_type}_{args.img_width}_{args.img_height}"
        f"_{args.precision}.engine"
    )
    onnx_path = os.path.join(args.output_dir, "onnx_save", onnx_filename)
    trt_path = os.path.join(args.output_dir, "trt_save", trt_filename)

    # ── Step 1: Load model and export to ONNX ──────────────────────
    if not os.path.exists(onnx_path):
        print(f"ONNX not found at {onnx_path}, generating...")
        if not os.path.isdir(args.weights_dir):
            raise FileNotFoundError(
                f"Weights directory not found: {args.weights_dir}\n"
                "Mount your pretrained weights directory, e.g.:\n"
                "  -v /path/to/weights:/workspace/alg-S2M2/weights/pretrain_weights"
            )

        model = load_model(
            args.weights_dir,
            args.model_type,
            not args.allow_negative,
            args.num_refine,
            "cpu",
        )
        model = model.to(device)
        model.eval()

        # Create synthetic input tensors (no sample images needed)
        left_torch = torch.randn(1, 3, args.img_height, args.img_width).to(device)
        right_torch = torch.randn(1, 3, args.img_height, args.img_width).to(device)

        print(f"Exporting ONNX to {onnx_path} ...")
        export_onnx(model, onnx_path, left_torch, right_torch)
        print("ONNX export complete.")
    else:
        print(f"Using existing ONNX: {onnx_path}")

    # ── Step 2: Build TensorRT engine ──────────────────────────────
    if not os.path.exists(trt_path):
        print(f"Building TensorRT engine: {trt_path} ...")
        build_trt_engine(onnx_path, trt_path, args.precision)
    else:
        print(f"Engine already exists: {trt_path}")

    print("Done.")


if __name__ == "__main__":
    main()
