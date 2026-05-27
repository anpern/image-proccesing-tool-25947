#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H

#include <stdint.h>

//cтруктура для хранения изображения
typedef struct{
    uint8_t* data;     // массив пикселей
    int width;         // ширина
    int height;        // высота
    int channels;      // 1 (серый) или 3 (RGB)
} Image;

//операции
Image* load_image(const char* filename);
int save_image(Image* img, const char* filename);
void free_image(Image* img);
Image* convert_to_grayscale(Image* img);

//фильтры
Image* convolution(Image* img, float* kernel, int kernel_size);
Image* median_filter(Image* img, int window_size);
Image* gaussian_filter(Image* img, int kernel_size, float sigma);
Image* detect_edges(Image* img, int threshold);
Image* adjust_brightness(Image* img, int delta);

#endif