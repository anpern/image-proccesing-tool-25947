CC = gcc
CFLAGS = -Wall -O2 //Флаг компиляции
LDFLAGS = -lm //Флаг линковщика
TARGET = imgproc
SRCS = main.c image_processing.c
OBJS = $(SRCS:.c=.o) //Объектные файлы замена .c на .o

all: $(TARGET) //запускает сборку

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) //линкует объектные файлы в исполняемый imgproc

%.o: %.c image_processing.h
	$(CC) $(CFLAGS) -c $< -o $@ //компилирует .c в .o, зависит от image_processing.h

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean