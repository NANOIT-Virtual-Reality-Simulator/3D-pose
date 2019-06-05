#pragma once

#include <map>
#include <opencv2/opencv.hpp>
#include "model/calibration/Intrinsics.h"
#include "model/calibration/Extrinsics.h"
#include "model/video/Frame3D.h"

class Video3D
{
public:
	Video3D(std::vector<int> cameras, std::map<int, Intrinsics*> intrinsics, std::map<int, Extrinsics*> extrinsics);
	std::vector<int> getCameras();
	std::map<int, Intrinsics*> getIntrinsics();
	std::map<int, Extrinsics*> getExtrinsics();
	int getFrameNumber();
	Frame3D* getNextFrame();
	void addFrame(Frame3D* frame);		
private:
	int frameNumber;
	std::vector<int> cameras;
	std::map<int, Intrinsics*> intrinsics;
	std::map<int, Extrinsics*> extrinsics;
	std::vector<Frame3D*> frames;
};