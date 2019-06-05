#include "Video3D.h"

Video3D::Video3D(std::vector<int> cameras, std::map<int, Intrinsics*> intrinsics, std::map<int, Extrinsics*> extrinsics, std::map<int, cv::Mat> frustumImages)
{
	this->currentFrame = 0;
	this->cameras = cameras;
	this->intrinsics = intrinsics;
	this->extrinsics = extrinsics;
	this->frustums = frustums;
	this->frames = std::vector<Frame3D*>();
}

std::vector<int> Video3D::getCameras()
{
	return cameras;
}

std::map<int, Intrinsics*> Video3D::getIntrinsics()
{
	return intrinsics;
}

std::map<int, Extrinsics*> Video3D::getExtrinsics()
{
	return extrinsics;
}

std::map<int, cv::Mat> Video3D::getFrustums()
{
	return frustums;
}

Frame3D* Video3D::getNextFrame()
{
	currentFrame++;

	if (currentFrame == frames.size())
	{
		currentFrame = 0;
	}

	return frames[currentFrame];
}

void Video3D::addFrame(Frame3D* frame)
{
	frames.push_back(frame);
}