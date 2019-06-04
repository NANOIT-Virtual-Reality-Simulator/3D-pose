﻿#include "CalibrationController.h"

CalibrationController::CalibrationController(ConfigController* configController, SceneController* sceneController)
{
	int charucoCols = configController->getCharucoCols();
	int charucoRows = configController->getCharucoRows();
	float charucoSquareLength = configController->getCharucoSquareLength();
	float charucoMarkerLength = configController->getCharucoMarkerLength();

	this->sceneController = sceneController;
	this->dictionary = aruco::getPredefinedDictionary(aruco::DICT_6X6_250);
	this->board = aruco::CharucoBoard::create(charucoCols, charucoRows, charucoSquareLength, charucoMarkerLength, dictionary);
}

void CalibrationController::generateCheckboard(int charucoWidth, int charucoHeight, int charucoMargin)
{
	Mat boardImage;
	board->draw(Size(charucoWidth, charucoHeight), boardImage, charucoMargin, 1);
	imwrite("board.png", boardImage);
}

bool CalibrationController::calibrate(Scene scene, CalibrationType calibrationType)
{
	bool calibrationSuccess = false;

	if (calibrationType == CalibrationType::INTRINSICS)
	{
		calibrationSuccess = calculateIntrinsics(scene, CaptureType::CALIBRATION);
	} 
	else if (calibrationType == CalibrationType::EXTRINSICS)
	{
		calibrationSuccess = calculateExtrinsics(scene, CaptureType::CALIBRATION);
	}
	else
	{
		calibrationSuccess = calculatePoses(scene, CaptureType::CALIBRATION);
	}

	return calibrationSuccess;
}

bool CalibrationController::calculateIntrinsics(Scene scene, CaptureType captureType)
{
	map<int, Intrinsics*> intrinsics;
	int capturedFrames = sceneController->getCapturedFrames(scene, captureType);
	vector<int> capturedCameras = sceneController->getCapturedCameras(scene, captureType);

	#pragma omp parallel for
	for (int cameraIndex = 0; cameraIndex < (int)capturedCameras.size(); cameraIndex++)
	{
		Size frameSize;
		int cameraNumber = capturedCameras[cameraIndex];
		
		vector<vector<int>> allCharucoIds;
		vector<vector<Point2f>> allCharucoCorners;
		for (int frameNumber = 0; frameNumber < capturedFrames; frameNumber++)
		{
			vector<int> charucoIds = vector<int>();
			vector<Point2f> charucoCorners = vector<Point2f>();
			Mat frame = sceneController->getFrame(scene, captureType, cameraNumber, frameNumber);
			frameSize = frame.size();
			
			if (detectCharucoCorners(frame, 5, charucoIds, charucoCorners))
			{
				allCharucoIds.push_back(charucoIds);
				allCharucoCorners.push_back(charucoCorners);		
			}			
		}

		Mat cameraMatrix = Mat::eye(3, 3, CV_64F);
		Mat distortionCoeffs = Mat::zeros(1, 5, CV_64F);
		int calibrationFlags = CALIB_FIX_ASPECT_RATIO | CALIB_FIX_PRINCIPAL_POINT | CALIB_ZERO_TANGENT_DIST;
		
		double reprojectionError = aruco::calibrateCameraCharuco(allCharucoCorners, allCharucoIds, board, frameSize, cameraMatrix, distortionCoeffs, noArray(), noArray(), calibrationFlags);
		intrinsics[cameraNumber] = new Intrinsics(cameraMatrix, distortionCoeffs, reprojectionError);
	}

	sceneController->saveIntrinsics(scene, intrinsics);
	return true;
}

