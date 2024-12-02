// Структура, позволяющая создавать видео из картинок с помощью ffmpeg
#pragma once

#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}


struct VideoWriter {
	
	const uint32_t width, height;
	const AVCodec *pCodec = nullptr;
	AVCodecContext *pCodecCtx = nullptr;
	AVPacket *pPacket = nullptr;
	AVFrame *pFrame = nullptr;
	AVFrame *pFrameRGBA = nullptr;
	SwsContext *swsCtx = nullptr;
	FILE *file = nullptr;
	const char *filename = nullptr;

	VideoWriter(const uint32_t _w, const uint32_t _h, const char* codecName);

	~VideoWriter() {
		close_file();
		avcodec_free_context(&pCodecCtx);
		av_frame_free(&pFrame);
		av_packet_free(&pPacket);
	}

	void open_file(const char * _fn);

	void close_file();

	// Готовит кадр к записи
	// data -- массив пикселей 8bit-RGBA размером width * height * 4
	int prepare_frame(const uint8_t *data);
	
	// Записывает готовый кадр в файл
	int write_frame_to_file();

	// Освобождает буффер при окончании записи
	int flush();
};