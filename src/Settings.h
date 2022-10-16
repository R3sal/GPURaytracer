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
#define RT_USE_BVH 1 //determines the usage of a bounding volume hierarchy (0: do not use BVH, 1: use BVH)

//camera settings
#define RT_CAMERA_FOV 1.5707964f //the field of view of the camera in radians
#define RT_CAMERA_NEARZ 0.01f //objects that are closer to the camera than this, are not rendered
#define RT_CAMERA_FARZ 10000.0f //objects that are farther away from the camera than this, are not rendered
#define RT_CAMERA_POSITION { -2.5f, 5.0f, -2.0f }; //the position of the camera
#define RT_CAMERA_FOCUS_POINT { 0.0f, 0.0f, 0.0f }; //the point, the camera will look at
#define RT_CAMERA_UP_DIRECTION { 0.0f, 1.0f, 0.0f }; //adjusts, if the camera leans in a certain direction