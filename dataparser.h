#ifndef DATAPARSER_H
#define DATAPARSER_H

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>

void setup_parser(void(*func_for_callback)(__uint8_t, __uint8_t*, int));
void parse_data(__uint8_t cmd, __uint8_t *data, size_t size);

#endif // DATAPARSER_H
