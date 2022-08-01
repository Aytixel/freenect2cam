#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>

#include <opencv2/opencv.hpp>

#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <fcntl.h>
#include <stdio.h>
#include <cstring>
#include <assert.h>

#define VIDEO_DEVICE "/dev/video0"
#define FRAME_FORMAT V4L2_PIX_FMT_YUV420
#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080
#define BYTES_PER_PIXEL 1.5f

using namespace cv;

int main() {
    libfreenect2::Freenect2 freenect2;
    libfreenect2::Freenect2Device *dev = 0;
    libfreenect2::PacketPipeline *pipeline = 0;

    assert(freenect2.enumerateDevices() != 0);

    pipeline = new libfreenect2::OpenGLPacketPipeline();
    dev = freenect2.openDevice(freenect2.getDefaultDeviceSerialNumber(), pipeline);

    libfreenect2::SyncMultiFrameListener listener(libfreenect2::Frame::Color);
    libfreenect2::FrameMap frames;

    dev->setColorFrameListener(&listener);
    dev->start();

    int fd = open(VIDEO_DEVICE, O_RDWR);

	assert(fd >= 0);

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

	assert(ioctl(fd, VIDIOC_S_FMT, &vid_format) != -1);

    struct v4l2_streamparm vid_parm;

    vid_parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vid_parm.parm.capture.timeperframe.numerator = 1;
    vid_parm.parm.capture.timeperframe.denominator = 60;

    assert(ioctl(fd, VIDIOC_S_PARM, &vid_parm) != -1);

    unsigned char* dest = (unsigned char*)malloc(BYTES_PER_PIXEL * VIDEO_WIDTH * VIDEO_HEIGHT);

    while(true) {
        listener.waitForNewFrame(frames);
        
        libfreenect2::Frame *rgb = frames[libfreenect2::Frame::Color];

        Mat mrgb(VIDEO_HEIGHT, VIDEO_WIDTH, CV_8UC4, rgb->data);
        Mat myuv(BYTES_PER_PIXEL * VIDEO_HEIGHT, VIDEO_WIDTH, CV_8UC1, dest);
        
        cvtColor(mrgb, myuv, COLOR_RGBA2YUV_YV12);
        write(fd, dest, BYTES_PER_PIXEL * VIDEO_WIDTH * VIDEO_HEIGHT);

        listener.release(frames);
    }

    free(dest);

    dev->stop();
    dev->close();
    
    close(fd);

    return 0;
}