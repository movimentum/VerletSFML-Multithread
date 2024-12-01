#include "video_writer.hpp"

/*
 * Constructor
*/
VideoWriter::VideoWriter(const uint32_t _w, const uint32_t _h, const char* codecName) : width(_w), height(_h) {

	// Находим энкодер по имени
	pCodec = avcodec_find_encoder_by_name(codecName);
	if (!pCodec) {
		std::cerr << "Codec '" << codecName << "' not found" << std::endl;
		exit(1);
	}

	// Выделяем место под контекст кодека
	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (!pCodecCtx) {
		std::cerr << "Failed to allocate video codec context." << std::endl;
		exit(1);
	}
	if (pCodec->id == AV_CODEC_ID_H264)
		av_opt_set(pCodecCtx->priv_data, "preset", "slow", 0);

	// Заводим пакет, который будет содержать закодированные данные кадра
	pPacket = av_packet_alloc();
	if (!pPacket) {
		std::cerr << "Failed to allocate packet." << std::endl;
		exit(1);
	}

	// Задаём некоторые параметры для кодирования
	pCodecCtx->bit_rate = 400000;
	pCodecCtx->width = width;   // должно быть кратно 2
	pCodecCtx->height = height; //
	pCodecCtx->time_base = { 1, 25 };
	pCodecCtx->framerate = { 25, 1 };
	pCodecCtx->gop_size = 10;
	pCodecCtx->max_b_frames = 1;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	// Открываем кодек
	if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0) {
		std::cerr << "Could not open codec '" << pCodec->long_name << "'" << std::endl;
		exit(1);
	}
}


/*
 * Open/close files
*/
void VideoWriter::open_file(const char * _fn) {
	if (file) {
		if (filename == _fn) {
			std::cerr << "File '" << filename << "' is already opened. Nothing to do." << std::endl;
			return;
		}
		close_file();
		std::cerr << "Previously opend file '" << filename << "' has been closed." << std::endl;
	}

	filename = _fn;
	file = fopen(filename, "wb");
	if (!file) {
		std::cerr << "Could not open file '" << filename << "' for writing." << std::endl;
		exit(1);
	}
}

void VideoWriter::close_file() {
	if (!file) return;
	fclose(file);
	file = nullptr;
}