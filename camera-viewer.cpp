#include "window.h"

#include <QApplication>

// Main program
int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stderr,"Usage: %s <video dev #>\n",argv[0]);
		return 0;
	}

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
	openCVparameters.deviceID = atoi(argv[1]);
	openCVparameters.framerate = 30;

	// Start the camera
	openCVparameters = camera.start(openCVparameters);

	if ( (0 == openCVparameters.height) || (0 == openCVparameters.width) ) {
		fprintf(stderr,"Capture device has frame size of zero. Exiting.\n");
		return -1;
	}

	fprintf(stderr, "Resolution: %d x %d\n", openCVparameters.width, openCVparameters.height);
	fprintf(stderr, "FOURCC:");
	for (long unsigned int i = 0; i < sizeof(openCVparameters.fourcc); i++)
		fprintf(stderr, "%c", ((char *)(&openCVparameters.fourcc))[i]);
	fprintf(stderr, "\n");

	// Execute the application. This is blocking till the user closes it.
	app.exec();

	camera.stop();
}
