#include "window.h"

#include <QApplication>

// Main program
int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	int vidDevNo = 0;

	if (argc > 1) {
		vidDevNo = atoi(argv[1]);
	}

	// Create camera
	Camera camera;

	// Create the window
	Window window;

	// Show the window
	window.show();

	// Connect the camera to the window
	camera.registerFrameCallback([&](const cv::Mat &m){window.updateImage(m);});	

	// Call the window timerEvent function every 20 ms to refresh the image
	window.startTimer(20);

	// Start the camera
	camera.start(vidDevNo,{{"/dev/v4l-subdev2",V4L2_CID_GAIN,0.5}});

	// Execute the application. This is blocking till the user closes it.
	app.exec();

	camera.stop();
}
