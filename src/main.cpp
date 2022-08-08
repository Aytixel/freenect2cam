#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/logger.h>

#include <opencv2/opencv.hpp>

#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <fcntl.h>
#include <csignal>
#include <dirent.h>
#include <string>
#include <stdio.h>

#define FRAME_FORMAT V4L2_PIX_FMT_YUV420
#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080
#define BYTES_PER_PIXEL 1.5f

using namespace std;
using namespace cv;
using namespace libfreenect2;

char *video_device_path;
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

char* itoa(int value){
    char *buf = new char;

    sprintf(buf,"%d",value);
	
    return buf;
}

void processdir(const struct dirent *dir) {
     puts(dir->d_name);
}

bool is_video_device_used() {
    // list all process except the current
    static char *pid = itoa(getpid());
    string proc_path("/proc/");
    DIR *proc_dir = opendir(proc_path.c_str());

    if (proc_dir != NULL) {
        struct dirent *proc_dir_ent;

        while ((proc_dir_ent = readdir(proc_dir)) != NULL) {
            if (proc_dir_ent->d_type == DT_DIR && strcmp(pid, proc_dir_ent->d_name)) {
                    // list all process handle
                    string process_path = string("/proc/") + proc_dir_ent->d_name + "/fd/";
                    DIR *process_dir = opendir(process_path.c_str());

                    if (process_dir != NULL) {
                        struct dirent *process_dir_ent;

                        while ((process_dir_ent = readdir(process_dir)) != NULL) {
                            // find if it has handle to the video device
                            if (process_dir_ent->d_type == DT_LNK) {
                                char *process_handle_path = realpath((process_path + process_dir_ent->d_name).c_str(), NULL);

                                if (process_handle_path != NULL && strcmp(process_handle_path, video_device_path) == 0) {
                                    free(process_handle_path);
                                    closedir(process_dir);
                                    closedir(proc_dir);

                                    return true;
                                }
                            }
                        }

                        closedir(process_dir);
                }
            }
        }

        closedir(proc_dir);
    } else {
        std::cerr << "Can't open /proc directory!" << std::endl;
    }

    return false;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handler);
    signal(SIGTERM, handler);

    if (argc < 2) {
        cerr << "Not enougth argument! You should enter at least the path to the virtual video device to use. Other argument will be ignored." << endl;
        return -1;
    }

    video_device_path = *(argv + 1);
    dest = (unsigned char*)calloc(BYTES_PER_PIXEL * VIDEO_WIDTH * VIDEO_HEIGHT, sizeof(unsigned char));

    // open the virtual video device and configure it
    fd = open(video_device_path, O_RDWR);

    if (fd < 0) {
        cerr << "Can't open virtual video device!" << endl;
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
        cerr << "Can't set virtual video device format!" << endl;
        return -1;
    }

    struct v4l2_streamparm vid_parm;

	memset(&vid_parm, 0, sizeof(vid_parm));

    vid_parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vid_parm.parm.capture.timeperframe.numerator = 1;
    vid_parm.parm.capture.timeperframe.denominator = 30;

    if (ioctl(fd, VIDIOC_S_PARM, &vid_parm) == -1) {
        cerr << "Can't set virtual video device frame rate!" << endl;
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

    bool video_device_used = false;

    while(true) {
        sleep(1);
        write(fd, dest, BYTES_PER_PIXEL * VIDEO_WIDTH * VIDEO_HEIGHT);

        bool video_device_in_use = is_video_device_used();

        if (video_device_used != video_device_in_use) {
            if (video_device_in_use) {
                cerr << "Kinect starting." << endl;

                dev->startStreams(true, false);
            } else {
                dev->stop();
                
                cerr << "Kinect shutdown." << endl;
            }

            video_device_used = video_device_in_use;
        }

        if (!running) {
            dev->stop();
            dev->close();

            close(fd);
            free(dest);

            return 0;
        }
    }
}