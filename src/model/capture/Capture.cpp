#include "Capture.h"

Capture::Capture() : frames(vector<FramesPacket>()), recording(list<FramesPacket>())
{
}

void Capture::addToCaptureFrame(FramesPacket frame)
{
	frames.push_back(frame);
}

void Capture::addToCaptureRecording(FramesPacket frame)
{
	recording.push_back(frame);
}

list<FramesPacket> Capture::getRecording()
{
	return recording;
}

vector<FramesPacket> Capture::getFrames()
{
	return frames;
}