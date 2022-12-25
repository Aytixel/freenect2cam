#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/packet_pipeline.h>
#include <libfreenect2/logger.h>

#include <linux/videodev2.h>

#include <csignal>
#include <dirent.h>
#include <iostream>
#include <cstring>

#include <video_device.hpp>
#include <video_tool.hpp>

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
using namespace libfreenect2;

char **video_device_paths;
u_char **image_buffers;

VideoDevice *video_devices;

bool running = true;

void handler(int s) {
    running = false;
}

class CustomFrameListener: public FrameListener {
    bool onNewFrame(Frame::Type type, Frame *frame) {
        switch (type) {
            case Frame::Color:
                bgra_to_yuv420(frame->data, image_buffers[0], COLOR_VIDEO_WIDTH, COLOR_VIDEO_HEIGHT);

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

                                free(process_handle_path);
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
    image_buffers[1] = (u_char*)calloc(IR_DEPTH_BYTES_PER_PIXEL * IR_DEPTH_VIDEO_WIDTH * IR_DEPTH_VIDEO_HEIGHT, sizeof(u_char));
    image_buffers[2] = (u_char*)calloc(IR_DEPTH_BYTES_PER_PIXEL * IR_DEPTH_VIDEO_WIDTH * IR_DEPTH_VIDEO_HEIGHT, sizeof(u_char));

    yuv420_black_image(image_buffers[0], COLOR_VIDEO_WIDTH, COLOR_VIDEO_HEIGHT);

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
        video_devices[1].feed_image(image_buffers[1]);
        video_devices[2].feed_image(image_buffers[2]);

        bool video_device_in_use = is_video_device_used();

        if (video_device_used != video_device_in_use) {
            if (video_device_in_use) {
                cout << "Kinect starting." << endl;

                dev->start();
            } else {
                dev->stop();

                yuv420_black_image(image_buffers[0], COLOR_VIDEO_WIDTH, COLOR_VIDEO_HEIGHT);
                grayscale_black_image(image_buffers[1], IR_DEPTH_VIDEO_WIDTH, IR_DEPTH_VIDEO_HEIGHT);
                grayscale_black_image(image_buffers[2], IR_DEPTH_VIDEO_WIDTH, IR_DEPTH_VIDEO_HEIGHT);

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