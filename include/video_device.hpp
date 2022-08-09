#include <unistd.h>

class VideoDevice {
    private:
        bool open;
        int fd;
        unsigned int image_size;

    public:
        unsigned int width;
        unsigned int height;
        unsigned int bytes_per_line;
        unsigned int pixel_format;
        unsigned int frame_rate;
        char *video_device_path;

        VideoDevice(char *video_device_path, unsigned int width, unsigned int height, unsigned int bytes_per_line, unsigned int pixel_format, unsigned int frame_rate);

        ssize_t feed_image(unsigned char *image_buffer);

        void close();
};