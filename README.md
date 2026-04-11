# opencv camera callback class

Callback based camera class for openCV where the client registers a callback
which then receives the frames at the framerate of the camera.

It should run with the default parameters if you have a simple camera setup on `/dev/video0`.
However, especially on ARM architectures such as the Raspberry PI or Rock5 there is a looooong
singnal processing pipeline. On the Raspberry PI you could use libcamera which "knows" about
the signal processing pipelines and exposes only one libcamera device. However, on other
architectures such as the Rock5 you need to do it the hard way: understand the signal
processing pipeline, configure sub-devices and then capture from the /dev/videoX which
openCV can read and decde. See the 4VL deep dive further down.

Here is the camera class (`camera.h` and `camera.cpp`):
```
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
}
```

## QT demo

Install the opencv development packages:
```
sudo apt install libopencv-dev
```

and the QT development packages:
```
sudo apt-get install qt6-base-dev
```

```
cmake .
make
./camera-viewer
```

The demo displays the camera in a QT window and is an example how it's done.

## V4L deep dive

I use as an example the Rock5B+ with two cameras attached which won't work with 
libcamera so we def need to do it the hard way:

Obtain with `media-ctl -p -d </dev/mediaX>` the video processing pipeline:

```
media-ctl -p -d 0
```
is usually the *raw* image capture chain without any further image processing or decoding. 
Here, the *last device* is the *camera* itself:

```
- entity 63: m00_b_imx219 3-0010 (1 pad, 1 link, 0 routes)
             type V4L2 subdev subtype Sensor flags 0
             device node name /dev/v4l-subdev2
	pad0: SOURCE
		[stream:0 fmt:SRGGB10_1X10/3280x2464@10000/210000 field:none]
		-> "rockchip-csi2-dphy0":0 [ENABLED]
```
it *only* has a pad called *SOURCE* but no *sink*.

The camera is a V4L *subdevice*.
If you want to find out which parameters of the camera subdevice `/dev/v4l-subdev2` can be changed you can query them with:
```
v4l2-ctl --device=/dev/v4l-subdev2 -L

User Controls

                       exposure 0x00980911 (int)    : min=0 max=4095 step=1 default=1575 value=1575
                           gain 0x00980913 (int)    : min=256 max=43663 step=1 default=256 value=21960
                horizontal_flip 0x00980914 (bool)   : default=0 value=1
                  vertical_flip 0x00980915 (bool)   : default=0 value=1

Image Source Controls

              vertical_blanking 0x009e0901 (int)    : min=36 max=36 step=1 default=36 value=36
            horizontal_blanking 0x009e0902 (int)    : min=164 max=164 step=1 default=164 value=164
                  analogue_gain 0x009e0903 (int)    : min=256 max=2816 step=1 default=512 value=512

```
This raw camera device outputs Bayer `SRGGB10` which is not really directly usable as it needs to
be converted to YUV or RGB. For that reason there is a hardware Image Signal Processor on the Rock5B+. This
image processing chain is described by *another* `/dev/mediaX`, here `/dev/media2`:

```
media-ctl -p -d 2
Media controller API version 6.1.115

Media device information
------------------------
driver          rkisp0-vir0
model           rkisp0
serial          
bus info        platform:rkisp0-vir0
hw revision     0x0
driver version  6.1.115

Device topology
- entity 1: rkisp-isp-subdev (4 pads, 10 links, 0 routes)
            type V4L2 subdev subtype Unknown flags 0
            device node name /dev/v4l-subdev3
	pad0: SINK,MUST_CONNECT
		[stream:0 fmt:SRGGB10_1X10/3280x2464 field:none
		 crop.bounds:(0,0)/3280x2464
		 crop:(0,0)/3280x2464]
		<- "rkisp_rawrd0_m":0 []
		<- "rkisp_rawrd2_s":0 []
		<- "rkisp_rawrd1_l":0 []
		<- "rkcif-mipi-lvds2":0 [ENABLED]
	pad1: SINK
		<- "rkisp-input-params":0 [ENABLED]
	pad2: SOURCE
		[stream:0 fmt:YUYV8_2X8/3280x2464 field:none colorspace:smpte170m quantization:full-range
		 crop.bounds:(0,0)/3280x2464
		 crop:(0,0)/3280x2464]
		-> "rkisp_mainpath":0 [ENABLED]
		-> "rkisp_selfpath":0 [ENABLED]
		-> "rkisp_fbcpath":0 [ENABLED]
		-> "rkisp_iqtool":0 [ENABLED]
	pad3: SOURCE
		-> "rkisp-statistics":0 [ENABLED]

- entity 6: rkisp_mainpath (1 pad, 1 link)
            type Node subtype V4L flags 0
            device node name /dev/video22
	pad0: SINK
		<- "rkisp-isp-subdev":2 [ENABLED]
		
- entity 12: rkisp_selfpath (1 pad, 1 link)
             type Node subtype V4L flags 0
             device node name /dev/video23
	pad0: SINK
		<- "rkisp-isp-subdev":2 [ENABLED]

- entity 60: rkcif-mipi-lvds2 (1 pad, 1 link, 0 routes)
             type V4L2 subdev subtype Unknown flags 0
             device node name /dev/v4l-subdev4
	pad0: SOURCE
		[stream:0 fmt:SRGGB10_1X10/3280x2464@10000/210000 field:none]
		-> "rkisp-isp-subdev":0 [ENABLED]
```
This shows that `/dev/v4l-subdev4` is again the camera itself, that this feeds to the cropping subdev `/dev/v4l-subdev3` and finally that feeds to `/dev/video22` and `/dev/video23` which can be captured! 

