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
struct V4LParameter
{
    std::string devicePath;
    int parameter;
    float value;
};

struct OpenCVparameters
{
    int deviceID = 0;
    unsigned int fourcc = 0;
    int width = 0;
    int height = 0;
};

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
     * and then the callback is called at the framerate.
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
