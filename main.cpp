#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <opencv2/opencv.hpp>
#include <strings.h>



extern "C" {
#include "tcpserver.h"
#include "tcpclient.h"
#include "ff.h"
}

static int send_flag = 0, encode_flag = 0;
static char _type;

void parsed_data(uint8_t cmd, uint8_t *data, int data_size) {
    if(cmd == 3) {
        send_flag = true;
    }
    else if(cmd == 4) {
        //std::vector<char> *buf = new std::vector<char>(data_size) ;
        //memcpy(buf->data(), data, data_size);
        //cv::Mat _mat = cv::imdecode(*buf, 1);
        cv::Mat _mat(480,640, CV_8UC3);
        //memcpy(_mat.data, data, data_size);
        /*if(_mat.rows) {
            cv::imshow("PNG", _mat);
            cv::waitKey(20);
        }*/
        //_mat.deallocate();
        uint8_t data[3];
        data[0] = 0x03;
        data[1] = 0x7F;
        data[2] = 0x80;
        cv::String str;

        if(_type == 's')
            server_write(data, 3);
        else if(_type == 'c')
            write_to_server(data, 3);
    }

    //cv::Mat mat;
    //cv::imdecode()
}
void ffmpeg_request() {
    encode_flag = 1;
}

int main(int argc, char *argv[])
{



    int _ch = 0, _size;
    printf("*** Input 's' and press enter for server operations, 'c' for client. ***\n\n");
    _type = getchar();
    if(_type == 's') {
        std::vector<uchar> buff, buff_pack;
        cv::Mat _mat;
        cv::VideoCapture _capture;
        int a;
        //int a = _capture.open("/home/vok/Видео/test.mp4");
        if(!a) a = _capture.open(0);
        if(!a)
            return -1;
        cv::waitKey(1000);
        bool b = _capture.read(_mat);
        cv::waitKey(1000);
        _size = _mat.cols * _mat.rows * _mat.elemSize1() * _mat.channels();

        ffmpeg_prepare_server();
        ffmpeg_init_translation(_mat.cols, _mat.rows, _capture.get(CV_CAP_PROP_FPS),
                (int)_mat.step[0], "http://localhost:8090/feed_detection.ffm",
                &ffmpeg_request);

        u_int8_t _data[_size + 8];
        _data[0] = 0x04;
        _data[1] = ((_size >> 0x18) & 0xFF);
        _data[2] = ((_size >> 0x10) & 0xFF);
        _data[3] = ((_size >> 0x08) & 0xFF);
        _data[4] = (_size & 0xFF);
        _data[_size + 6] = 0x7F;
        _data[_size + 7] = 0x80;


        if(create_server(20894, &parsed_data) < 0)
             return -1;
         start_server();

         printf("*** Input 'q' and press enter to exit. ***\n\n");

         while (1) {
            _capture.read(_mat);
            imencode(".jpg", _mat, buff);
            buff_pack.resize(5);
            memcpy(&buff_pack[0], &_data[0], 5);
            buff_pack.insert(buff_pack.begin() + 5, buff.begin(), buff.end());
            size_t size_buff_pack = buff_pack.size() - 5;
            buff_pack[1] = ((size_buff_pack >> 0x18) & 0xFF);
            buff_pack[2] = ((size_buff_pack >> 0x10) & 0xFF);
            buff_pack[3] = ((size_buff_pack >> 0x08) & 0xFF);
            buff_pack[4] = (size_buff_pack & 0xFF);
            //buff_pack.push_back(0x7F);
            //buff_pack.push_back(0x80);
            //if(_mat.rows)
            //    cv::imshow("ORIGINAL", _mat);
            memcpy(&_data[5], _mat.data, (size_t)_size);

            for(int _i = 0; _i < _size; _i++)
                _data[_size + 5] ^= _data[_i + 5];
            if(send_flag) {
                //server_write(_data, _size + 8);
                server_write(&buff_pack[0], buff_pack.size());
                send_flag = 0;
            }
            if(encode_flag && _mat.rows) {

                ffmpeg_send_frame(_mat.data, _mat.rows);
            }
            _ch = cv::waitKey(33);
            if(_ch == 'q') {
                printf("Disconnecting.\n");
                break;
            }
            buff_pack.erase(buff_pack.begin(), buff_pack.end());
            //usleep(33333);
             /*_ch = getchar();
             if(_ch == 'q') {
                 printf("Disconnecting.\n");
                 break;
             }
             usleep(50000);*/
        }
        _capture.release();
        ffmpeg_stop_server();
        //stop_server();

    } else if(_type == 'c') {

        printf("*** Input 'q' and press enter to exit. ***\n\n");
        if(connect_to_server("192.168.0.1", 20894, &parsed_data) == -1)
            return -1;
        u_int8_t data[3];
        data[0] = 0x03;
        data[1] = 0x7F;
        data[2] = 0x80;
        write_to_server(data, 3);
        while (1) {
            _ch = getchar();
            if(_ch == 'q') {
                printf("Disconnecting.\n");
                break;
            }
            usleep(50000);
        }

        disconnect();
    }

    return 0;

}


