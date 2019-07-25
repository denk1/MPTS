#ifndef TCPCLIENT_H
#define TCPCLIENT_H


#include <sys/types.h>


int connect_to_server(char *address, u_int16_t port, void(*dataCallback)(u_int8_t cmd, u_int8_t *data, int size));
void disconnect();
void write_to_server(u_int8_t *data, int size);

#endif // TCPCLIENT_H
