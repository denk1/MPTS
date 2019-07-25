//***************************************************************************************
//--- Includes --------------------------------------------------------------------------
//***************************************************************************************

#include "dataparser.h"

#include <string.h>
#include <strings.h>

//***************************************************************************************
//--- Private variables -----------------------------------------------------------------
//***************************************************************************************
static int packets_count = 0;
static __uint8_t
        *data_buffer,
        cmd,
        *parsed_data;

//***************************************************************************************
//--- Private function prototypes -------------------------------------------------------
//***************************************************************************************

static void (*callback)(__uint8_t, __uint8_t *, int);

void setup_parser(void(*func_for_callback)(__uint8_t, __uint8_t *, int)) {
    callback = func_for_callback;
    data_buffer = malloc(100000);
    parsed_data = malloc(100000);
}

void parse_data(__uint8_t cmd, __uint8_t *data, size_t data_size) {

    data_buffer = realloc(data, data_size);

    __uint8_t _crc = 0x00;
    printf("Packets count: %d\n", ++packets_count);
    printf("Data size: %d\n\n", data_size);
    // Проверка конечных байт
    if(data_buffer[data_size - 2] != 0x7F && data_buffer[data_size - 1] != 0x80) {
        printf("Bad tail %d %d\n\n", data_buffer[data_size - 2], data_buffer[data_size - 1]);
    }

    if(cmd == 0x01 || cmd == 0x03 || cmd == 0xFF) {
        callback(cmd, NULL, 0);
    }
    else if(cmd == 0x04) {
        /*for(size_t _i = 5; _i < data_size - 3; _i++) {
            _crc = _crc ^ data_buffer[_i];
        }*/

        //if(_crc == data_buffer[data_size - 3]) {
            free(parsed_data);
            parsed_data = malloc(data_size - 8);
            memcpy(parsed_data, &data_buffer[5], data_size - 8);
            callback(cmd, parsed_data, data_size - 8);

        /*} else {
            printf("Bad crc %d %d\n\n", data_buffer[data_size - 3], _crc);
        }*/
    }
}
