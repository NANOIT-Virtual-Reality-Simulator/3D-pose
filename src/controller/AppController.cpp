#include "AppController.h"

AppController::AppController()
{
	property_tree::ptree root;
	property_tree::read_json("app-config.json", root);

	string path = root.get<string>("config.path");
	int maxCheckboards = root.get<int>("config.maxCheckboards");
	int cameraFps = root.get<int>("config.cameraFps");

	this->sceneController = new SceneController(path);
	this->calibrationController = new CalibrationController(maxCheckboards);
	this->cameraController = new CameraController(cameraFps);
}

bool AppController::sceneExists(string name)
{
	return sceneController->sceneExists(name);
}

Scene AppController::createScene(string name)
{
	return sceneController->createScene(name);
}

Scene AppController::loadScene(string name)
{
	return sceneController->loadScene(name);
}

bool AppController::hasCapture(Scene scene, Operation operation)
{
	return sceneController->hasCapture(scene, operation);
}

void AppController::deleteCapture(Scene scene, Operation operation)
{
	sceneController->deleteCapture(scene, operation);
}

bool AppController::startCameras(CaptureMode captureMode)
{
	return cameraController->startCameras(captureMode);
}

void AppController::stopCameras()
{
	cameraController->stopCameras();
}

void AppController::captureFrame()
{
	cameraController->captureFrame();
}

void AppController::startRecordingFrames()
{
	cameraController->startRecording();
}

void AppController::stopRecordingFrames()
{
	cameraController->stopRecording();
}

void AppController::dumpCapture(Scene scene, Operation operation)
{
	sceneController->saveCapture(scene, operation, cameraController->getCapture());
}

int AppController::getCamerasFps()
{
	return cameraController->getCamerasFps();
}

FramesPacket* AppController::getSafeFrame()
{
	return cameraController->getSafeFrame();
}

void AppController::updateSafeFrame()
{
	cameraController->updateSafeFrame();
}

int AppController::getMaxCheckboards()
{
	return calibrationController->getMaxCheckboards();
}