bool CalibrationController::calculateExtrinsics(Scene scene, CaptureType captureType)
{
	int capturedFrames = sceneController->getCapturedFrames(scene, captureType);
	vector<int> capturedCameras = sceneController->getCapturedCameras(scene, captureType);

	map<int, Extrinsics*> extrinsics;
	map<int, Intrinsics*> intrinsics = sceneController->getIntrinsics(scene);

	int originCamera = 0;
	Mat originCameraMatrix = intrinsics[originCamera]->getCameraMatrix();
	Mat originDistortionCoefficients = intrinsics[originCamera]->getDistortionCoefficients();

	bool foundPose = false;
	vector<int> charucoIds;
	vector<Point2f> charucoCorners;
	Mat frame = sceneController->getFrame(scene, captureType, originCamera, capturedFrames - 1);

	if (detectCharucoCorners(frame, 2, originCameraMatrix, originDistortionCoefficients, charucoIds, charucoCorners))
	{
		BOOST_LOG_TRIVIAL(warning) << "Found board in the floor of camera " << originCamera;

		Mat originRotationVector;
		Mat originTranslationVector;
		foundPose = aruco::estimatePoseCharucoBoard(charucoCorners, charucoIds, board, originCameraMatrix, originDistortionCoefficients, originRotationVector, originTranslationVector);

		if (foundPose)
		{
			Mat originRotationMatrix;
			cv::Rodrigues(originRotationVector, originRotationMatrix);

			Mat cameraRotation = originRotationMatrix.t();
			Mat cameraTranslation = (originRotationMatrix.t() * originTranslationVector) * (-1);

			extrinsics[originCamera] = new Extrinsics(cameraTranslation, cameraRotation, intrinsics[originCamera]->getReprojectionError());
		}		
	}

	if (!foundPose)
	{
		BOOST_LOG_TRIVIAL(warning) << "Did not find board in the floor of camera " << originCamera;

		Mat originTranslationVector = Mat::zeros(3, 1, CV_64F);
		Mat originRotationVector = Mat::zeros(3, 1, CV_64F);
		extrinsics[originCamera] = new Extrinsics(originTranslationVector, originRotationVector, intrinsics[originCamera]->getReprojectionError());
	}

	#pragma omp parallel for
	for (int cameraIndex = 1; cameraIndex < (int)capturedCameras.size(); cameraIndex++)
	{
		int cameraLeft = capturedCameras[cameraIndex - 1];
		int cameraRight = capturedCameras[cameraIndex];

		BOOST_LOG_TRIVIAL(warning) << "Calibrating for pair " << cameraLeft << "->" << cameraRight;

		int totalSamples = 0;		
		Size cameraSize = sceneController->getFrame(scene, captureType, cameraLeft, 0).size();
		Mat cameraMatrixLeft = intrinsics[cameraLeft]->getCameraMatrix();
		Mat cameraMatrixRight = intrinsics[cameraRight]->getCameraMatrix();
		Mat distortionCoefficientsLeft = intrinsics[cameraLeft]->getDistortionCoefficients();		
		Mat distortionCoefficientsRight = intrinsics[cameraRight]->getDistortionCoefficients();

		vector<vector<Point3f>> allObjects;
		vector<vector<Point2f>> allCornersLeft;
		vector<vector<Point2f>> allCornersRight;		
		for (int frameNumber = 0; frameNumber < capturedFrames; frameNumber++)
		{
			vector<int> idsLeft;
			vector<Point2f> cornersLeft;
			Mat frameLeft = sceneController->getFrame(scene, captureType, cameraLeft, frameNumber);
			if (!detectCharucoCorners(frameLeft, 4, cameraMatrixLeft, distortionCoefficientsLeft, idsLeft, cornersLeft))
			{
				continue;
			}

			vector<int> idsRight;
			vector<Point2f> cornersRight;
			Mat frameRight = sceneController->getFrame(scene, captureType, cameraRight, frameNumber);
			if (!detectCharucoCorners(frameRight, 4, cameraMatrixLeft, distortionCoefficientsRight, idsRight, cornersRight))
			{
				continue;
			}

			vector<Point3f> finalObjects;
			vector<Point2f> finalCornersRight;
			vector<Point2f> finalCornersLeft;
			for (int leftIdIndex = 0; leftIdIndex < (int)idsLeft.size(); leftIdIndex++)
			{
				for (int rightIdIndex = 0; rightIdIndex < (int)idsRight.size(); rightIdIndex++)
				{
					int idLeft = idsLeft[leftIdIndex];
					int idRight = idsRight[rightIdIndex];

					if (idLeft == idRight)
					{
						Point3f idPosition = board->chessboardCorners[idLeft];
						Point2f leftPosition = Point2f(cornersLeft[leftIdIndex].x, cornersLeft[leftIdIndex].y);
						Point2f rightPosition = Point2f(cornersRight[rightIdIndex].x, cornersRight[rightIdIndex].y);

						finalObjects.push_back(idPosition);
						finalCornersLeft.push_back(leftPosition);
						finalCornersRight.push_back(rightPosition);
					}
				}
			}

			if (finalObjects.size() > 3) 
			{
				allObjects.push_back(finalObjects);
				allCornersLeft.push_back(finalCornersLeft);
				allCornersRight.push_back(finalCornersRight);
				totalSamples += (int)finalObjects.size();
			}
		}

		BOOST_LOG_TRIVIAL(warning) << "Samples for pair " << cameraLeft << "->" << cameraRight << ": " << totalSamples;

		Mat translationVector;
		Mat rotationVector;
		Mat essentialMatrix;
		Mat fundamentalMatrix;
		double reprojectionError = cv::stereoCalibrate(allObjects, allCornersLeft, allCornersRight, cameraMatrixLeft, distortionCoefficientsLeft, cameraMatrixRight, distortionCoefficientsRight, cameraSize, rotationVector, translationVector, essentialMatrix, fundamentalMatrix);
		
		cv::Rodrigues(rotationVector, rotationVector);
		extrinsics[cameraRight] = new Extrinsics(translationVector, rotationVector, reprojectionError);

		BOOST_LOG_TRIVIAL(warning) << "Finished calibrating for pair " << cameraLeft << "->" << cameraRight;
	}

	for (int cameraNumber = 1; cameraNumber < (int)extrinsics.size(); cameraNumber++)
	{
		Mat composedTranslationVector;
		Mat composedRotationVector;
		Mat currentTranslationVector = extrinsics[cameraNumber]->getTranslationVector();
		Mat currentRotationVector = extrinsics[cameraNumber]->getRotationVector();
		Mat previousTranslationVector = extrinsics[cameraNumber - 1]->getTranslationVector();
		Mat previousRotationVector = extrinsics[cameraNumber - 1]->getRotationVector();

		cv::composeRT(previousRotationVector, previousTranslationVector, currentRotationVector, currentTranslationVector, composedRotationVector, composedTranslationVector);
		extrinsics[cameraNumber] = new Extrinsics(composedTranslationVector, composedRotationVector, extrinsics[cameraNumber]->getReprojectionError());
	}

	sceneController->saveExtrinsics(scene, extrinsics);
	return true;
}

