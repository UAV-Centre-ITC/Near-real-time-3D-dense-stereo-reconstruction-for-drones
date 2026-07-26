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
from s2m2.core.utils.vis_utils import visualize_stereo_results_2d, custom_visualize_stereo_results_2d

device = torch.device('cuda:0' if torch.cuda.is_available() else 'cpu')
project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
torch._dynamo.config.verbose = True
torch.backends.cudnn.benchmark = True
torch.manual_seed(0)
torch.cuda.manual_seed(0)
np.random.seed(0)

def get_args_parser():
    parser = argparse.ArgumentParser()
    
    # --- ADDED ARGUMENTS ---
    parser.add_argument('--left_img', type=str, required=True, 
                        help='path to the left input image')
    parser.add_argument('--right_img', type=str, required=True, 
                        help='path to the right input image')
    # -----------------------

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


def main(args):
    # 1. Get the directory of this script (e.g., .../alg-S2M2/demo)
    current_script_dir = os.path.dirname(os.path.abspath(__file__))

    # 2. Go UP one level to get the project root (e.g., .../alg-S2M2)
    project_root = os.path.dirname(current_script_dir)

    # 3. Construct the path to weights relative to the project root
    weights_path = os.path.join(project_root, "weights", "pretrain_weights")

    if not os.path.exists(weights_path):
        print(f"ERROR: Weights folder not found at {weights_path}")
        print("Please ensure the 'weights' folder is inside 'alg-S2M2'")
        sys.exit(1)

    # load stereo model
    model = load_model(weights_path, args.model_type, not args.allow_negative, args.num_refine, device)
    if args.torch_compile:
        model = torch.compile(model)

    # --- MODIFIED SECTION ---
    # Use arguments directly
    left_path = args.left_img
    right_path = args.right_img

    # Validate image paths
    if not os.path.exists(left_path):
        print(f"ERROR: Left image not found at {left_path}")
        sys.exit(1)

    if not os.path.exists(right_path):
         print(f"ERROR: Right image not found at {right_path}")
         sys.exit(1)
    # ------------------------

    # load stereo images
    left, right = read_images(left_path, right_path)

    img_height, img_width = left.shape[:2]
    print(f"original image size: img_height({img_height}), img_width({img_width})")
    img_height = (img_height // 32) * 32
    img_width = (img_width // 32) * 32
    print(f"cropped image size: img_height({img_height}), img_width({img_width})")

    # image crop
    left = left[:img_height, :img_width]
    right = right[:img_height, :img_width]

    # to torch tensor
    left_torch = (torch.from_numpy(left).permute(-1, 0, 1).unsqueeze(0)).to(device)
    right_torch = (torch.from_numpy(right).permute(-1, 0, 1).unsqueeze(0)).to(device)

    # run stereo matching
    _ = run_stereo_matching(model, left_torch, right_torch, device) #pre-run
    pred_disp, pred_occ, pred_conf, avg_conf_score, avg_run_time = run_stereo_matching(model, left_torch, right_torch, device, N_repeat=5)
    print(F"torch avg inference time:{(avg_run_time)/1000}, FPS:{1000/(avg_run_time)}")
    #TODO save in .pfm the disparity image output
    #convert to numpy array from torch tensor and save in .pfm format: 
      
    pred_disp_numpy = pred_disp.detach().cpu().numpy()
    save_pfm("disp_demo.pfm", pred_disp_numpy)
    # opencv 2D visualization
    pred_disp, pred_occ, pred_conf = pred_disp.cpu().numpy(), pred_occ.cpu().numpy(), pred_conf.cpu().numpy()
    custom_visualize_stereo_results_2d(left, right, pred_disp, pred_occ, pred_conf)

if __name__ == '__main__':
    parser = get_args_parser()
    args = parser.parse_args()
    print(args)
    main(args)
