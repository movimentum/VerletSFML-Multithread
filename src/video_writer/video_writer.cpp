#include "video_writer.hpp"

/*
 * Constructor
*/
VideoWriter::VideoWriter(const uint32_t _w, const uint32_t _h, const char* codecName, const int _bitrate, const int _framerate) : width(_w), height(_h) {

	// ������� ������� �� �����
	pCodec = avcodec_find_encoder_by_name(codecName);
	if (!pCodec) {
		std::cerr << "Codec '" << codecName << "' not found" << std::endl;
		exit(1);
	}

	// �������� ����� ��� �������� ������
	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (!pCodecCtx) {
		std::cerr << "Failed to allocate video codec context." << std::endl;
		exit(1);
	}
	if (pCodec->id == AV_CODEC_ID_H264)
		av_opt_set(pCodecCtx->priv_data, "preset", "slow", 0);

	// ������� �����, ������� ����� ��������� �������������� ������ �����
	pPacket = av_packet_alloc();
	if (!pPacket) {
		std::cerr << "Failed to allocate packet." << std::endl;
		exit(1);
	}

	// ����� ��������� ��������� ��� �����������
	pCodecCtx->bit_rate = _bitrate;
	pCodecCtx->width = width;   // ������ ���� ������ 2
	pCodecCtx->height = height; //
	pCodecCtx->time_base = { 1, _framerate };
	pCodecCtx->framerate = { _framerate, 1 };
	pCodecCtx->gop_size = 10;
	pCodecCtx->max_b_frames = 1;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	// ��������� �����
	if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0) {
		std::cerr << "Could not open codec '" << pCodec->long_name << "'" << std::endl;
		exit(1);
	}

	// ������� ����, ���������� ���������������� ������
	pFrame = av_frame_alloc();
	pFrameRGBA = av_frame_alloc();
	if (!pFrame || !pFrameRGBA) {
		std::cerr << "Could not allocate video frame" << std::endl;
		exit(1);
	}
	pFrame->format = pCodecCtx->pix_fmt;
	pFrame->width = pCodecCtx->width;
	pFrame->height = pCodecCtx->height;
	pFrameRGBA->format = AV_PIX_FMT_RGBA; // ����� �������� sf::Image ����� ������ RGBA
	pFrameRGBA->width = pCodecCtx->width;
	pFrameRGBA->height = pCodecCtx->height;

	if (av_frame_get_buffer(pFrame, 32) < 0) {
		std::cerr << "Could not allocate video frame data" << std::endl;
		exit(1);
	}

	// �������� ��� ����������� ��������
	swsCtx = sws_getContext(
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGBA,
		pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		SWS_BILINEAR, nullptr, nullptr, nullptr
	);
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

	avcodec_free_context(&pCodecCtx);
	av_frame_free(&pFrame);
	av_packet_free(&pPacket);
}

/*
 * ���������� ����� � ������
 * ����������� rgba �� sf::Image � yuv420 ��� ������
 */
int VideoWriter::prepare_frame(const uint8_t *pRGBA) {

	// ��������� ������������� ���� � �������� �������
	// https://stackoverflow.com/questions/35678041/what-is-linesize-alignment-meaning
	int ret = av_image_fill_arrays(pFrameRGBA->data, pFrameRGBA->linesize, pRGBA, AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height, 32);
	if (ret < 0) {
		std::cerr << "ERROR: Failed to connect FrameRGBA with data." << std::endl;
		return -1;
	}

	/*
	std::cout
		<< "linesize " << pFrameRGBA->linesize[0] << " " << pFrameRGBA->linesize[1] << std::endl
		<< "width " << pFrameRGBA->width << std::endl
		<< "height " << pFrameRGBA->height << std::endl;
	*/

	// ��������� �������� ����
	int outHeight = sws_scale(swsCtx, pFrameRGBA->data, pFrameRGBA->linesize, 0, pFrame->height, pFrame->data, pFrame->linesize);

	return 0;
}

/*
* ���������� ������� ���� � ����
*/
int VideoWriter::send_frame_to_file() {

	// ��������� ���������� ����� � ������
	if (av_frame_make_writable(pFrame) < 0) {
		std::cerr << "ERROR: Frame cannot be made writable." << std::endl;
		return -1;
	}

	// �������� ���� �� �����������
	if (avcodec_send_frame(pCodecCtx, pFrame) < 0) {
		std::cerr << "ERROR sending a frame to encoder." << std::endl;
		return -2;
	}
	
	// ����������, ���� ���-�� �������
	write_buffered_if_ready();

	if (nFramesWritten >= reportEveryNFrames)
		report_written_frames();

	return 0;
}

int VideoWriter::flush() {

	if (avcodec_send_frame(pCodecCtx, nullptr) < 0) {
		std::cerr << "ERROR when trying to flush." << std::endl;
		return -1;
	}

	write_buffered_if_ready();

	report_written_frames("final flush");
		

	/* Add sequence end code to have a real MPEG file.
	   It makes only sense because this tiny examples writes packets
	   directly. This is called "elementary stream" and only works for some
	   codecs. To create a valid file, you usually need to write packets
	   into a proper file format or protocol; see mux.c.
	 */
	uint8_t endcode[] = { 0, 0, 1, 0xb7 };
	if (pCodec->id == AV_CODEC_ID_MPEG1VIDEO || pCodec->id == AV_CODEC_ID_MPEG2VIDEO)
		fwrite(endcode, 1, sizeof(endcode), file);

	return 0;
}

int VideoWriter::write_buffered_if_ready()
{
	int ret = 0;

	while (ret >= 0) {
		ret = avcodec_receive_packet(pCodecCtx, pPacket);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return 0;
		else if (ret < 0) {
			std::cerr << "ERROR during encoding." << std::endl;
			return ret; // Should exit on error
		}

		fwrite(pPacket->data, 1, pPacket->size, file);
		av_packet_unref(pPacket);
		++nFramesWritten;
	}

	// Should never reach this point
	// return 0;
}


void VideoWriter::report_written_frames(std::string explanation) {
	nFramesTotal += nFramesWritten;

	std::cout << nFramesWritten << " more frames written to file" << explanation.c_str();
	if (!explanation.empty())
		std::cout << " (" << explanation.c_str() << ")";
	std::cout << "." << std::endl;

	nFramesWritten = 0;
}