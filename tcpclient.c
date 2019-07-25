//***************************************************************************************
//--- Includes --------------------------------------------------------------------------
//***************************************************************************************

#include "dataparser.h"
#include "tcpclient.h"

#include <netdb.h>
#include <poll.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

//***************************************************************************************
//--- Private variables -----------------------------------------------------------------
//***************************************************************************************
#define MAX_READ_BYTES 1024000
#define SA struct sockaddr

static int
        connected = 0,
        len = 0,
        socket_fd = 0;

static struct sockaddr_in
        client_address,
        server_address;
static pthread_t read_thread;
//***************************************************************************************
//--- Private function prototypes -------------------------------------------------------
//***************************************************************************************

static inline size_t get_size(uint8_t *);
static void* read_loop();
static inline void thread_printf (const char *__restrict __format, ...);

int connect_to_server(char *address, u_int16_t port,
                      void(*dataCallback)(u_int8_t, u_int8_t*, int)) {
    setup_parser(dataCallback);

    //  Создание сокета и его проверка
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd  == -1) {
        printf("Socket creation error.\n");
        return -1;
    }
    else {
        thread_printf("Socket successfully created.\n");
    }
    bzero(&server_address, sizeof (server_address));
    //  Присвоение IP, порта
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("192.168.0.1");
    server_address.sin_port = htons(20894);
    //  Связывание сокета
    if(connect(socket_fd, (SA*)&server_address, sizeof (server_address)) != 0) {
        printf("Socket not connected. Your target server an asshole.\n");
        close(socket_fd);
        return -1;
    } else {
        connected = 1;
        void* thread_data = NULL;
        pthread_create(&read_thread, NULL, read_loop, thread_data);
        printf("Connected.\n\n");
        fflush(stdout);
    }

    return 0;
}

void disconnect() {
    connected = 0;
    close(socket_fd);
    pthread_join(read_thread, NULL);
}

void *read_loop() {
    size_t
            _bytes_receive = 0,
            _expected_size = 0,
            _total_bytes_read = 0;
    struct pollfd pfd;
    uint8_t
            _cmd = 0x00,
            _receive_buffer[MAX_READ_BYTES],
            *_received_data;

    pfd.fd = socket_fd;
    pfd.events = POLLIN | POLLHUP | POLLERR | POLLNVAL | POLLPRI | POLLOUT;
    pfd.revents = 0;
    bzero(_receive_buffer, MAX_READ_BYTES);
    //  Цикл приёма передачи, пока клиент не отключится
    while(socket_fd != -1 && connected) {
        bzero(_receive_buffer, MAX_READ_BYTES);
        _bytes_receive = read(socket_fd, _receive_buffer, MAX_READ_BYTES);

        if(_bytes_receive > 0 && socket_fd != -1) {

            _total_bytes_read += _bytes_receive;
            //  Проверка на незавершённые данные
            if(_expected_size){

                //  Изменение размера массива данных и добавление данных
                _received_data = realloc(_received_data, _total_bytes_read);
                memcpy(&_received_data[_total_bytes_read - _bytes_receive],
                        &_receive_buffer, _bytes_receive);

                //  Обработка данных
                if(_total_bytes_read >= _expected_size) {
                    parse_data(_cmd, _received_data, _expected_size);
                    _received_data = realloc(_received_data, _total_bytes_read - _expected_size);
                    _total_bytes_read -= _expected_size;
                    _expected_size = 0;
                }
            }
            //  Новые данные
            else {
                //Создание массива под данные и копирование полученных данных в него
                _received_data = malloc(_bytes_receive);
                memcpy(_received_data, &_receive_buffer, _bytes_receive);
                _cmd = _received_data[0];

                if(_cmd == 0x01 || _cmd == 0x03 || _cmd == 0xFF){
                    //  Обработка данных
                    if(_total_bytes_read > 2) {
                        parse_data(_cmd, _received_data, 3);
                        _received_data = realloc(_received_data, _total_bytes_read - 3);
                        _total_bytes_read -= 3;
                    } else {
                        _expected_size = 3;
                        continue;
                    }

                } else if(_cmd == 0x04) {
                    // Ожидание пока станет возможным прочитать размер сообщения
                    if(_total_bytes_read > 4) {
                        _expected_size = get_size(&_received_data[1]) + 8;
                        if(_total_bytes_read >= _expected_size){
                            parse_data(_cmd, _received_data, _expected_size);
                            _received_data = realloc(_received_data, _total_bytes_read - _expected_size);
                            _total_bytes_read -= _expected_size;
                            _expected_size = 0;
                        }
                    } else {
                        _expected_size = 5;
                        continue;
                    }
                }
            }
        }
        //http://stefan.buettcher.org/cs/conn_closed.html
        //Проверка, что сокет не закрылся
        /*int _p = poll(&pfd, 1, 100);
        if((_p > 0) || (socket_fd == -1)) {

            thread_printf("Client disconnected\n");
            break;
        }*/
    }
    close(socket_fd);
}

//***************************************************************************************
//--- Функция печати информации из потока. ----------------------------------------------
//---------------------------------------------------------------------------------------
///-- Дополнительная функция необходима т.к. стандартная функция "printf" выводит
///-- информацию только после завершения работы потока. ---------------------------------
//***************************************************************************************

inline void thread_printf (const char *__restrict format, ...) {
    va_list arg;
    int done;
    va_start (arg, format);
    done = vfprintf (stdout, format, arg);
    va_end (arg);
    fflush(stdout); //Вывести информацию.
}

void write_to_server(u_int8_t *data, int size) {
    write(socket_fd, data, size);
}

static inline size_t get_size(uint8_t *array) {
    return  ((array[0] << 0x18) & 0xFF000000) |
            ((array[1] << 0x10) & 0x00FF0000) |
            ((array[2] << 0x08) & 0x0000FF00) |
            (array[3] & 0xFF);
}