The whole pipeline looks like this:

> imx219 sensor, /dev/v4l-subdev2, SRGGB10_1X10/3280x2464 &#10233; /dev/v4l-subdev4 &#10233; /dev/v4l-subdev3, YUYV8_2X8/3280x2464, crop &#10233; /dev/video22 & /dev/video23, various pixelformats


The ISP can convert to the following formats:

```
v4l2-ctl --list-formats-ext -d 23

ioctl: VIDIOC_ENUM_FMT
	Type: Video Capture Multiplanar

	[0]: 'UYVY' (UYVY 4:2:2)
		Size: Stepwise 32x32 - 1920x2464 with step 8/8
	[1]: 'NV16' (Y/UV 4:2:2)
		Size: Stepwise 32x32 - 1920x2464 with step 8/8
	[2]: 'NV61' (Y/VU 4:2:2)
		Size: Stepwise 32x32 - 1920x2464 with step 8/8
	[3]: 'NV21' (Y/VU 4:2:0)
		Size: Stepwise 32x32 - 1920x2464 with step 8/8
	[4]: 'NV12' (Y/UV 4:2:0)
		Size: Stepwise 32x32 - 1920x2464 with step 8/8
	[5]: 'NM21' (Y/VU 4:2:0 (N-C))
		Size: Stepwise 32x32 - 1920x2464 with step 8/8
	[6]: 'NM12' (Y/UV 4:2:0 (N-C))
		Size: Stepwise 32x32 - 1920x2464 with step 8/8
	[7]: 'GREY' (8-bit Greyscale)
		Size: Stepwise 32x32 - 1920x2464 with step 8/8
	[8]: 'RGBP' (16-bit RGB 5-6-5)
		Size: Stepwise 32x32 - 1920x2464 with step 8/8
```

Specify the required image format to the openCV parameters as the fourcc string, for example 'NV12'.

The overall init of the two cameras then looks like:

```
OpenCVparameters openCVparameters1;
openCVparameters1.deviceID = 23;
openCVparameters1.framerate = 10;
openCVparameters1.fourcc = cv::VideoWriter::fourcc('G', 'R', 'E', 'Y');
// 1st camera sensor is v4l-subdev2
camera1.start(openCVparameters1,
    {{"/dev/v4l-subdev2", V4L2_CID_GAIN, 0.5},
     {"/dev/v4l-subdev2", V4L2_CID_HFLIP, 1},
     {"/dev/v4l-subdev2", V4L2_CID_VFLIP, 1}});
OpenCVparameters openCVparameters2;
openCVparameters2.deviceID = 32;
openCVparameters2.framerate = 10;
openCVparameters2.fourcc = cv::VideoWriter::fourcc('G', 'R', 'E', 'Y');
// 2nd camera sensor is v4l-subdev7
camera2.start(openCVparameters2,
    {{"/dev/v4l-subdev7", V4L2_CID_GAIN, 0.5},
     {"/dev/v4l-subdev7", V4L2_CID_HFLIP, 1},
     {"/dev/v4l-subdev7", V4L2_CID_VFLIP, 1}});
```
where the 1st camera is on `/dev/video23` and the 2nd camera is on `/dev/video32`.
Using their sensor subdevices we directly configure the sensors to flip the image and
to apply a gain of 0.5.

# Credits

 * Bernd Porr, mail@berndporr.me.uk, bernd.porr@glasgow.ac.uk
 * [Ross Gardiner](https://github.com/rossGardiner)
 * [Adam Frew](https://github.com/Saweenbarra) 
 * [Alban Joseph](https://github.com/albanjoseph)
 * [Lewis Russell](https://github.com/charger4241)

