#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/registration.h>
#include <libfreenect2/packet_pipeline.h>
#include <libfreenect2/logger.h>

#include <opencv2/opencv.hpp>

#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <fcntl.h>
#include <csignal>
#include <dirent.h>

#include <video_device.hpp>

#define VIDEO_FRAME_RATE 30

#define COLOR_FRAME_FORMAT V4L2_PIX_FMT_YUV420
#define COLOR_VIDEO_WIDTH 1920
#define COLOR_VIDEO_HEIGHT 1080
#define COLOR_BYTES_PER_PIXEL 1.5f

#define IR_DEPTH_FRAME_FORMAT V4L2_PIX_FMT_Y16
#define IR_DEPTH_VIDEO_WIDTH 512
#define IR_DEPTH_VIDEO_HEIGHT 424
#define IR_DEPTH_BYTES_PER_PIXEL 2

using namespace std;
using namespace cv;
using namespace libfreenect2;

char **video_device_paths;
u_char **image_buffers;

VideoDevice *video_devices;

bool running = true;

Registration *registration;

void handler(int s) {
    running = false;
}

void float_to_y16(u_char *input_image_buffer, u_char *output_image_buffer, uint width, uint height) {
    uint float_image_size = 4 * width * height;
    float f;
    ushort us;

    for (uint i = 0, j = 0; i < float_image_size; i += 4, j += 2) {
        memcpy(&f, input_image_buffer + i, sizeof(f));

        us = f;

        output_image_buffer[j] = us & 0xFF00;
        output_image_buffer[j + 1] = us & 0x00FF;
    }
}

class CustomFrameListener: public FrameListener {
    bool onNewFrame(Frame::Type type, Frame *frame) {
        Mat mrgb(COLOR_VIDEO_HEIGHT, COLOR_VIDEO_WIDTH, CV_8UC4, frame->data);
        Mat myuv(COLOR_BYTES_PER_PIXEL * COLOR_VIDEO_HEIGHT, COLOR_VIDEO_WIDTH, CV_8UC1, image_buffers[0]);

        switch (type) {
            case Frame::Color:
                cvtColor(mrgb, myuv, COLOR_BGRA2YUV_I420);

                video_devices[0].feed_image(image_buffers[0]);
                break;
            case Frame::Ir:
                float_to_y16(frame->data, image_buffers[1], IR_DEPTH_VIDEO_WIDTH, IR_DEPTH_VIDEO_HEIGHT);

                video_devices[1].feed_image(image_buffers[1]);
                break;
            case Frame::Depth:
                float_to_y16(frame->data, image_buffers[2], IR_DEPTH_VIDEO_WIDTH, IR_DEPTH_VIDEO_HEIGHT);

                video_devices[2].feed_image(image_buffers[2]);
                break;
        }

        return false;
    }
};

