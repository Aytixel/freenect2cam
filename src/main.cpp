#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/logger.h>

#include <opencv2/opencv.hpp>

#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <fcntl.h>
#include <stdio.h>
#include <csignal>

#define FRAME_FORMAT V4L2_PIX_FMT_YUV420
#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080
#define BYTES_PER_PIXEL 1.5f

using namespace cv;
using namespace libfreenect2;

char *video_device;
unsigned char *dest;
int fd = -1;
bool running = true;

void handler(int s) {
    running = false;
}

class CustomFrameListener: public FrameListener {
    bool onNewFrame(Frame::Type type, Frame *frame) {
        if (type == Frame::Type::Color) {
            Mat mrgb(VIDEO_HEIGHT, VIDEO_WIDTH, CV_8UC4, frame->data);
            Mat myuv(BYTES_PER_PIXEL * VIDEO_HEIGHT, VIDEO_WIDTH, CV_8UC1, dest);

            cvtColor(mrgb, myuv, COLOR_BGR2YUV_I420);
            write(fd, dest, BYTES_PER_PIXEL * VIDEO_WIDTH * VIDEO_HEIGHT);
        }

        return false;
    }
};

int main(int argc, char *argv[]) {
    signal(SIGINT, handler);
    signal(SIGTERM, handler);

    if (argc < 2) {
        std::cerr << "Not enougth argument! You should enter at least the path to the virtual video device to use. Other argument will be ignored." << std::endl;
        return -1;
    }

    video_device = *(argv + 1);
    dest = (unsigned char*)malloc(BYTES_PER_PIXEL * VIDEO_WIDTH * VIDEO_HEIGHT);

    // open the virtual video device and configure it
    fd = open(video_device, O_RDWR);

    if (fd < 0) {
        std::cerr << "Can't open virtual video device!" << std::endl;
        return -1;
    }

	struct v4l2_format vid_format;

	memset(&vid_format, 0, sizeof(vid_format));

	vid_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	vid_format.fmt.pix.width = VIDEO_WIDTH;
	vid_format.fmt.pix.height = VIDEO_HEIGHT;
	vid_format.fmt.pix.pixelformat = FRAME_FORMAT;
	vid_format.fmt.pix.sizeimage = BYTES_PER_PIXEL * VIDEO_WIDTH * VIDEO_HEIGHT;
	vid_format.fmt.pix.field = V4L2_FIELD_NONE;
	vid_format.fmt.pix.bytesperline = BYTES_PER_PIXEL * VIDEO_WIDTH;
	vid_format.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;

	if (ioctl(fd, VIDIOC_S_FMT, &vid_format) == -1) {
        std::cerr << "Can't set virtual video device format!" << std::endl;
        return -1;
    }

    struct v4l2_streamparm vid_parm;

	memset(&vid_parm, 0, sizeof(vid_parm));

    vid_parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vid_parm.parm.capture.timeperframe.numerator = 1;
    vid_parm.parm.capture.timeperframe.denominator = 30;

    if (ioctl(fd, VIDIOC_S_PARM, &vid_parm) == -1) {
        std::cerr << "Can't set virtual video device frame rate!" << std::endl;
        return -1;
    }

    // open the kinect
    Freenect2 freenect2;
    Freenect2Device *dev = 0;

    // disable the freenect logger
    setGlobalLogger(NULL);

    if (freenect2.enumerateDevices() == 0) {
        std::cerr << "Kinect not connected!" << std::endl;
        return -1;
    }

    dev = freenect2.openDevice(freenect2.getDefaultDeviceSerialNumber());

    CustomFrameListener listener;

    dev->setColorFrameListener(&listener);
    dev->startStreams(true, false);

    while(running) {
        sleep(1);
    }

    dev->stop();
    dev->close();
    
    close(fd);
    free(dest);

    return 0;
}