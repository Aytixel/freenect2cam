#include <opencv2/opencv.hpp>

void float_to_y16(u_char *input_image_buffer, u_char *output_image_buffer, uint width, uint height);

void bgra_to_yuv420(u_char *input_image_buffer, u_char *output_image_buffer, uint width, uint height);

void yuv420_black_image(u_char* output_image_buffer, uint width, uint height);

void grayscale_black_image(u_char* output_image_buffer, uint width, uint height);