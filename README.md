# Middlebury Stereo Evaluation with S2M2

This repository includes a Middlebury evaluation harness under [MiddEval3_F](MiddEval3_F). The S2M2 integration lives in [MiddEval3_F/alg-S2M2](MiddEval3_F/alg-S2M2) and is invoked through the standard Middlebury scripts.

## Prerequisites

Before running the evaluation, make sure all of the following are available:

- The Middlebury data folders exist next to the SDK root, including:
  - [MiddEval3_F/trainingF](MiddEval3_F/trainingF)
  - [MiddEval3_F/testF](MiddEval3_F/testF)
- The pretrained S2M2 weights are present at:
  - [MiddEval3_F/alg-S2M2/weights/pretrain_weights](MiddEval3_F/alg-S2M2/weights/pretrain_weights)
- Python and the required S2M2 dependencies are installed (see [MiddEval3_F/alg-S2M2/environment.yml](MiddEval3_F/alg-S2M2/environment.yml)). Follow the
online instructions of S2M2 authors for setting up the environment at [S2M2 github](https://github.com/junhong-3dv/s2m2)
- Activated conda env : `conda activate s2m2` 
- c-shell installed : `sudo apt install csh`

## Run the full evaluation

From the repository root:

```bash
cd MiddEval3_F
./runalg F training S2M2
./runevalF F training 1.0 S2M2
```

This runs S2M2 on all training datasets and evaluates the generated disparity maps using full-resolution ground truth.

## Run on all datasets

To run S2M2 on both training and test scenes:

```bash
cd MiddEval3_F
./runalg F all S2M2
```

## Evaluate a single scene

To test a single dataset first:

```bash
cd MiddEval3_F
./runalg F Motorcycle S2M2
./runevalF F Motorcycle 1.0 S2M2
```

## Notes

- The S2M2 wrapper writes the files expected by the Middlebury harness:
  - disp0.pfm
  - time.txt
- The official full-resolution evaluation is done with [MiddEval3_F/runevalF](MiddEval3_F/runevalF), while [MiddEval3_F/runeval](MiddEval3_F/runeval) uses the same-resolution ground truth and is faster but less official.
