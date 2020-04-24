#pragma once

void switchCamera();
void enableFlashlight();
// OpenGL control part
void initGL();
void render(int width, int height, int rotation);

bool start();
bool stop();

void onResume();
void onPause();
void onDestroy();

