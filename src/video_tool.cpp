#include <video_tool.hpp>

using namespace cv;

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

void bgra_to_yuv420(u_char *input_image_buffer, u_char *output_image_buffer, uint width, uint height) {
    Mat mbgr(height, width, CV_8UC4, input_image_buffer);
    Mat myuv(1.5f * height, width, CV_8UC1, output_image_buffer);

    cvtColor(mbgr, myuv, COLOR_BGRA2YUV_I420);
}

void yuv420_black_image(u_char* output_image_buffer, uint width, uint height) {
    u_char *temp_black_image = (u_char*)calloc(3 * width * height, 1);
    Mat mrgb(height, width, CV_8UC3, temp_black_image);
    Mat myuv(1.5f * height, width, CV_8UC1, output_image_buffer);

    cvtColor(mrgb, myuv, COLOR_RGB2YUV_I420);
    free(temp_black_image);
}

void grayscale_black_image(u_char* output_image_buffer, uint width, uint height) {
    memset(output_image_buffer, 0, 2 * width * height);
}