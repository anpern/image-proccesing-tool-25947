#include "image_processing.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//подключаем stb_image (это должно быть в только одном .c файле)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image_read.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

//безопасное получение пикселя с зеркальным отражением на границах
//если координата выходит за пределы изображения, она отражается зеркально
static uint8_t get_pixel_safe(Image* img, int x, int y, int channel){
    //отражаем отрицательные координаты (левый и верхний края)
    if (x < 0) x = -x;
    if (y < 0) y = -y;
    //отражаем координаты, превышающие размеры (правый и нижний края)
    if (x >= img->width) x = 2 * img->width - x - 2;
    if (y >= img->height) y = 2 * img->height - y - 2;
    //вычисляем индекс в одномерном массиве пикселей
    int index = (y * img->width + x) * img->channels + channel;
    return img->data[index];
}

//сортировка пузырьком для медианного фильтра
//сортирует массив по возрастанию, сравнивая соседние элементы
static void bubble_sort(uint8_t* arr, int n){
    for (int i = 0; i < n - 1; i++){
        for (int j = 0; j < n - i - 1; j++){
            if (arr[j] > arr[j + 1]){
                //меняем местами, если левый больше правого
                uint8_t temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

//загрузка изображения из файла
//использует stb_image для чтения png, jpg, bmp
Image* load_image(const char* filename){
    //выделяем память под структуру изображения
    Image* img = malloc(sizeof(Image));
    if (!img) return NULL;
    //загружаем данные пикселей через stb_image
    img->data = stbi_load(filename, &img->width, &img->height, &img->channels, 0);
    if (!img->data){
        free(img);
        return NULL;
    }
    //выводим информацию о загруженном изображении
    printf("загружено: %s (%dx%d, %d каналов)\n", filename, img->width, img->height, img->channels);
    return img;
}

//сохранение изображения в файл
//формат определяется по расширению файла
int save_image(Image* img, const char* filename){
    if (!img || !img->data) return 0;
    
    int success = 0;
    //находим последнюю точку в имени файла (расширение)
    const char* ext = strrchr(filename, '.');
    //выбираем формат сохранения в зависимости от расширения
    if (ext && strcmp(ext, ".png") == 0){
        success = stbi_write_png(filename, img->width, img->height, img->channels, img->data, img->width * img->channels);
    } 
    else if (ext && (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)){
        success = stbi_write_jpg(filename, img->width, img->height, img->channels, img->data, 90);
    } 
    else if (ext && strcmp(ext, ".bmp") == 0){
        success = stbi_write_bmp(filename, img->width, img->height, img->channels, img->data);
    } 
    else {
        fprintf(stderr, "неподдерживаемый формат: %s\n", ext ? ext : "unknown");
        return 0;
    }
    
    if (success){
        printf("сохранено: %s\n", filename);
    }
    else {
        fprintf(stderr, "ошибка сохранения: %s\n", filename);
    }
    return success;
}

//освобождение памяти, занятой изображением
void free_image(Image* img){
    if (img){
        if (img->data) stbi_image_free(img->data);
        free(img);
    }
}

//преобразование цветного изображения в оттенки серого
//используется формула яркости: 0.299*R + 0.587*G + 0.114*B
Image* convert_to_greyscale(Image* img){
    if (!img || img->channels == 1) return NULL;
    //создаём новое изображение с одним каналом
    Image* grey = malloc(sizeof(Image));
    grey->width = img->width;
    grey->height = img->height;
    grey->channels = 1;
    grey->data = malloc(img->width * img->height);
    //проходим по всем пикселям
    for (int y = 0; y < img->height; y++){
        for (int x = 0; x < img->width; x++){
            int idx = (y * img->width + x) * img->channels;
            uint8_t r = img->data[idx];
            uint8_t g = img->data[idx + 1];
            uint8_t b = img->data[idx + 2];
            //вычисляем яркость и сохраняем
            grey->data[y * img->width + x] = (uint8_t)(0.299f * r + 0.587f * g + 0.114f * b);
        }
    }
    return grey;
}

//свёртка изображения с ядром
//ядро должно быть нечётного размера (3x3, 5x5 и т.д.)
Image* convolution(Image* img, float* kernel, int kernel_size){
    if (!img || !kernel || kernel_size % 2 == 0) return NULL;
    //pad - количество пикселей от центра до края ядра
    int pad = kernel_size / 2;
    Image* result = malloc(sizeof(Image));
    result->width = img->width;
    result->height = img->height;
    result->channels = img->channels;
    result->data = malloc(img->width * img->height * img->channels);
    //проходим по каждому пикселю выходного изображения
    for (int y = 0; y < img->height; y++){
        for (int x = 0; x < img->width; x++){
            //обрабатываем каждый цветовой канал отдельно
            for (int c = 0; c < img->channels; c++){
                float sum = 0.0f;
                //проходим по окну размером kernel_size x kernel_size
                for (int ky = -pad; ky <= pad; ky++){
                    for (int kx = -pad; kx <= pad; kx++){
                        //получаем соседний пиксель с зеркальным отражением на границах
                        uint8_t pixel = get_pixel_safe(img, x + kx, y + ky, c);
                        //получаем коэффициент ядра
                        float k_val = kernel[(ky + pad) * kernel_size + (kx + pad)];
                        //накапливаем взвешенную сумму
                        sum += pixel * k_val;
                    }
                }
                //приводим результат к диапазону 0-255
                int val = (int)sum;
                if (val < 0) val = 0;
                if (val > 255) val = 255;
                //сохраняем результат
                result->data[(y * img->width + x) * img->channels + c] = (uint8_t)val;
            }
        }
    }
    return result;
}

//медианный фильтр для удаления шума
//заменяет каждый пиксель на медиану значений в окне
Image* median_filter(Image* img, int window_size){
    if (!img || window_size % 2 == 0) return NULL;
    
    int pad = window_size / 2;
    int window_area = window_size * window_size;
    //временный массив для хранения пикселей окна
    uint8_t* window = malloc(window_area);
    
    Image* result = malloc(sizeof(Image));
    result->width = img->width;
    result->height = img->height;
    result->channels = img->channels;
    result->data = malloc(img->width * img->height * img->channels);
    //проходим по всем пикселям
    for (int y = 0; y < img->height; y++){
        for (int x = 0; x < img->width; x++){
            for (int c = 0; c < img->channels; c++){
                int idx = 0;
                //заполняем окно пикселями
                for (int ky = -pad; ky <= pad; ky++){
                    for (int kx = -pad; kx <= pad; kx++){
                        window[idx++] = get_pixel_safe(img, x + kx, y + ky, c);
                    }
                }
                //сортируем окно
                bubble_sort(window, window_area);
                //берём медиану (средний элемент отсортированного массива)
                result->data[(y * img->width + x) * img->channels + c] = window[window_area / 2];
            }
        }
    }
    free(window);
    return result;
}

//создание гауссовского ядра для размытия
//sigma контролирует степень размытия (чем больше, тем сильнее)
static float* create_gaussian_kernel(int size, float sigma, int* out_size){
    *out_size = size;
    float* kernel = malloc(size * size * sizeof(float));
    float sum = 0.0f;
    int center = size / 2;
    //вычисляем значения по формуле гаусса
    for (int i = 0; i < size; i++){
        for (int j = 0; j < size; j++){
            int x = i - center;
            int y = j - center;
            //формула гаусса: e^(-(x^2+y^2)/(2*sigma^2))
            kernel[i * size + j] = expf(-(x*x + y*y) / (2 * sigma * sigma));
            sum += kernel[i * size + j];
        }
    }
    //нормализация: делим каждый элемент на сумму, чтобы общая сумма была равна 1
    for (int i = 0; i < size * size; i++){
        kernel[i] /= sum;
    }
    return kernel;
}

//гауссовский фильтр для плавного размытия
Image* gaussian_filter(Image* img, int kernel_size, float sigma){
    if (!img || kernel_size % 2 == 0) return NULL;
    //создаём ядро гаусса
    int size;
    float* kernel = create_gaussian_kernel(kernel_size, sigma, &size);
    //применяем свёртку
    Image* result = convolution(img, kernel, size);
    free(kernel);
    return result;
}

//детектор границ с использованием оператора собеля
//threshold - порог: если >0, оставляет только сильные границы
Image* detect_edges(Image* img, int threshold){
    if (!img) return NULL;
    //преобразуем в оттенки серого, если нужно
    Image* grey = img;
    int own_grey = 0;
    if (img->channels == 3){
        grey = convert_to_greyscale(img);
        own_grey = 1;
    }
    //ядра собеля для поиска градиента по x и y
    float sobel_x[9] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};
    float sobel_y[9] = {-1, -2, -1, 0, 0, 0, 1, 2, 1};
    //применяем свёртку с обоими ядрами
    Image* gx = convolution(grey, sobel_x, 3);
    Image* gy = convolution(grey, sobel_y, 3);
    //создаём изображение для границ
    Image* edges = malloc(sizeof(Image));
    edges->width = grey->width;
    edges->height = grey->height;
    edges->channels = 1;
    edges->data = malloc(grey->width * grey->height);
    //вычисляем величину градиента: sqrt(gx^2 + gy^2)
    float max_grad = 0;
    for (int i = 0; i < grey->width * grey->height; i++){
        float grad = sqrtf(gx->data[i] * gx->data[i] + gy->data[i] * gy->data[i]);
        if (grad > max_grad) max_grad = grad;
        edges->data[i] = (uint8_t)grad;
    }
    //нормализуем и применяем порог
    for (int i = 0; i < grey->width * grey->height; i++){
        float norm = (edges->data[i] / max_grad) * 255.0f;
        if (threshold > 0){
            //если превышает порог - белый, иначе - чёрный
            edges->data[i] = (norm > threshold) ? 255 : 0;
        } else {
            edges->data[i] = (uint8_t)norm;
        }
    }
    //освобождаем временные изображения
    free_image(gx);
    free_image(gy);
    if (own_grey) free_image(grey);
    return edges;
}

//изменение яркости изображения
//delta - значение, прибавляемое к каждому каналу каждого пикселя
Image* adjust_brightness(Image* img, int delta){
    if (!img) return NULL;
    
    Image* result = malloc(sizeof(Image));
    result->width = img->width;
    result->height = img->height;
    result->channels = img->channels;
    result->data = malloc(img->width * img->height * img->channels);
    
    int total = img->width * img->height * img->channels;
    for (int i = 0; i < total; i++){
        int val = img->data[i] + delta;
        //ограничиваем значения диапазоном 0-255
        if (val < 0) val = 0;
        if (val > 255) val = 255;
        result->data[i] = (uint8_t)val;
    }
    return result;
}
