#pragma once

#include <opencv2/opencv.hpp>
#include "model/config/ConfigController.h"
#include "model/camera/Packet.h"
#include "wtypes.h"

class Renderer2D
{
public:
	Renderer2D(ConfigController* configController);
	void calculateProportions(float prop);
	void render(Packet* Packet);
	int getGuiFps();
private:
	int guiFps;
	int rows;
	int cols;
	float prop;
	int cameraWidth;
	int cameraHeight;
	int maxWidth;
	int maxHeight;
	int cameraNumber;
	int barHeight;		
};
