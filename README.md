# Near real-time 3D stereo reconstruction from drone imagery and SLAM pose estimations. - Branch : dockerized_ros2impl

On this branch, the *ros2_impl* branch is containerized with docker. It was created for convenience in case future usage of the other branches fails due to different package versionings or updates on the machine, and for portability purposes. 

**DISCLAIMER**: AI assistance was used to create all docker-related scripts, due to lack of knowledge and time for this software tool.  

## Architecture: Two-Container Design

```
┌──────────────────────────┐       shared volume       ┌──────────────────────────────┐
│   s2m2-export            │   ────────────────────>   │   s2m2-inference             │
│                          │   s2m2_models:/models     │                              │
│  Python / Conda (CUDA 12)│   (.engine files)         │  C++ / ROS2 / colcon         │
│  alg-S2M2/ source        │                           │  cpp_ws/ros2_impl/ source    │
│                          │                           │                              │
│  Builds .engine from     │                           │  Loads .engine at runtime    │
│  PyTorch -> ONNX -> TRT  │                           │  Runs stereo pipeline        │
└──────────────────────────┘                           └──────────────────────────────┘
```

The first container is responsible for exporting the s2m2 model within the miniconda environment that the S2M2 authors use. Once the TensorRT model is succesfully exported, a different container is used to run the stereo vision pipeline, because in principle different packages are required there. 

## Table of contents
- [Near real-time 3D stereo reconstruction from drone imagery and SLAM pose estimations. - Branch : dockerized\_ros2impl](#near-real-time-3d-stereo-reconstruction-from-drone-imagery-and-slam-pose-estimations---branch--dockerized_ros2impl)
  - [Architecture: Two-Container Design](#architecture-two-container-design)
  - [Table of contents](#table-of-contents)
  - [How to use this branch](#how-to-use-this-branch)
    - [System Info](#system-info)
    - [Running the docker containers](#running-the-docker-containers)
    - [Debug - GDB support](#debug---gdb-support)
    - [Keeping params in sync](#keeping-params-in-sync)
  - [Acknowledgements](#acknowledgements)
  - [Contact](#contact)


## How to use this branch

### System Info
System has been tested on linux772 server of ITC. In case of setup on different  machine with different hardware, some dependencies might need to be adjusted within the dockerfiles. 

System info : 
- Ubuntu 22.04
- Ros2 Humble
- CUDA 13.0

### Running the docker containers

We will first use the *export* container to get the tensorRT engine file of the s2m2 model. From the thesis/ folder, run :  


1. Place the S2M2 model weights within alg-S2M2/
2. `bash docker/scripts/build_export.sh`
3. Once the container is built, we run it with : `bash docker/scripts/run_export.sh` The image will persist after logging out of the server, so this script does not need to be run every time. 
4. To enter the docker container from this point on, use `bash docker/scripts/enter_export.sh`
5. This container will automatically run the necessary scripts to export both the ONNX and the TensorRT engine of the S2M2 model. 

Then, we build and run the *inference* container that has the whole stereo vision pipeline similarily to the *ros2_impl* branch. 

1. `bash docker/scripts/build_inference.sh`
2. Once built, we run it once and it will persist, using `bash docker/scripts/run_inference.sh`
3. To enter the container, run `bash docker/scripts/enter_inference.sh`
4. From within the container, build and launch:
```
build     # colcon build the workspace (all cmake/cuda flags are included)
launch    # launch the full stereo pipeline
```

The `build` and `launch` commands work from any shell inside the container (no need to source anything manually).


### Debug - GDB support 

For debugging, gdb can be attached to a node, as an extra argument at launch time. I used xterm for the debugging terminal session. Example to activate debugging both s2m2 inference and the pointcloud node: 

`ros2 launch stereo_system_bringup_pkg stereo_system_bringup.launch.py gdb_debug_s2m2_node:=true gdb_debug_pointcloud_node:=true`

Also, make sure to enable DEBUG build from 
CMakeLists.txt : 

`set(CMAKE_BUILD_TYPE Debug)`

For best performance, switch back to release: 

`set(CMAKE_BUILD_TYPE Release)`


### Keeping params in sync

The native parameters live in `cpp_ws/ros2_impl/s2m2_inference_cpp_pkg/config/params.yaml`. The Docker image uses a minimal override file at `docker/inference/config/params_docker.yaml` that contains only container-specific filesystem paths (`/models/`, `/data/`).

**When you change any parameter in the native `params.yaml`** (e.g. model dimensions, thresholds, topic names), check whether the corresponding Docker override needs updating too:

- Path parameters (`engine_filepath`, `time_profiling_dirpath`, `rectified_images_dirpath`) — update in `params_docker.yaml` if the model filename or output paths change.  
- All other parameters (dimensions, thresholds, topic names) — already flow from the native file automatically; no Docker changes needed.

If unsure, always verify with `ros2 param dump /s2m2_node` after a fresh build.

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

If you have any questions regarding the code, please contact me at geoder.097@gmail.com.
















