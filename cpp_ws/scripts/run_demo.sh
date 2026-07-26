#!/bin/bash

# Check if correct number of arguments are passed
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <path_to_left_image> <path_to_right_image> <scale_factor>"
    echo "Example: ./run_demo.sh ./data/left.jpg ./data/right.jpg 0.5"
    exit 1
fi
# --- FIX START: Convert inputs to Absolute Paths ---
# This ensures the paths remain valid even after you 'cd' into other folders
LEFT_IMG=$(realpath "$1")
RIGHT_IMG=$(realpath "$2")
# --- FIX END ---
SCALE="$3"

echo "=========================================="
echo "STEP 0: Setup the conda environment and install dependencies"
echo "=========================================="

cd /home/s3488977/thesis/MiddEval3_F/alg-S2M2

#this line allows 'conda activate' to work inside a bash script
eval "$(conda shell.bash hook)"

conda activate s2m2
pip install -e . 
#return to the original directory: 
cd -

echo ""
echo "=========================================="
echo "STEP 1: Resizing Images by factor $SCALE"
echo "=========================================="

# Run the python resizing script
python3 resize_images.py --left "$LEFT_IMG" --right "$RIGHT_IMG" --scale "$SCALE"

# Check if the python script failed
if [ $? -ne 0 ]; then
    echo "Error: Image resizing failed."
    exit 1
fi

# Bash string manipulation to determine the names of the generated files
# This extracts the extension and filename to append "_scaled"
# Logic: path/to/image.png -> path/to/image_scaled.png

# For Left Image
L_DIR=$(dirname "$LEFT_IMG")
L_FILENAME=$(basename "$LEFT_IMG")
L_NAME="${L_FILENAME%.*}"
LEFT_SCALED="${L_DIR}/${L_NAME}_scaled.png"

# For Right Image
R_DIR=$(dirname "$RIGHT_IMG")
R_FILENAME=$(basename "$RIGHT_IMG")
R_NAME="${R_FILENAME%.*}"
RIGHT_SCALED="${R_DIR}/${R_NAME}_scaled.png"

echo ""
echo "=========================================="
echo "STEP 2: Running Stereo Algorithm"
echo "Inputs: $LEFT_SCALED & $RIGHT_SCALED"
echo "=========================================="

# Go to the directory where the stereo vision script is located: 
cd /home/s3488977/thesis/MiddEval3_F/alg-S2M2/demo

# Run the existing stereo vision script
python3 visualize_2d_simple.py --left_img "$LEFT_SCALED" --right_img "$RIGHT_SCALED"

echo ""
echo "Done running the demo script. Disparity should now be saved in .pfm format."
