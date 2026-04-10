# opencv camera callback class

Callback based camera class for openCV where the client registers a callback
which then receives the frames at the framerate of the camera.

It should run with the default parameters if you have a simple camera setup on `/dev/video0`.
However, especially on ARM architectures such as the Raspberry PI or Rock5 there is a looooong
singnal processing pipeline which you can obtain with `media-ctl -p -d </dev/mediaX>` with. In
particular the camera might be a subdevice so needs to be configured separately for example
`/dev/v4l-subdev2` on a Rock5.

Here is the camera class:
```
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

## V4L controls

### Finding out which device does what

```
media-ctl -p -d 0
```
is usually the raw image capture chain without any further processing. The last device is the camera itself:

```
- entity 63: m00_b_imx219 3-0010 (1 pad, 1 link, 0 routes)
             type V4L2 subdev subtype Sensor flags 0
             device node name /dev/v4l-subdev2
	pad0: SOURCE
		[stream:0 fmt:SRGGB10_1X10/3280x2464@10000/210000 field:none]
		-> "rockchip-csi2-dphy0":0 [ENABLED]
```
this can be seen as it has *only* a pad called SOURCE but no *sink*.

There might be a matching Image Signal Processor chain. This is in a different `/dev/media`. For example, `/dev/media2`:

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

- entity 60: rkcif-mipi-lvds2 (1 pad, 1 link, 0 routes)
             type V4L2 subdev subtype Unknown flags 0
             device node name /dev/v4l-subdev4
	pad0: SOURCE
		[stream:0 fmt:SRGGB10_1X10/3280x2464@10000/210000 field:none]
		-> "rkisp-isp-subdev":0 [ENABLED]
```
This shows that `/dev/v4l-subdev4` is again camera itself, that this feeds to the cropping subdev `/dev/v4l-subdev3` and finally that feeds to `/dev/video22` which can be captured!

So the parameters can be changed via the subdevices and devices. The `start()` method has a vector which allows to set parameters such as gain or horizontal flip.


# Credits

 * [Ross Gardiner](https://github.com/rossGardiner)
 * [Adam Frew](https://github.com/Saweenbarra) 
 * [Alban Joseph](https://github.com/albanjoseph)
 * [Lewis Russell](https://github.com/charger4241)
 * Bernd Porr