bool CalibrationController::calculatePoses(Scene scene, CaptureType captureType)
{
	int capturedFrames = sceneController->getCapturedFrames(scene, captureType);
	vector<int> capturedCameras = sceneController->getCapturedCameras(scene, captureType);

	map<int, Intrinsics*> intrinsics = sceneController->getIntrinsics(scene);
	map<int, Extrinsics*> extrinsics = sceneController->getExtrinsics(scene);
	vector<Frame3D*> allPoses;	

	for (int frameNumber = 0; frameNumber < capturedFrames; frameNumber++)
	{
		BOOST_LOG_TRIVIAL(warning) << "Calculating poses for cameras of frame " << frameNumber;
		Frame3D* framePoses = new Frame3D();
		bool foundPoses = false;

		#pragma omp parallel for
		for (int cameraIndex = 0; cameraIndex < capturedCameras.size(); cameraIndex++)
		{
			int cameraNumber = capturedCameras[cameraIndex];
			Mat cameraMatrix = intrinsics[cameraNumber]->getCameraMatrix();
			Mat distortionCoefficients = intrinsics[cameraNumber]->getDistortionCoefficients();
			
			vector<int> charucoIds;
			vector<Point2f> charucoCorners;
			Mat frame = sceneController->getFrame(scene, captureType, cameraNumber, frameNumber);

			if (detectCharucoCorners(frame, 1, cameraMatrix, distortionCoefficients, charucoIds, charucoCorners))
			{
				Mat rotationVector;
				Mat translationVector;
				if (aruco::estimatePoseCharucoBoard(charucoCorners, charucoIds, board, cameraMatrix, distortionCoefficients, rotationVector, translationVector))
				{
					Mat composedRotationVector;
					Mat composedTranslationVector;
					Mat composedRotationMatrix;
					Mat cameraRotationVector = extrinsics[cameraNumber]->getRotationVector();
					Mat cameraTranslationVector = extrinsics[cameraNumber]->getTranslationVector();

					cv::composeRT(cameraRotationVector, cameraTranslationVector, rotationVector, translationVector, composedRotationVector, composedTranslationVector);
					cv::Rodrigues(composedRotationVector, composedRotationMatrix);

					list<Point3d> composedPoses;										
					Affine3d affineMatrix = Affine3d(composedRotationMatrix, composedTranslationVector);
					for (int cornerIndex = 0; cornerIndex < board->chessboardCorners.size(); cornerIndex++)
					{
						Mat homogeneousPoint = Mat(board->chessboardCorners[cornerIndex]);
						homogeneousPoint.convertTo(homogeneousPoint, CV_64F);
						homogeneousPoint.push_back(1.0);

						Point3d transformedPoint = Mat(affineMatrix.matrix * homogeneousPoint, Range(0, 3));
						composedPoses.push_back(transformedPoint);
					}

					if (!composedPoses.empty())
					{
						foundPoses = true;
						framePoses->addData(cameraNumber, composedPoses);
					}
				}
			}
		}

		if (!foundPoses)
		{
			delete framePoses;
			allPoses.push_back(nullptr);
		}
		else
		{
			allPoses.push_back(framePoses);
		}		
	}

	BOOST_LOG_TRIVIAL(warning) << "Dumping all the poses to disk";
	sceneController->savePoses(scene, captureType, allPoses);
	return true;
}

