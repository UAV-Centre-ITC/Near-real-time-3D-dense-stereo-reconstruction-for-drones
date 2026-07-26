import cv2

def swap_and_save(filename):
    # 1. Read image (OpenCV reads this as BGR)
    img = cv2.imread(filename)
    if img is None:
        print(f"Could not find {filename}")
        return

    # 2. Manually swap Red and Blue channels
    # This turns BGR -> RGB
    img_swapped = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

    # 3. Save using OpenCV 
    # Because cv2.imwrite EXPECTS BGR, saving an RGB array 
    # will result in a file that looks "wrong" in your viewer.
    # This confirms the channels have actually moved.
    cv2.imwrite(f"swapped_{filename}", img_swapped)
    print(f"Saved swapped_{filename}. View this file; colors should look 'inverted'.")

swap_and_save("rectified_left.png")
swap_and_save("rectified_right.png")