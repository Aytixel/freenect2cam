#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <fcntl.h>
#include <iostream>
#include <cstring>

#include <video_device.hpp>

using namespace std;

VideoDevice::VideoDevice(char *__video_device_path, uint __width, uint __height, uint __bytes_per_line, uint __pixel_format, uint __frame_rate) {
    open = true;
    image_size = __bytes_per_line * __height;

    width = __width;
    height = __height;
    bytes_per_line = __bytes_per_line;
    pixel_format = __pixel_format;
    frame_rate = __frame_rate;
    video_device_path = __video_device_path;

    // open the virtual video device and configure it
    fd = ::open(video_device_path, O_RDWR);

    if (fd < 0) {
        cerr << "Can't open virtual video device!" << endl;
        throw -1;
    }

    struct v4l2_format vid_format;

    memset(&vid_format, 0, sizeof(vid_format));

    vid_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    vid_format.fmt.pix.width = width;
    vid_format.fmt.pix.height = height;
    vid_format.fmt.pix.pixelformat = pixel_format;
    vid_format.fmt.pix.sizeimage = image_size;
    vid_format.fmt.pix.field = V4L2_FIELD_NONE;
    vid_format.fmt.pix.bytesperline = bytes_per_line;
    vid_format.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;

    if (ioctl(fd, VIDIOC_S_FMT, &vid_format) == -1) {
        cerr << "Can't set virtual video device format!" << endl;
        throw -1;
    }
    
    struct v4l2_streamparm vid_parm;

    memset(&vid_parm, 0, sizeof(vid_parm));

    vid_parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vid_parm.parm.capture.timeperframe.numerator = 1;
    vid_parm.parm.capture.timeperframe.denominator = frame_rate;

    if (ioctl(fd, VIDIOC_S_PARM, &vid_parm) == -1) {
        cerr << "Can't set virtual video device frame rate!" << endl;
        throw -1;
    }
}

ssize_t VideoDevice::feed_image(u_char *image_buffer) {
    return open ? write(fd, image_buffer, image_size) : 0;
}

void VideoDevice::close() {
    if (open) {
        ::close(fd);

        open = false;
    }
}