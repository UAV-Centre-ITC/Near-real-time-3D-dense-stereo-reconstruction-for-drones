import numpy as np
import cv2
import os
import argparse

def read_pfm(file_path):
    """Reads a .pfm file and returns a numpy array."""
    with open(file_path, 'rb') as f:
        header = f.readline().decode('utf-8').rstrip()
        if header not in ['PF', 'Pf']:
            raise ValueError("Not a valid PFM file.")
        
        channels = 3 if header == 'PF' else 1
        dims = f.readline().decode('utf-8').rstrip()
        width, height = map(int, dims.split())

        scale = float(f.readline().decode('utf-8').rstrip())
        endian = '<' if scale < 0 else '>' 

        data = np.fromfile(f, endian + 'f')
        shape = (height, width, channels) if channels > 1 else (height, width)
        
        image = np.reshape(data, shape)
        image = np.flipud(image)
        return image

def main():
    # Set up the command line argument parser
    parser = argparse.ArgumentParser(description="Convert a .pfm disparity image to a .png file.")
    parser.add_argument("input_file", help="Path to the .pfm file you want to convert.")
    args = parser.parse_args()

    pfm_path = args.input_file

    # 1. Validation
    if not os.path.exists(pfm_path):
        print(f"Error: The file '{pfm_path}' does not exist.")
        return
    
    if not pfm_path.lower().endswith('.pfm'):
        print("Error: Input file must have a .pfm extension.")
        return

    # 2. Processing
    print(f"Reading {pfm_path}...")
    try:
        disp_array = read_pfm(pfm_path)
    except Exception as e:
        print(f"Failed to read PFM: {e}")
        return

    # Normalize data for 8-bit PNG visualization
    disp_array[np.isinf(disp_array)] = 0
    max_val = disp_array.max()
    
    if max_val > 0:
        normalized_disp = (disp_array / max_val) * 255
    else:
        normalized_disp = disp_array
        
    final_image = normalized_disp.astype(np.uint8)

    # 3. Save output
    png_path = os.path.splitext(pfm_path)[0] + ".png"
    cv2.imwrite(png_path, final_image)
    print(f"Successfully converted and saved to: {png_path}")

if __name__ == "__main__":
    main()
