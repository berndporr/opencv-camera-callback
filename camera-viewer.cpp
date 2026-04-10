#include "window.h"

#include <QApplication>

// Main program
int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	// Create camera
	Camera camera;

	// Create the window
	Window window;

	// Show the window
	window.show();

	// Connect the camera to the window
	camera.registerFrameCallback([&](const cv::Mat &m)
								 { window.updateImage(m); });

	// Call the window timerEvent function every 20 ms to refresh the image
	window.startTimer(20);

	OpenCVparameters openCVparameters;
	openCVparameters.fourcc = cv::VideoWriter::fourcc('N', 'V', '1', '2');
	if (argc > 1)
	{
		openCVparameters.deviceID = atoi(argv[1]);
	}

	// Start the camera
	openCVparameters = camera.start(openCVparameters,
									{{"/dev/v4l-subdev2", V4L2_CID_GAIN, 0.5}});

	fprintf(stderr, "Resolution: %d x %d\n", openCVparameters.width, openCVparameters.height);
	fprintf(stderr, "FOURCC:");
	for (long unsigned int i = 0; i < sizeof(openCVparameters.fourcc); i++)
		fprintf(stderr, "%c", ((char *)(&openCVparameters.fourcc))[i]);
	fprintf(stderr, "\n");

	// Execute the application. This is blocking till the user closes it.
	app.exec();

	camera.stop();
}
