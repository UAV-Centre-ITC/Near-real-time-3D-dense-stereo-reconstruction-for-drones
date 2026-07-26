import cv2
import argparse
import os
import sys

def resize_and_save(image_path, scale_factor):
    if not os.path.exists(image_path):
        print(f"Error: File not found at {image_path}")
        sys.exit(1)

    img = cv2.imread(image_path)
    if img is None:
        print(f"Error: Could not decode image at {image_path}")
        sys.exit(1)

    width = int(img.shape[1] * scale_factor)
    height = int(img.shape[0] * scale_factor)
    dim = (width, height)

    # INTER_AREA is technically the best for downsampling as it prevents aliasing.
    # However, if you feel it is too blurry, you can try cv2.INTER_LANCZOS4
    interpolation = cv2.INTER_AREA
    resized_img = cv2.resize(img, dim, interpolation=interpolation)

    # --- FIX: Force saving as PNG to prevent JPEG artifacts ---
    root, _ = os.path.splitext(image_path)
    new_path = f"{root}_scaled.png" 

    # Save (PNG compression is lossless, 3 is default compression level)
    cv2.imwrite(new_path, resized_img, [cv2.IMWRITE_PNG_COMPRESSION, 3])
    print(f"Saved: {new_path}")
    
    return new_path

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--left', type=str, required=True)
    parser.add_argument('--right', type=str, required=True)
    parser.add_argument('--scale', type=float, required=True)

    args = parser.parse_args()

    resize_and_save(args.left, args.scale)
    resize_and_save(args.right, args.scale)
