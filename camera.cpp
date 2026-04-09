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
		printf("parameter range: %d to %d (step %d)\n",
			   query.minimum, query.maximum, query.step);
		int d = query.maximum - query.minimum;
		struct v4l2_control control;
		control.id = v4lParameter.parameter;
		control.value = query.minimum + (int)round(v4lParameter.value*d);
		if (control.value > query.maximum) control.value = query.maximum;
		if (control.value < query.minimum) control.value = query.minimum;
		if (ioctl(fd, VIDIOC_S_CTRL, &control) < 0)
		{
			perror("Setting Parameter");
		}
		else
		{
			printf("Parameter set to %d\n", control.value);
		}
	} else {
		perror("Querying video device.");
		std::cerr << v4lParameter.devicePath << "," << v4lParameter.parameter << "," << v4lParameter.value << std::endl;
	}
}

/*!
 * Starts the worker thread recording
 */
void Camera::start(int deviceID, const std::vector<V4LParameter> v4lParameters)
{
	for(const auto &p: v4lParameters) {
		setV4Lparameter(p);
	}
	videoCapture.open(deviceID, cv::CAP_V4L2);
	cameraThread = std::thread(&Camera::threadLoop, this);
}

/*!
 * Frees thread resources and stops recording, must be called prior to program exit.
 */
void Camera::stop()
{
	isOn = false;
	cameraThread.join();
}
