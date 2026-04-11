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
#include <atomic>

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
  * /dev/video0 but for cameras with an Image Processor pipelie it will be a subdevice. You can find 
  * these with "media-ctl -p -d 0".
  */
struct V4LParameter
{
    /**
     *  The device path, for example /dev/v4l-subdev2 
     * */
    std::string devicePath;

    /**
     * The V4L control parameter (V4L2_CID_*), for example V4L2_CID_GAIN
     */
    int parameter;

    /**
     * The value of the parameter. It's normalised between 0 and 1. Or for boolan it's either 0 or 1.
     */
    float value;
};

/**
 * The parameters passed to openCV.
 */
struct OpenCVparameters
{
    /**
     * The ID of the video device (/dev/videoX). Default is /dev/video0.
     * Find out with "media-ctl -p -d </dev/mediaX>" which /dev/videoX is
     * the capture device. On SBCs it's usually the output of the Image Signal Processor
     * after it has turned the raw camera data into YUV or RGB.
     */
    int deviceID = 0;

    /**
     * Forcing the fourcc code of the capture format. Default is 0.
     * Use "v4l2-ctl --list-formats-ext -d /dev/video23" to find out the allowed formats
     * and then you can set the FourCC code, for example "cv::VideoWriter::fourcc('N', 'V', '1', '2')".
     */
    unsigned int fourcc = 0;

    /**
     * The requested image width (0=default)
     */
    int width = 0;

    /**
     * The requested image height (0=default)
     */
    int height = 0;

    /**
     * The requested framerate (0=default)
     */
    int framerate = 0;
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
     * @returns The modified parameters are returned with the actual values for widths, framerate etc
     **/
    OpenCVparameters start(const OpenCVparameters openCVparameters = OpenCVparameters(),
                           const std::vector<V4LParameter> v4lParameters = {});

    /**
     * Stops the data aqusisition
     **/
    void stop();

    /**
     * Registers the callback which receives the frames.
     * @param sc The std::function which receives the openCV Mat with the frame.
     **/
    void registerFrameCallback(OnFrame sc)
    {
        onFrame = sc;
    }

private:
    void threadLoop();
    cv::VideoCapture videoCapture;
    std::thread cameraThread;
    std::atomic<bool> isOn = false;
    OnFrame onFrame;
    void setV4Lparameter(const V4LParameter &v4lParameter);
};
