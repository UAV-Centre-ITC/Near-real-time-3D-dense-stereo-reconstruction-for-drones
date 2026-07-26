import cv2
import argparse
import os
import sys
import math


def pad_to_multiple_of_32(img, multiple=32):
    h, w = img.shape[:2]

    new_h = math.ceil(h / multiple) * multiple
    new_w = math.ceil(w / multiple) * multiple

    pad_bottom = new_h - h
    pad_right = new_w - w

    # Pad only bottom and right (important for stereo consistency)
    padded = cv2.copyMakeBorder(
        img,
        top=0,
        bottom=pad_bottom,
        left=0,
        right=pad_right,
        borderType=cv2.BORDER_CONSTANT,
        value=[0, 0, 0],  # black padding
    )

    return padded


def resize_and_save(image_path, scale_factor):
    if not os.path.exists(image_path):
        print(f"Error: File not found at {image_path}")
        sys.exit(1)

    img = cv2.imread(image_path)
    if img is None:
        print(f"Error: Could not decode image at {image_path}")
        sys.exit(1)

    # --- Resize ---
    width = int(img.shape[1] * scale_factor)
    height = int(img.shape[0] * scale_factor)
    dim = (width, height)

    interpolation = cv2.INTER_AREA
    resized_img = cv2.resize(img, dim, interpolation=interpolation)

    # --- Pad to multiple of 32 ---
    padded_img = pad_to_multiple_of_32(resized_img, multiple=32)

    # --- Save as PNG ---
    root, _ = os.path.splitext(image_path)
    new_path = f"{root}_scaled.png"

    cv2.imwrite(new_path, padded_img, [cv2.IMWRITE_PNG_COMPRESSION, 3])
    print(f"Saved: {new_path}")
    print(f"Final shape: {padded_img.shape}")
    print("Dimensions are multiples of 32, for the model input.")
    return new_path


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--left', type=str, required=True)
    parser.add_argument('--right', type=str, required=True)
    parser.add_argument('--scale', type=float, required=True)

    args = parser.parse_args()

    resize_and_save(args.left, args.scale)
    resize_and_save(args.right, args.scale)
