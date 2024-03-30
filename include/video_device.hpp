#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <fcntl.h>
#include <iostream>
#include <cstring>

class VideoDevice {
    private:
        bool open;
        int fd;
        uint image_size;

    public:
        uint width;
        uint height;
        uint bytes_per_line;
        uint pixel_format;
        uint frame_rate;
        char *video_device_path;

        VideoDevice(char *video_device_path, uint width, uint height, uint bytes_per_line, uint pixel_format, uint frame_rate);

        ssize_t feed_image(u_char *image_buffer);

        void close();
};