#pragma once

//window size
#define RT_WINDOW_WIDTH 1920
#define RT_WINDOW_HEIGHT 1080

//raytracing properties
#define RT_SCENE_FILENAME "assets/testobject5_1.obj"
#define RT_MAX_RAYS_PER_PIXEL 4; //number of rays per pixel, the higher this value, the better AA and DOF effects will be
#define RT_MAX_RAY_DEPTH 3; //after shooting this number of rays, we return to shooting a ray from the camera
#define RT_AA_SAMPLE_SPREAD 1.5f; //anti-aliasing: the bigger the value, the blurrier the image, disabled at 0.0f, default is 1.0f
#define RT_DOF_SAMPLE_SPREAD 0.0f; //depth of field: the bigger the value, the stronger the DOF effect, disabled at 0.0f, default is 1.0f