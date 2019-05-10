#pragma once

#include <map>
#include <thread>
#include <atomic>
#include <opencv2/opencv.hpp>
#include <boost/log/trivial.hpp>
#include <cameralibrary.h>
#include "model/camera/FramesPacket.h"
#include "model/util/Config.h"

#define MAX_CAMERAS 32

using namespace std;
using namespace cv;
using namespace CameraLibrary;

class OptitrackCamera
{
public:
	OptitrackCamera(Config* config);
	bool startCameras(Core::eVideoMode mode);
	FramesPacket* captureFramesPacket();
	void stopCameras();
	void shutdownCameras();
private:
	map<int, int> camerasOrder;
	int camerasFps;
	int cameraCount;
	atomic<int> frameCount;
	CameraList list;
	Camera* camera[MAX_CAMERAS];
	cModuleSync* sync;	
};