bool CalibrationController::detectCharucoCorners(Mat& inputImage, int minCharucoCorners, vector<int>& charucoIds, vector<Point2f>& charucoCorners)
{
	Ptr<aruco::DetectorParameters> params = aruco::DetectorParameters::create();
	params->cornerRefinementMethod = aruco::CORNER_REFINE_NONE;

	vector<int> arucoIds;
	vector<vector<Point2f>> arucoCorners;	
	aruco::detectMarkers(inputImage, dictionary, arucoCorners, arucoIds, params);

	if (arucoIds.size() > 0)
	{
		aruco::interpolateCornersCharuco(arucoCorners, arucoIds, inputImage, board, charucoCorners, charucoIds);

		if (charucoIds.size() >= minCharucoCorners)
		{
			return true;
		}
	}

	return false;
}

bool CalibrationController::detectCharucoCorners(Mat& inputImage, int minCharucoCorners, Mat& cameraMatrix, Mat& distortionCoefficients, vector<int>& charucoIds, vector<Point2f>& charucoCorners)
{
	Ptr<aruco::DetectorParameters> params = aruco::DetectorParameters::create();
	params->cornerRefinementMethod = aruco::CORNER_REFINE_NONE;

	vector<int> arucoIds;
	vector<vector<Point2f>> arucoCorners;
	aruco::detectMarkers(inputImage, dictionary, arucoCorners, arucoIds, params, noArray(), cameraMatrix, distortionCoefficients);

	if (arucoIds.size() > 0)
	{
		aruco::interpolateCornersCharuco(arucoCorners, arucoIds, inputImage, board, charucoCorners, charucoIds, cameraMatrix, distortionCoefficients);

		if (charucoIds.size() >= minCharucoCorners)
		{
			return true;
		}
	}

	return false;
}