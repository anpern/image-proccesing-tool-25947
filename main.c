#include "image_processing.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void print_usage(const char* prog){
    printf("\n=== Image Processing Tool ===\n");
    printf("Usage:\n");
    printf("  %s <input> -median <size> <output>\n", prog);
    printf("  %s <input> -gauss <size> <sigma> <output>\n", prog);
    printf("  %s <input> -edges [threshold] <output>\n", prog);
    printf("  %s <input> -bright <delta> <output>\n\n", prog);
    printf("Examples:\n");
    printf("  %s photo.png -median 3 result.png\n", prog);
    printf("  %s photo.jpg -gauss 5 1.5 result.jpg\n", prog);
    printf("  %s photo.bmp -edges 50 result.bmp\n", prog);
    printf("  %s photo.png -bright 30 result.png\n\n", prog);
}

int main(int argc, char* argv[]){
    if (argc < 4){//проверка кол-ва аргументов
        print_usage(argv[0]);
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* operation = argv[2];
    
    printf("\n[1/4] Загрузка: %s\n", input_file);
    Image* img = load_image(input_file);
    if (!img){
        fprintf(stderr, "ОШИБКА: не удалось загрузить %s\n", input_file);
        return 1;
    }
    
    Image* result = NULL;
    const char* output_file = NULL;
    
    printf("[2/4] Операция: %s\n", operation);
    //выбор фильтра
    if (strcmp(operation, "-median") == 0 && argc >= 5){
        int size = atoi(argv[3]);
        output_file = argv[4];
        printf("      Медианный фильтр (окно %dx%d)\n", size, size);
        result = median_filter(img, size);
    }
    else if (strcmp(operation, "-gauss") == 0 && argc >= 6){
        int size = atoi(argv[3]);
        float sigma = atof(argv[4]);
        output_file = argv[5];
        printf("      Гауссов фильтр (ядро %dx%d, sigma=%.2f)\n", size, size, sigma);
        result = gaussian_filter(img, size, sigma);
    }
    else if (strcmp(operation, "-edges") == 0){
        int threshold = 0;
        if (argc >= 5){
            threshold = atoi(argv[3]);
            output_file = argv[4];
        }
        else{
            output_file = argv[3];
        }
        printf("      Детекция границ (порог=%d)\n", threshold);
        result = detect_edges(img, threshold);
    }
    else if (strcmp(operation, "-bright") == 0 && argc >= 5){
        int delta = atoi(argv[3]);
        output_file = argv[4];
        printf("      Изменение яркости (delta=%d)\n", delta);
        result = adjust_brightness(img, delta);
    }
    else{
        fprintf(stderr, "ОШИБКА: неизвестная операция или неверные аргументы\n");
        print_usage(argv[0]);
        free_image(img);
        return 1;
    }
    
    if (!result){
        fprintf(stderr, "ОШИБКА: не удалось выполнить операцию\n");
        free_image(img);
        return 1;
    }
    //сохранение результата
    printf("[3/4] Сохранение: %s\n", output_file);
    if (!save_image(result, output_file)){
        fprintf(stderr, "ОШИБКА: не удалось сохранить %s\n", output_file);
        free_image(img);
        free_image(result);
        return 1;
    }
    
    printf("[4/4] ГОТОВО!\n\n");
    
    free_image(img);
    free_image(result);
    return 0;
}