void create_black_image(u_char* image) {
    u_char *temp_black_image = (u_char*)calloc(3 * COLOR_VIDEO_HEIGHT * COLOR_VIDEO_WIDTH, 1);

    Mat mrgb(COLOR_VIDEO_HEIGHT, COLOR_VIDEO_WIDTH, CV_8UC3, temp_black_image);
    Mat myuv(COLOR_BYTES_PER_PIXEL * COLOR_VIDEO_HEIGHT, COLOR_VIDEO_WIDTH, CV_8UC1, image);

    cvtColor(mrgb, myuv, COLOR_RGB2YUV_I420);
    free(temp_black_image);
}

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

                                if (
                                    process_handle_path != NULL &&
                                    (
                                        strcmp(process_handle_path, video_device_paths[0]) == 0 ||
                                        strcmp(process_handle_path, video_device_paths[1]) == 0 ||
                                        strcmp(process_handle_path, video_device_paths[2]) == 0
                                    )
                                ) {
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
        cerr << "Can't open /proc directory!" << endl;
    }

    return false;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handler);
    signal(SIGTERM, handler);

    if (argc < 4) {
        cerr << "Not enougth argument! You should enter at least the path to the 3 virtual video device to use. Other argument will be ignored." << endl;
        return -1;
    }

    video_device_paths = (char**)malloc(3 * sizeof(char*));

    video_device_paths[0] = *(argv + 1);
    video_device_paths[1] = *(argv + 2);
    video_device_paths[2] = *(argv + 3);

    image_buffers = (u_char**)malloc(3 * sizeof(u_char*));

    image_buffers[0] = (u_char*)malloc(COLOR_BYTES_PER_PIXEL * COLOR_VIDEO_WIDTH * COLOR_VIDEO_HEIGHT);
    image_buffers[1] = (u_char*)malloc(IR_DEPTH_BYTES_PER_PIXEL * IR_DEPTH_VIDEO_WIDTH * IR_DEPTH_VIDEO_HEIGHT);
    image_buffers[2] = (u_char*)malloc(IR_DEPTH_BYTES_PER_PIXEL * IR_DEPTH_VIDEO_WIDTH * IR_DEPTH_VIDEO_HEIGHT);

    create_black_image(image_buffers[0]);

    video_devices = (VideoDevice*)malloc(3 * sizeof(VideoDevice));

    video_devices[0] = VideoDevice(video_device_paths[0], COLOR_VIDEO_WIDTH, COLOR_VIDEO_HEIGHT, COLOR_BYTES_PER_PIXEL * COLOR_VIDEO_WIDTH, COLOR_FRAME_FORMAT, VIDEO_FRAME_RATE);
    video_devices[1] = VideoDevice(video_device_paths[1], IR_DEPTH_VIDEO_WIDTH, IR_DEPTH_VIDEO_HEIGHT, IR_DEPTH_BYTES_PER_PIXEL * IR_DEPTH_VIDEO_WIDTH, IR_DEPTH_FRAME_FORMAT, VIDEO_FRAME_RATE);
    video_devices[2] = VideoDevice(video_device_paths[2], IR_DEPTH_VIDEO_WIDTH, IR_DEPTH_VIDEO_HEIGHT, IR_DEPTH_BYTES_PER_PIXEL * IR_DEPTH_VIDEO_WIDTH, IR_DEPTH_FRAME_FORMAT, VIDEO_FRAME_RATE);

    // open the kinect
    Freenect2 freenect2;
    Freenect2Device *dev;
    PacketPipeline *pipeline;

    // disable the freenect logger
    setGlobalLogger(NULL);

    while (freenect2.enumerateDevices() == 0) {
        usleep(1000 * 100);

        if (!running) {
            video_devices[0].close();
            video_devices[1].close();
            video_devices[2].close();

            free(video_device_paths);
            free(image_buffers[0]);
            free(image_buffers[1]);
            free(image_buffers[2]);
            free(image_buffers);
            free(video_devices);

            cout << "Kinect close." << endl;

            return 0;
        }
    }

    pipeline = new CudaPacketPipeline();
    dev = freenect2.openDevice(freenect2.getDefaultDeviceSerialNumber(), pipeline);

    CustomFrameListener listener;

    dev->setColorFrameListener(&listener);
    dev->setIrAndDepthFrameListener(&listener);

    bool video_device_used = false;

    while(true) {
        sleep(1);
            
        video_devices[0].feed_image(image_buffers[0]);

        bool video_device_in_use = is_video_device_used();

        if (video_device_used != video_device_in_use) {
            if (video_device_in_use) {
                cout << "Kinect starting." << endl;

                dev->start();
                registration = new libfreenect2::Registration(dev->getIrCameraParams(), dev->getColorCameraParams());

                Frame undistorted(512, 424, 4), registered(512, 424, 4);
            } else {
                dev->stop();

                create_black_image(image_buffers[0]);

                cout << "Kinect shutdown." << endl;
            }

            video_device_used = video_device_in_use;
        }

        if (!running) {
            dev->stop();
            dev->close();
            video_devices[0].close();
            video_devices[1].close();
            video_devices[2].close();

            free(video_device_paths);
            free(image_buffers[0]);
            free(image_buffers[1]);
            free(image_buffers[2]);
            free(image_buffers);
            free(video_devices);

            cout << "Kinect close." << endl;

            return 0;
        }
    }
}