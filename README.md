# Selenite - A NURBS Renderer utilising ray tracing

## Introduction
This project finds the intersection between a ray and a NURBS patch using Newton's method in compute shader. There are three steps in the rendering pipeline:
1) Convert the control points to B basis functions - This step can be done in the content cooker (which I haven't got a chance to make :D, but the code is there in the NurbsViewer project)
2) Multiply the B basis functions with corresponding control points and weights to convert B basis functions to rational functions. - This is similar to what vertex shaders do and can be done on either CPU or GPU. The computation complicity is low as there won't be too many control points to calculate.
3) Fire rays and trace them against the NURBS patches using Newton's method - This is computationally expensive and should be done on GPU. We should leverage ray tracing acceleration structures e.g. Oct trees and other algorithms to optimise this process.

## Building Instructions
1) Run Engine\Nurbs\GPU\Build.ps1 to build shaders
2) Build the project with the latest MSVC

## Current result
![image](https://github.com/a1q123456/Selenite/assets/1059544/8ebc709e-4341-4594-bcec-724278c14eb1)

I'm currently using the normal as the colour, which is not nice. It's not difficult to add textures to the surface in the future.

