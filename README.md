# GPURaytracer
A raytracer which uses DirectX Compute Shader to compute the Rays


Build Instructions
------------------
1. Download or clone the repository
2. Run GenerateProject.bat to generate a Visual Studio 2022 Solution (if you are using a different code editor, you need to edit the file)

Important: You need the Windows SDK, to run this app.


Adjusting the Raytracing Properties
-----------------------------------
In the Settings.h file are all properties of the raytracer, such as window width and height
Simply customize them and recompile the project


Known Issues
------------
- The program might trigger timeout detection when using complex scenes


This Raytracer was built and tested on a x64 machine using a graphics card with the AMD Polaris architecture
