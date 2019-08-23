//***************************************************************************************
//--- Includes --------------------------------------------------------------------------
//***************************************************************************************

#include "ff.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
//ffmpeg

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavcodec/avdct.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>


//***************************************************************************************
//--- Private variables -----------------------------------------------------------------
//***************************************************************************************

static AVCodec *video_codec;
static AVCodecParameters *codec_parametres;
static AVCodecContext *codec_context;
static AVFormatContext *out_format_context;
static AVFrame *frame, *temp_frame;
static AVPacket *packet;
static AVStream *output_stream;
static struct SwsContext *sws_context;

static int
        dst_width = 640,
        dst_height = 480,
        frame_lock = 0,
        stride = 0,
        translate = 0;
static int
        timer_trigger = 0;
static pthread_t
        translation_thread;
uint8_t *framebuf;

//***************************************************************************************
//--- Private function prototypes -------------------------------------------------------
//***************************************************************************************
static void *translation_loop(void*);
static void (*request_callback)(void);

//***************************************************************************************
//--- Функция инициализации ffmpeg трансляции -------------------------------------------
//***************************************************************************************
int ffmpeg_init_translation(int src_width, int src_height, int fps, int image_stride,
                     char *outfile, void(*frame_request_callback)(void)) {
    //char *outfile = "/home/vok/1.webm";
    const AVRational dst_fps = {fps, 1};
    const AVRational dst_fps_output = {15, 1};

    request_callback = frame_request_callback;
    stride = image_stride;
    timer_trigger = 1000 / fps;

    av_register_all();
    avcodec_register_all();
    avformat_network_init();
    packet = malloc(sizeof (AVPacket));
    av_init_packet(packet);

    video_codec = avcodec_find_encoder(AV_CODEC_ID_VP8);
    int _ret = avformat_alloc_output_context2(&out_format_context, NULL, NULL, outfile);
    if(_ret < 0) {
        printf("FFmpeg: Fail to avformat_alloc_output_context2(\"%s\")\n", outfile);
        avformat_free_context(out_format_context);
        return -1;
    }
    _ret = avio_open2(&out_format_context->pb, outfile, AVIO_FLAG_WRITE, NULL, NULL);
    if(_ret < 0) {
        printf("FFMpeg: Fail to avio_open2().\n");
        return -1;
    }
    output_stream = avformat_new_stream(out_format_context, video_codec);
    if(!output_stream) {
        printf("FFmpeg: Fail to avformat_new_stream()\n");
    }
    codec_parametres = output_stream->codecpar;
    codec_parametres->codec_tag = 0;
    codec_parametres->format = 0;
    codec_parametres->bit_rate = 512e3;
    codec_parametres->width = dst_width;
    codec_parametres->height = dst_height;
    codec_context = avcodec_alloc_context3(video_codec);
    codec_context = output_stream->codec;
    codec_context->width = dst_width;
    codec_context->height = dst_height;
    codec_context->bit_rate = 512e3;
    codec_context->framerate = dst_fps;
    codec_context->pix_fmt = video_codec->pix_fmts[0];
    codec_context->time_base = output_stream->time_base = av_inv_q(dst_fps_output);
    output_stream->r_frame_rate = output_stream->avg_frame_rate = dst_fps_output;
    if (out_format_context->oformat->flags & AVFMT_GLOBALHEADER) {
        codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    avcodec_parameters_to_context(codec_context, codec_parametres);
    _ret = avcodec_open2(codec_context, video_codec, NULL);
    if (_ret < 0)  {
        printf("FFmpeg: Fail to avcodec_open2()\n");
        return -1;
    }
    sws_context = sws_getContext(src_width, src_height, AV_PIX_FMT_BGR24,
                                        dst_width, dst_height, AV_PIX_FMT_YUV420P,
                                        SWS_BICUBIC, NULL, NULL, NULL);
    if (!sws_context) {
        printf("FFmpeg: Fail to sws_getCachedContext()\n");
        return -1;
    }
    frame = av_frame_alloc();
    framebuf = malloc((size_t)av_image_get_buffer_size(codec_context->pix_fmt, dst_width,
                                               dst_height,1));
    _ret = av_image_fill_arrays(frame->data, frame->linesize, framebuf,
                                codec_context->pix_fmt, dst_width, dst_height, 1);
    if (_ret < 0) {
        printf("FFmpeg: Fail to av_image_fill_arrays()\n");
        return -1;
    }
    frame->width = dst_width;
    frame->height = dst_height;
    frame->format = codec_context->pix_fmt;
    av_dump_format(out_format_context, 0, outfile, 1);
    _ret = avformat_write_header(out_format_context, NULL);
    if (_ret < 0) {
        printf("FFmpeg: Fail to avformat_write_header()");
        return -1;
    }
    sleep(1);
    request_callback();
    return 0;
}

//***************************************************************************************
//--- Функция передачи новых данных циклу трансляции ------------------------------------
//***************************************************************************************
void ffmpeg_send_frame(const uint8_t *data, int rows) {
    temp_frame = frame;
    frame_lock = 1;
    int _ret = sws_scale(sws_context, &data, &stride, 0, rows, frame->data,
                         frame->linesize);
    if (_ret < 0) {
        printf("FFmpeg: Fail to sws_scale()\n");
    }
    if (translate == 0) {
        translate = 1;
        pthread_create(&translation_thread, NULL, translation_loop, NULL);
    }
    frame_lock = 0;
    request_callback();
}

//***************************************************************************************
//--- Функция запуска системного процесса ffserver --------------------------------------
//***************************************************************************************
int ffmpeg_prepare_server() {

    char buf[512];
    FILE *cmd_pipe = popen("pidof -s ffserver", "r");
    fgets(buf, 512, cmd_pipe);
    pid_t pid = strtoul(buf, NULL, 10);
    if(pid > 0) system("killall -9 ffserver");
    bzero(buf, 512);
    //system("pwd");
    system("ffserver -f ffserver.conf &");
    cmd_pipe = popen("pidof -s ffserver", "r");
    fgets(buf, 512, cmd_pipe);
    pid = strtoul(buf, NULL, 10);
    free(cmd_pipe);
    if(pid > 0) {
        sleep(1);
        return 0;
    }
    else return -1;
}

//***************************************************************************************
//--- Функция остановки процесса ffserver -----------------------------------------------
//***************************************************************************************
void ffmpeg_stop_server() {
    translate = 0;
    pthread_join(translation_thread,NULL);
    char buf[512];
    FILE *cmd_pipe = popen("pidof -s ffserver", "r");
    fgets(buf, 512, cmd_pipe);
    pid_t pid = (pid_t)strtoul(buf, NULL, 10);
    if(pid > 0) system("killall -9 ffserver");
    free(cmd_pipe);
}

//***************************************************************************************
//--- Цикл трансляции -------------------------------------------------------------------
//***************************************************************************************
void *translation_loop(void* data) {

    int _ret = 0, _timer_delta, _delay = 20;
    long    _msec = 0,
            _nb_frames = 0;
    clock_t _before, _difference;

    while (translate == 1) {
        _before = clock();
        packet->data = NULL;
        packet->size = 0;
        if (frame_lock) {
            temp_frame->pts = _nb_frames;
            _ret = avcodec_send_frame(codec_context, temp_frame);
        } else {
            frame->pts = _nb_frames;
            _ret = avcodec_send_frame(codec_context, frame);
        }
        if (_ret < 0) {
            printf("FFmpef: fail to avcodec_send_frame()\n");
        }
        while (_ret > -1) {
            _ret = avcodec_receive_packet(codec_context, packet);
            if(_ret == AVERROR(EAGAIN) || _ret == AVERROR_EOF) {
                break;
            }
            else if (_ret < 0) {
                printf("FFmpeg: Fail to avcodec_receive_packet()");
                break;
            }
            packet->stream_index = output_stream->index;
            packet->dts = av_rescale_q(_nb_frames,
                                       codec_context->time_base,
                                       output_stream->time_base);
            packet->pts = av_rescale_q(_nb_frames,
                                                     codec_context->time_base,
                                                     output_stream->time_base);
            _ret = av_interleaved_write_frame(out_format_context, packet);
            if(_ret < 0) {
                printf("FFmpeg: Fail to av_interleaved_write_frame()\n");
            } else {
                _nb_frames ++;
            }
        }
        /*do {
          /*
           * Do something to busy the CPU just here while you drink a coffee
           * Be sure this code will not take more than `trigger` ms
           */

        _difference = clock() - _before;
        _msec = _difference * 1000 / CLOCKS_PER_SEC;
        _delay = timer_trigger - _msec + _timer_delta;
        if(_delay > 0) {
            usleep(_delay * 1000);
            _timer_delta = 0;
        } else {
            _timer_delta = _delay;
        }
        printf("mSecs: %d\n", _timer_delta);
        /*
        } while ( _msec < _delay );*/

    }
    av_write_trailer(out_format_context);
    avcodec_free_context(&codec_context);
    av_frame_free(&frame);

}
