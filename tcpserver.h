#ifndef TCPSERVER_H
#define TCPSERVER_H

/// Include this like:
///
/// extern "C" {
/// #include "tcpserver.h"
/// }
///
/// Don't be an asshole.


#include <netdb.h>
#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

/// Создание сервера
int create_server(u_int16_t port, void(*dataCallback)(uint8_t, uint8_t*, int));

void server_write(uint8_t *data, int size);
/// Запуск сервера
int start_server();
/// Остановка сервера
void stop_server();

void test(void (*)());

#endif // TCP_SERVER_H
