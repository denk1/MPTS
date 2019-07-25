//***************************************************************************************
//--- Includes --------------------------------------------------------------------------
//***************************************************************************************

#include "dataparser.h"
#include "tcpserver.h"

#include <arpa/inet.h>
#include <poll.h>
#include <pthread.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/socket.h>

//***************************************************************************************
//--- Private variables -----------------------------------------------------------------
//***************************************************************************************

#define MAX_READ_BYTES 1024000
#define SA struct sockaddr

static int
        len = 0,
        listen_flag = 0,
        socket_fd = 0,
        client_fd;

static struct sockaddr_in
        client_address,
        server_address;

static pthread_t listen_thread;

//***************************************************************************************
//--- Private function prototypes -------------------------------------------------------
//***************************************************************************************

static inline size_t get_size(uint8_t *);
static void* read_loop();
static inline void thread_printf (const char *__restrict __format, ...);

//***************************************************************************************
//--- Функция создания сервера. ---------------------------------------------------------
//---------------------------------------------------------------------------------------
///-- Возвращает "0", если сервер создан успешно. В случае ошибки возвращает "-1" и
///-- и выводит онформацию об ошибке в консоль. -----------------------------------------
//***************************************************************************************

int create_server(u_int16_t port, void(*func_for_callback)(uint8_t, uint8_t*, int)) {



    printf("Server initialization.\n");
    fflush(stdout);

    setup_parser(func_for_callback);
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
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port);
    //  Связывание сокета
    if(bind(socket_fd, (SA*)&server_address, sizeof (server_address)) != 0) {
        printf("Socket bind like FUCK. Sorry...\n");
        close(socket_fd);
        return -1;
    } else {
        printf("Socket bind OK.\n\n");
        fflush(stdout);
    }
    //  Начатие прослушивания
    if((listen(socket_fd, 5)) != 0) {
        printf("Socket listen as asshole. Try again later.Tomorrow for example.\n");
        return -1;
    } else {
        thread_printf("Server is listenning.\nPort: %d\n\n", port);
    }
    len = sizeof(client_address);
    return 0;
}

//***************************************************************************************
//--- Функция запуска сервера. ----------------------------------------------------------
//***************************************************************************************

int start_server() {
    void* thread_data = NULL;
    listen_flag = 1;
    pthread_create(&listen_thread, NULL, read_loop, thread_data);
    return 0;
}

//***************************************************************************************
//-- Фунцкия остановки сервера. ---------------------------------------------------------
//****************************4096***********************************************************

void stop_server() {
    listen_flag = 0;
    shutdown(socket_fd, SHUT_RD);
    close(socket_fd);
    pthread_join(listen_thread, NULL);
    printf("Server stopped\n");
}

//***************************************************************************************
//--- Функция прослушивания и приёма. ---------------------------------------------------
//---------------------------------------------------------------------------------------
///-- Цикл одновременно может работать только с одним клиентом. -------------------------
//***************************************************************************************

void* read_loop() {

    thread_printf("Listen thread start\n\n");

    char
            _client_address[INET_ADDRSTRLEN + 1];
    size_t
            _bytes_receive = 0,
            _expected_size = 0,
            _total_bytes_read = 0;
    struct pollfd pfd;
    uint8_t
            _cmd = 0x00,
            _receive_buffer[MAX_READ_BYTES],
            *_received_data;

    while (listen_flag) {

        //  Ожидание подключения клиента.
        client_fd = accept(socket_fd, (SA*)&client_address, &len);
        if (client_fd == -1 && listen_flag == 1) {
            thread_printf("Sad, but true. Server acccept give a fuck.\n");
        }
        else if(client_fd != -1 && listen_flag == 1){
            inet_ntop(AF_INET, &client_address.sin_addr, _client_address,
                      INET_ADDRSTRLEN);
            _client_address[INET_ADDRSTRLEN] = 0x00;
            thread_printf("Server acccept the client.\n");
            thread_printf("Client IPv4 address: %s\n\n", _client_address);
        }
        else {
            break;
        }
        pfd.fd = client_fd;
        pfd.events = POLLIN | POLLHUP | POLLERR | POLLNVAL | POLLPRI | POLLOUT;
        pfd.revents = 0;
        bzero(_receive_buffer, MAX_READ_BYTES);
        //  Цикл приёма передачи, пока клиент не отключится
        while(client_fd != -1 && listen_flag) {
            bzero(_receive_buffer, MAX_READ_BYTES);
            _bytes_receive = read(client_fd, _receive_buffer, MAX_READ_BYTES);

            if(_bytes_receive > 0 && client_fd != -1) {

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
                        memcpy(_received_data, &_received_data[_expected_size], _total_bytes_read);
                        bzero(&_received_data[_total_bytes_read], _expected_size);
                        _received_data = realloc(_received_data, _total_bytes_read);
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
                            _total_bytes_read -= 3;
                            memcpy(_received_data, &_received_data[3], _total_bytes_read);
                            bzero(&_received_data[_total_bytes_read], 3);
                            _received_data = realloc(_received_data, _total_bytes_read);
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
                                _total_bytes_read -= _expected_size;

                                memcpy(_received_data, &_received_data[_expected_size], _total_bytes_read);
                                bzero(&_received_data[_total_bytes_read], _expected_size);
                                _received_data = realloc(_received_data, _total_bytes_read);
                                _expected_size = 0;
                            }
                        } else {
                            _expected_size = 5;
                            continue;
                        }
                    }
                }
            }
            else if (_bytes_receive == 0) {
                printf("0 bytes\n");
                break;
            }
            //http://stefan.buettcher.org/cs/conn_closed.html
            //Проверка, что сокет не закрылся
            /*int _p = poll(&pfd, 1, 100);
            if((_p > 0) || (client_fd == -1)) {

                thread_printf("Client disconnected\n");
                break;
            }*/
        }
        close(client_fd);
    }

    return 0;
}


void server_write(uint8_t *data, int size) {
    write(client_fd, data, size);
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

//***************************************************************************************
//--- Функция подсчёта размера информационных данных. -----------------------------------
//***************************************************************************************

static inline size_t get_size(uint8_t *array) {
    return  ((array[0] << 0x18) & 0xFF000000) |
            ((array[1] << 0x10) & 0x00FF0000) |
            ((array[2] << 0x08) & 0x0000FF00) |
            (array[3] & 0xFF);
}

void test(void (*func)()) {
    func();
}
