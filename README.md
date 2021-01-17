## Introduction

This project is the main code of our "Real-to-Sim Registration" algorithm. If you find our code is useful, please kindly refer to our paper
https://arxiv.org/abs/2011.00800

If you would like to simply reproduce our work without changing the main algorithm, the below repository is the one you're looking for. 
https://github.com/y8han/Registration

## Requirements

1. Python

2. Pybind11(optional)

3. PCL-Pointcloud library(optional)

## To start

1. Modify the CMakeList to suit your environment

2. Compile and build the project. It should result in a file named 'registration.cpython-xxxxxxx.so'. In the repository, a pre-compiled .so file is included. However, in most cases, you cannot use it directly. Please delete it once you build your own .so file.

3. Run the Python script 'example.py' following the instruction in the file. 

4. If you get everything right in place, you could find your results in './result/'. 

5. Now you're ready to make any modification to the algorithm.

## File structure

1. The algorithm is mainly represented in 'registration.cpp'.

2. './src/' and './include/' contain several utilities including one Mesh class defined by us. You may find some wonderful functions about mesh operation in './src/mesh.cpp'. 

## Recommended readings

1. PCL (Pointcloud library)

2. Nvidia Cuda (simulation environment and mesh class)
