#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include <iostream>
#include <stdlib.h>
#include <thread>
#include <functional>

/*!
 * Camera class with callback
 * GNU GPL v3.0
 * (C) 2022-2026
 * [Ross Gardiner](https://github.com/rossGardiner)
 * [Adam Frew](https://github.com/Saweenbarra)
 * [Alban Joseph](https://github.com/albanjoseph)
 * [Lewis Russell](https://github.com/charger4241)
 * Bernd Porr
 */

 /**
  * Raw V4L parameters which can be queried with: "v4l2-ctl --device=/dev/v4l-subdev2 -L" and then use the
  * matching V4L2_CID_*** macro to change the value. For simple camera setups the device path will be
  * /dev/video0 but for cameras with an Image Processor pipelie and will be a subdevice. You can find 
  * these with "media-ctl -p -d 0".
  */
struct V4LParameter
{
    std::string devicePath; // the device path, for example /dev/v4l-subdev2
    int parameter; // the parameter, for example V4L2_CID_GAIN
    float value; // the value of the parameter. It's normalised between 0 and 1. boolan is either 0 or 1.
};

/**
 * The parameters passed to openCV.
 */
struct OpenCVparameters
{
    int deviceID = 0; // the ID of the video device. Default is /dev/video0
    unsigned int fourcc = 0; // the fourcc code of the capture format
    int width = 0; // the requested image width (0=default)
    int height = 0; // the requested image height (0=default)
};

/**
* Camera class which captures with openCV and returns frames in a callback
*/
class Camera
{
public:
    /**
     * Callback which is called when a new frame is available
     **/
    using OnFrame = std::function<void(const cv::Mat &)>;

    /**
     * Default constructor
     **/
    Camera() = default;

    /**
     * Starts the acquisition from the camera
     * and then the callback is called at the default framerate.
     * @param openCVparameters The parameters passed to openCV.
     * @param v4lParameters The parameters passed directly to an v4l subdevice.
     **/
    OpenCVparameters start(const OpenCVparameters openCVparameters = OpenCVparameters(),
                           const std::vector<V4LParameter> v4lParameters = {});

    /**
     * Stops the data aqusisition
     **/
    void stop();

    /**
     * Registers the callback which receives the frames.
     **/
    void registerFrameCallback(OnFrame sc)
    {
        onFrame = sc;
    }

private:
    void threadLoop();
    cv::VideoCapture videoCapture;
    std::thread cameraThread;
    bool isOn = false;
    OnFrame onFrame;
    void setV4Lparameter(const V4LParameter &v4lParameter);
};
