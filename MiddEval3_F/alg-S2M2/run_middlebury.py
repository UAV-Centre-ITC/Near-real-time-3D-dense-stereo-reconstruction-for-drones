import argparse
import os
import sys
os.environ["PYTORCH_CUDA_ALLOC_CONF"] = "expandable_segments:True"

import numpy as np
import cv2
import torch
import torch._dynamo
from s2m2.core.utils.model_utils import load_model, run_stereo_matching
from s2m2.core.utils.image_utils import read_images
from s2m2.core.utils.vis_utils import visualize_stereo_results_2d

device = torch.device('cuda:0' if torch.cuda.is_available() else 'cpu')
project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
torch._dynamo.config.verbose = True
torch.backends.cudnn.benchmark = True
torch.manual_seed(0)
torch.cuda.manual_seed(0)
np.random.seed(0)

def get_args_parser():
    parser = argparse.ArgumentParser()

    #-------For the Middlebury run.sh script--------# : 
    parser.add_argument('--img_left', required=True, type=str,
                        help='Path to left image')
    parser.add_argument('--img_right', required=True, type=str,
                        help='Path to right image')
    parser.add_argument('--output_dir', required=True, type=str,
                        help='Output directory for results')
    parser.add_argument('--num_disparities', required=True, type=int,
                        help='Maximum number of disparities (used for scaling)')
    #-------------#

    parser.add_argument('--model_type', default='S', type=str,
                        help='select model type: S,M,L,XL')
    parser.add_argument('--num_refine', default=3, type=int,
                        help='number of local iterative refinement')
    parser.add_argument('--torch_compile', action='store_true', help='apply torch_compile')
    parser.add_argument('--allow_negative', action='store_true', help='allow negative disparity for imperfect rectification')
    return parser

def save_pfm(filename, data, scale=-1.0):
    # Data must be numpy array
    if len(data.shape) != 2:
        raise ValueError("Data must be 2D array for PFM format")
    
    # MIDDLEBURY CONVENTION: Flip vertical
    data = np.flipud(data)
    
    height, width = data.shape
    with open(filename, 'wb') as f:
        f.write(b'Pf\n')
        f.write(f'{width} {height}\n'.encode())
        f.write(f'{scale}\n'.encode())
        
        # --- FIXED FOR NUMPY 2.0 ---
        # '<f4' means: Little-Endian (<), Float (f), 4-bytes (32-bit)
        data.astype('<f4').tofile(f)

def save_timelog(filename, time_seconds):
    # Mode 'w' for text, not 'wb'
    with open(filename, 'w') as f:
        f.write(f"{time_seconds:.4f}")

def main(args):
    # 1. Setup paths
    # Ensure output directory exists
    if not os.path.exists(args.output_dir):
        os.makedirs(args.output_dir)

    pfm_output_path = os.path.join(args.output_dir, 'disp0.pfm')
    time_output_path = os.path.join(args.output_dir, 'time.txt')

    algorithm_folder = os.path.dirname(os.path.abspath(__file__))
    weights_path = os.path.join(algorithm_folder, "weights", "pretrain_weights")

    print(f"Looking for model at: {weights_path}")    
    # Check if path exists to give a clear error if it doesn't
    if not os.path.exists(weights_path):
        print(f"ERROR: Weights folder not found at {weights_path}")
        print("Please ensure the 'weights' folder is copied inside 'alg-S2M2'")
        sys.exit(1)

    # Load model
    model = load_model(weights_path, args.model_type, not args.allow_negative, args.num_refine, device)
    if args.torch_compile:
        model = torch.compile(model)

    # 2. Load Images (OpenCV loads as HWC)
    left_path = args.img_left
    right_path = args.img_right
    
    left, right = read_images(left_path, right_path) 
    
    # 3. To Torch (Format: Batch x Channels x Height x Width)
    # We pass the original size images directly.
    # The run_stereo_matching function handles the padding internally.
    left_torch = (torch.from_numpy(left).permute(2, 0, 1).unsqueeze(0)).to(device)
    right_torch = (torch.from_numpy(right).permute(2, 0, 1).unsqueeze(0)).to(device)

    # 4. Inference
    print("Running inference...")
    
    # Warmup (optional, but good practice)
    _ = run_stereo_matching(model, left_torch, right_torch, device, N_repeat=1)
    
    # Actual Run
    # Returns: disp, occ, conf, score, time_ms
    pred_disp, _, _, _, avg_run_time_ms = run_stereo_matching(
        model, left_torch, right_torch, device, N_repeat=5
    )
    
    # 5. Post-Processing
    # The function returns a Tensor. We need a Numpy array on CPU.
    # It also ensures the shape matches the input (H, W), so we don't need to crop.
    pred_disp_numpy = pred_disp.detach().cpu().numpy()

    # 6. Save Results
    print(f"Saving PFM to {pfm_output_path}")
    
    # save_pfm handles the vertical flip required by Middlebury
    save_pfm(pfm_output_path, pred_disp_numpy)
    
    # Convert milliseconds (from torch event) to seconds (for Middlebury)
    time_seconds = avg_run_time_ms / 1000.0
    save_timelog(time_output_path, time_seconds)

if __name__ == '__main__':
    parser = get_args_parser()
    args = parser.parse_args()
    main(args)
