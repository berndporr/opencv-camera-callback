#include "camera.h"

/*!
 * Loops while camera is on to add frames to the pipeline
 */
void Camera::threadLoop()
{
	isOn = true;
	while (isOn)
	{
		cv::Mat cap;

		videoCapture.set(cv::CAP_PROP_CONVERT_RGB, true);
		videoCapture.read(cap);
		// check if we succeeded
		if (cap.empty())
		{
			std::cerr << "ERROR! blank frame grabbed\n";
			return;
		}
		if (onFrame)
		{
			onFrame(cap);
		}
	}
}

void Camera::setV4Lparameter(const V4LParameter &v4lParameter)
{
	int fd = open(v4lParameter.devicePath.c_str(), O_RDWR);
	if (fd < 0)
	{
		perror("Opening video device");
		return;
	}

	struct v4l2_queryctrl query;
	query.id = v4lParameter.parameter;
	if (ioctl(fd, VIDIOC_QUERYCTRL, &query) == 0)
	{
		int d = query.maximum - query.minimum;
		struct v4l2_control control;
		control.id = v4lParameter.parameter;
		control.value = query.minimum + (int)round(v4lParameter.value * d);
		if (control.value > query.maximum)
			control.value = query.maximum;
		if (control.value < query.minimum)
			control.value = query.minimum;
		if (ioctl(fd, VIDIOC_S_CTRL, &control) < 0)
		{
			perror("Setting Parameter");
		}
	}
	else
	{
		perror("Querying video device.");
		std::cerr << v4lParameter.devicePath << "," << v4lParameter.parameter << "," << v4lParameter.value << std::endl;
	}
}

/*!
 * Starts the worker thread recording
 */
OpenCVparameters Camera::start(OpenCVparameters openCVparameters,
							   const std::vector<V4LParameter> v4lParameters)
{
	for (const auto &p : v4lParameters)
	{
		setV4Lparameter(p);
	}
	videoCapture.open(openCVparameters.deviceID, cv::CAP_V4L2);
	if (openCVparameters.fourcc > 0)
	{
		videoCapture.set(cv::CAP_PROP_FOURCC, openCVparameters.fourcc);
	}
	if ((openCVparameters.width > 0) && (openCVparameters.height > 0))
	{
		videoCapture.set(cv::CAP_PROP_FRAME_WIDTH, openCVparameters.width);
		videoCapture.set(cv::CAP_PROP_FRAME_HEIGHT, openCVparameters.height);
	}
	openCVparameters.width = videoCapture.get(cv::CAP_PROP_FRAME_WIDTH);
	openCVparameters.height = videoCapture.get(cv::CAP_PROP_FRAME_HEIGHT);
	videoCapture.set(cv::CAP_PROP_CONVERT_RGB, 1);
	openCVparameters.fourcc = videoCapture.get(cv::CAP_PROP_FOURCC);
	if (openCVparameters.framerate > 0) {
		videoCapture.set(cv::CAP_PROP_FPS, openCVparameters.framerate);		
	}
	openCVparameters.framerate = videoCapture.get(cv::CAP_PROP_FPS);
	//starting capture
	cameraThread = std::thread(&Camera::threadLoop, this);
	return openCVparameters;
}

/*!
 * Frees thread resources and stops recording, must be called prior to program exit.
 */
void Camera::stop()
{
	isOn = false;
	cameraThread.join();
}
