#pragma once

#include <opencv2/opencv.hpp>
#include "model/camera/FramesPacket.h"

using namespace cv;

class CameraRenderer
{
public:
	CameraRenderer(int barHeight, int cameraHeight, int cameraWidth, int rows, int cols);
	void render(FramesPacket framesPacket);
private:
	int barHeight;
	int cameraWidth;
	int cameraHeight;
	int rows;
	int cols;
};
