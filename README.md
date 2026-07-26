


# Near real-time 3D stereo reconstruction from drone imagery and SLAM pose estimations. - Branch : uav2_impl

- [Near real-time 3D stereo reconstruction from drone imagery and SLAM pose estimations. - Branch : uav2\_impl](#near-real-time-3d-stereo-reconstruction-from-drone-imagery-and-slam-pose-estimations---branch--uav2_impl)
  - [Code preparation](#code-preparation)
  - [Acknowledgements](#acknowledgements)
  - [Contact](#contact)


**EXPERIMENTAL BRANCH**: 

This branch uses a 4 image samples from the M13_2018 aerial dataset to test the stereo pipeline. It is identical to the *full_resolution_uav* branch, apart from image resizing. This branch does not split the image to 4 equal patches. It uses a scale factor parameter and resizes the images **before** the rectification step. Camera intrinsics are also scaled acoordingly to produce proper rectification results. 

## Code preparation 

**Identical to the other sample-based branches**. See *full_resolution_uav* on how to export the TensorRT engine file of S2M2, build OpenCV 4.13 with cuda modules enabled, and how to build and launch the nodes. 


## Acknowledgements

This project uses the S2M2 model(Junhong Min et al.). I would like to thank them for their work and for providing this open-source model. 
``` bibtex
@inproceedings{min2025s2m2,
  title={{S\textsuperscript{2}M\textsuperscript{2}}: Scalable Stereo Matching Model for Reliable Depth Estimation},
  author={Junhong Min and Youngpil Jeon and Jimin Kim and Minyong Choi},
  booktitle={Proceedings of the IEEE/CVF International Conference on Computer Vision (ICCV)},
  year={2025}
}
```

## Contact

If you have any questions regarding the code, please contact me at geoder.097@gmail.com 



