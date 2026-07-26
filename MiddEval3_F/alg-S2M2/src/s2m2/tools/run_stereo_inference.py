"""
Stereo inference script for S²M² model with Middlebury-compatible output.

This script runs stereo matching on a pair of images and outputs results
in PFM format compatible with Middlebury evaluation benchmarks.

Usage:
    python run_stereo_inference.py --img_left left.png --img_right right.png --output_dir ./output --num_disparities 128
"""

import argparse
import os
import sys
import time
import numpy as np
import cv2
import torch
from pathlib import Path

# Add project root to path
project_root = Path(__file__).parent.parent.parent
sys.path.insert(0, str(project_root))

from s2m2.core.utils.model_utils import load_model, run_stereo_matching
from s2m2.core.utils.image_utils import read_images
from s2m2.core.utils.pfm_utils import save_disparity_pfm, save_occlusion_pfm, save_confidence_pfm


def parse_arguments():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description='S²M² Stereo Inference Script',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python run_stereo_inference.py --img_left im0.png --img_right im1.png --output_dir ./output --num_disparities 128
  python run_stereo_inference.py --img_left im0.png --img_right im1.png --output_dir ./output --num_disparities 128 --model_type L --refine_iter 2
        """
    )
    
    parser.add_argument('--img_left', required=True, type=str,
                        help='Path to left image')
    parser.add_argument('--img_right', required=True, type=str,
                        help='Path to right image')
    parser.add_argument('--output_dir', required=True, type=str,
                        help='Output directory for results')
    parser.add_argument('--num_disparities', required=True, type=int,
                        help='Maximum number of disparities (used for scaling)')
    
    parser.add_argument('--model_type', default='XL', type=str, choices=['S', 'M', 'L', 'XL'],
                        help='Model type: S (26.5M), M (80.4M), L (181M), XL (406M) - default: XL')
    parser.add_argument('--refine_iter', default=3, type=int,
                        help='Number of local iterative refinement steps - default: 3')
    parser.add_argument('--timing_runs', default=5, type=int,
                        help='Number of timing runs for accurate measurement - default: 5')
    parser.add_argument('--torch_compile', action='store_true',
                        help='Enable torch.compile for faster inference')
    parser.add_argument('--allow_negative', action='store_true',
                        help='Allow negative disparity for imperfect rectification')
    parser.add_argument('--sparse', action='store_true',
                        help='Generate sparse output (no interpolation of "holes")')
    
    return parser.parse_args()


def validate_inputs(args):
    """Validate input arguments and files."""
    errors = []
    
    # Check input files exist
    if not os.path.exists(args.img_left):
        errors.append(f"Left image not found: {args.img_left}")
    
    if not os.path.exists(args.img_right):
        errors.append(f"Right image not found: {args.img_right}")
    
    # Check output directory
    try:
        os.makedirs(args.output_dir, exist_ok=True)
    except Exception as e:
        errors.append(f"Cannot create output directory: {e}")
    
    # Validate num_disparities
    if args.num_disparities <= 0:
        errors.append("num_disparities must be positive")
    
    # Validate timing_runs
    if args.timing_runs <= 0:
        errors.append("timing_runs must be positive")
    
    if errors:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return False
    
    return True


def load_and_preprocess_images(left_path, right_path):
    """Load and preprocess stereo image pair."""
    try:
        # Load images
        left, right = read_images(left_path, right_path)
        
        # Check image dimensions match
        if left.shape != right.shape:
            raise ValueError(f"Image dimensions don't match: {left.shape} vs {right.shape}")
        
        # Check if images are grayscale or color
        if len(left.shape) == 2:
            # Convert grayscale to RGB
            left = cv2.cvtColor(left, cv2.COLOR_GRAY2RGB)
            right = cv2.cvtColor(right, cv2.COLOR_GRAY2RGB)
        elif left.shape[2] == 4:
            # Convert RGBA to RGB
            left = left[:, :, :3]
            right = right[:, :, :3]
        
        # Ensure images are in RGB format (OpenCV loads as BGR)
        if left.shape[2] == 3:
            left = cv2.cvtColor(left, cv2.COLOR_BGR2RGB)
            right = cv2.cvtColor(right, cv2.COLOR_BGR2RGB)
        
        return left, right, left.shape[:2]
        
    except Exception as e:
        raise RuntimeError(f"Failed to load images: {e}")


def run_inference(model, left_torch, right_torch, device, timing_runs):
    """Run stereo inference with timing measurement."""
    try:
        # Pre-run to warm up
        _ = run_stereo_matching(model, left_torch, right_torch, device, N_repeat=1)
        
        # Run inference with timing
        pred_disp, pred_occ, pred_conf, avg_conf_score, avg_run_time = run_stereo_matching(
            model, left_torch, right_torch, device, N_repeat=timing_runs
        )
        
        return pred_disp, pred_occ, pred_conf, avg_conf_score, avg_run_time
        
    except Exception as e:
        raise RuntimeError(f"Inference failed: {e}")


def save_results(output_dir, pred_disp, pred_occ, pred_conf, timing_info, args):
    """Save inference results to output directory."""
    try:
        # Convert tensors to numpy arrays
        pred_disp_np = pred_disp.cpu().numpy()
        pred_occ_np = pred_occ.cpu().numpy()
        pred_conf_np = pred_conf.cpu().numpy()
        
        # Save disparity maps
        disp0_path = os.path.join(output_dir, 'disp0.pfm')
        save_disparity_pfm(disp0_path, pred_disp_np)
        print(f"Saved dense disparity map: {disp0_path}")
        
        # Save sparse disparity map if requested
        if args.sparse:
            # Apply occlusion mask to create sparse map
            sparse_disp = pred_disp_np.copy()
            sparse_disp[pred_occ_np < 0.5] = np.inf  # Mark occluded pixels as infinity
            
            disp0_s_path = os.path.join(output_dir, 'disp0_s.pfm')
            save_disparity_pfm(disp0_s_path, sparse_disp)
            print(f"Saved sparse disparity map: {disp0_s_path}")
        
        # Save occlusion map
        occ_path = os.path.join(output_dir, 'occlusion.pfm')
        save_occlusion_pfm(occ_path, pred_occ_np)
        print(f"Saved occlusion map: {occ_path}")
        
        # Save confidence map
        conf_path = os.path.join(output_dir, 'confidence.pfm')
        save_confidence_pfm(conf_path, pred_conf_np)
        print(f"Saved confidence map: {conf_path}")
        
        # Save timing information
        time_path = os.path.join(output_dir, 'time.txt')
        with open(time_path, 'w') as f:
            # Convert milliseconds to seconds for Middlebury format
            runtime_seconds = timing_info['avg_run_time'] / 1000.0
            f.write(f"{runtime_seconds:.6f}\n")
        print(f"Saved timing information: {time_path}")
        
        # Save metadata
        metadata_path = os.path.join(output_dir, 'metadata.txt')
        with open(metadata_path, 'w') as f:
            f.write(f"Model Type: {args.model_type}\n")
            f.write(f"Refinement Iterations: {args.refine_iter}\n")
            f.write(f"Timing Runs: {args.timing_runs}\n")
            f.write(f"Average Runtime (ms): {timing_info['avg_run_time']:.3f}\n")
            f.write(f"Average Runtime (s): {timing_info['avg_run_time']/1000.0:.6f}\n")
            f.write(f"FPS: {1000.0/timing_info['avg_run_time']:.2f}\n")
            f.write(f"Average Confidence Score: {timing_info['avg_conf_score']:.4f}\n")
            f.write(f"Image Dimensions: {timing_info['image_shape']}\n")
            f.write(f"Allow Negative Disparity: {args.allow_negative}\n")
            f.write(f"Torch Compile: {args.torch_compile}\n")
            f.write(f"Timestamp: {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
        print(f"Saved metadata: {metadata_path}")
        
        return True
        
    except Exception as e:
        raise RuntimeError(f"Failed to save results: {e}")


def main():
    """Main inference function."""
    args = parse_arguments()
    
    # Validate inputs
    if not validate_inputs(args):
        sys.exit(1)
    
    print(f"S²M² Stereo Inference")
    print(f"Model: {args.model_type}")
    print(f"Left image: {args.img_left}")
    print(f"Right image: {args.img_right}")
    print(f"Output directory: {args.output_dir}")
    print(f"Max disparities: {args.num_disparities}")
    print(f"Refinement iterations: {args.refine_iter}")
    print(f"Timing runs: {args.timing_runs}")
    print()
    
    # Set up device
    device = torch.device('cuda:0' if torch.cuda.is_available() else 'cpu')
    print(f"Using device: {device}")
    
    if device.type == 'cuda':
        print(f"CUDA available: {torch.cuda.is_available()}")
        print(f"CUDA device count: {torch.cuda.device_count()}")
        if torch.cuda.is_available():
            print(f"CUDA current device: {torch.cuda.current_device()}")
            print(f"CUDA device name: {torch.cuda.get_device_name()}")
    print()
    
    try:
        # Load model
        print("Loading model...")
        model = load_model(
            os.path.join(project_root, "weights/pretrain_weights"),
            args.model_type,
            not args.allow_negative,
            args.refine_iter,
            device
        )
        
        if model is None:
            print("ERROR: Failed to load model", file=sys.stderr)
            sys.exit(1)
        
        if args.torch_compile:
            print("Compiling model with torch.compile...")
            model = torch.compile(model)
        
        print("Model loaded successfully")
        print()
        
        # Load and preprocess images
        print("Loading images...")
        left_img, right_img, image_shape = load_and_preprocess_images(args.img_left, args.img_right)
        print(f"Image shape: {image_shape}")
        print()
        
        # Prepare tensors
        print("Preparing tensors...")
        left_torch = (torch.from_numpy(left_img).permute(-1, 0, 1).unsqueeze(0)).to(device)
        right_torch = (torch.from_numpy(right_img).permute(-1, 0, 1).unsqueeze(0)).to(device)
        print("Tensors prepared")
        print()
        
        # Run inference
        print("Running inference...")
        start_time = time.time()
        
        pred_disp, pred_occ, pred_conf, avg_conf_score, avg_run_time = run_inference(
            model, left_torch, right_torch, device, args.timing_runs
        )
        
        inference_time = time.time() - start_time
        print(f"Inference completed in {inference_time:.3f} seconds")
        print(f"Average runtime per inference: {avg_run_time:.3f} ms")
        print(f"FPS: {1000.0/avg_run_time:.2f}")
        print(f"Average confidence score: {avg_conf_score:.4f}")
        print()
        
        # Save results
        print("Saving results...")
        timing_info = {
            'avg_run_time': avg_run_time,
            'avg_conf_score': avg_conf_score,
            'image_shape': image_shape,
            'inference_time': inference_time
        }
        
        save_results(args.output_dir, pred_disp, pred_occ, pred_conf, timing_info, args)
        print()
        print("Inference completed successfully!")
        print(f"Results saved to: {args.output_dir}")
        
    except Exception as e:
        print(f"ERROR: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
