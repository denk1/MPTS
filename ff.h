#ifndef FF_H
#define FF_H

#include <stdint.h>

//Подготовка трансляции
int ffmpeg_init_translation(int srcWidth, int srcHeight, int fps, int stride, char *outfile,
                     void(*frame_request_callback)(void));
//Запуск сервера
int ffmpeg_prepare_server();
//Отправка изображения
void ffmpeg_send_frame(const uint8_t *data, int rows);
//Остановка сервера
void ffmpeg_stop_server();

#endif // FF_H
