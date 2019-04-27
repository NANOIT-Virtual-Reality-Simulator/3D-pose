#pragma once

#include <string>
#include <opencv2/aruco/charuco.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include "model/calibration/IntrinsicCalibration.h"
#include "model/util/Config.h"

using namespace std;
using namespace cv;

class CalibrationController
{
public:
	CalibrationController(Config* config);
	void generateCheckboard(string pathToOutput);
	IntrinsicCalibration calculateIntrinsics(string pathToInput);
	int getMaxCheckboards();
private:
	string checkboardName;
	int checkboardWidth;
	int checkboardHeight;
	int checkboardMargin;
	int maxCheckboards;
	Ptr<aruco::Dictionary> dictionary;
	Ptr<aruco::CharucoBoard> board;
};