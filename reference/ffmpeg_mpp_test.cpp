#include <csignal>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>
}

#define MP4_PATH "./test.mp4"
#define OUTPUT_FILENAME "./output.hevc"
#define DECODEC_NAME "h264_rkmpp"
#define ENCODEC_NAME "hevc_rkmpp"

static const AVInputFormat *input_format;
static AVStream *video_in_stream;
static int video_in_stream_idx = -1;
static const AVCodec *rk_h264_decodec;
static const AVCodec *rk_hevc_encodec;
static AVCodecContext *rk_decodec_ctx = nullptr;
static AVCodecContext *rk_encodec_ctx = nullptr;
static AVFormatContext *mp4_fmt_ctx = nullptr;
static FILE *ouput_file;
static AVFrame *frame;
static AVPacket *mp4_video_pkt;
static AVPacket *hevc_pkt;

static void encode(AVFrame *frame, AVPacket *hevc_pkt, FILE *outfile)
{
	int ret;

	if (frame)
		printf("Send frame %3" PRId64 "\n", frame->pts);

	ret = avcodec_send_frame(rk_encodec_ctx, frame);
	if (ret < 0)
	{
		char errbuf[AV_ERROR_MAX_STRING_SIZE];
		av_strerror(ret, errbuf, sizeof(errbuf));
		std::cerr << "Error sending a frame for encoding: " << errbuf << std::endl;
		exit(1);
	}

	while (ret >= 0)
	{
		ret = avcodec_receive_packet(rk_encodec_ctx, hevc_pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return;
		else if (ret < 0)
		{
			char errbuf[AV_ERROR_MAX_STRING_SIZE];
			av_strerror(ret, errbuf, sizeof(errbuf));
			std::cerr << "Error during encoding: " << errbuf << std::endl;
			exit(1);
		}

		printf("Write packet %3" PRId64 " (size=%5d)\n", hevc_pkt->pts,
			   hevc_pkt->size);

		fwrite(hevc_pkt->data, 1, hevc_pkt->size, outfile);

		av_frame_unref(frame);
		av_packet_unref(hevc_pkt);
	}
}

static void decode(AVPacket *mp4_video_pkt, AVFrame *frame)
{
	int ret;

	ret = avcodec_send_packet(rk_decodec_ctx, mp4_video_pkt);
	if (ret < 0)
	{
		char errbuf[AV_ERROR_MAX_STRING_SIZE];
		av_strerror(ret, errbuf, sizeof(errbuf));
		std::cerr << "Error sending a frame for decoding: " << errbuf << std::endl;
		exit(1);
	}

	while (ret >= 0)
	{
		ret = avcodec_receive_frame(rk_decodec_ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			return;
		}
		else if (ret < 0)
		{
			char errbuf[AV_ERROR_MAX_STRING_SIZE];
			av_strerror(ret, errbuf, sizeof(errbuf));
			std::cerr << "Error during decoding: " << errbuf << std::endl;
			exit(1);
		}

		encode(frame, hevc_pkt, ouput_file);
	}
}

int main(int argc, char **argv)
{

	int ret;

	input_format = av_find_input_format("mp4");
	if (!input_format)
	{
		std::cerr << "Could not find input format" << std::endl;
		return EXIT_FAILURE;
	}

	// 分配一个AVFormatContext。
	mp4_fmt_ctx = avformat_alloc_context();
	if (!mp4_fmt_ctx)
	{
		std::cerr << "Could not allocate format context" << std::endl;
		return EXIT_FAILURE;
	}

	// 打开输入流并读取头部信息。此时编解码器尚未开启。
	if (avformat_open_input(&mp4_fmt_ctx, MP4_PATH, input_format, nullptr) < 0)
	{
		std::cerr << "Could not open input" << std::endl;
		return EXIT_FAILURE;
	}

	// 读取媒体文件的数据包以获取流信息。
	if (avformat_find_stream_info(mp4_fmt_ctx, nullptr) < 0)
	{
		std::cerr << "Could not find stream info" << std::endl;
		return EXIT_FAILURE;
	}

	// 打印视频信息
	av_dump_format(mp4_fmt_ctx, 0, MP4_PATH, 0);

	// 查找视频流
	if ((ret = av_find_best_stream(mp4_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1,
								   nullptr, 0)) < 0)
	{
		std::cerr << "Could not find video stream" << std::endl;
		return EXIT_FAILURE;
	}

	video_in_stream_idx = ret;
	video_in_stream = mp4_fmt_ctx->streams[video_in_stream_idx];

	std::cout << "video_in_stream->duration: " << video_in_stream->duration
			  << std::endl;

	const char *filename = OUTPUT_FILENAME;
	int i = 0;

	// 查找解码器
	rk_h264_decodec = avcodec_find_decoder_by_name(DECODEC_NAME);
	if (!rk_h264_decodec)
	{
		std::cerr << "Codec '" << DECODEC_NAME << "' not found" << std::endl;
		exit(1);
	}

	rk_decodec_ctx = avcodec_alloc_context3(rk_h264_decodec);
	if (!rk_decodec_ctx)
	{
		std::cerr << "Could not allocate video rk_h264_decodec context"
				  << std::endl;
		exit(1);
	}

	// 将视频参数复制到rk_h264_decodec上下文中。
	if (avcodec_parameters_to_context(rk_decodec_ctx, video_in_stream->codecpar) <
		0)
	{
		std::cerr << "Could not copy video parameters to rk_h264_decodec context"
				  << std::endl;
		exit(1);
	}

	AVDictionary *opts = NULL;
	av_dict_set_int(&opts, "buf_mode", 1, 0);

	if (avcodec_open2(rk_decodec_ctx, rk_h264_decodec, &opts) < 0)
	{
		std::cerr << "Could not open rk_h264_decodec" << std::endl;
		exit(1);
	}

	// 查找编码器
	rk_hevc_encodec = avcodec_find_encoder_by_name(ENCODEC_NAME);
	if (!rk_hevc_encodec)
	{
		std::cerr << "Codec '" << ENCODEC_NAME << "' not found" << std::endl;
		exit(1);
	}

	rk_encodec_ctx = avcodec_alloc_context3(rk_hevc_encodec);

	if (!rk_encodec_ctx)
	{
		std::cerr << "Could not allocate video rk_hevc_encodec context"
				  << std::endl;
		exit(1);
	}

	// 设置编码器参数
	rk_encodec_ctx->width = video_in_stream->codecpar->width;
	rk_encodec_ctx->height = video_in_stream->codecpar->height;
	rk_encodec_ctx->pix_fmt = AV_PIX_FMT_NV12;
	rk_encodec_ctx->time_base = video_in_stream->time_base;
	rk_encodec_ctx->framerate = video_in_stream->r_frame_rate;
	rk_encodec_ctx->gop_size = 50;
	rk_encodec_ctx->bit_rate = 1024 * 1024 * 10;

	av_opt_set(rk_encodec_ctx->priv_data, "profile", "main", 0);
	av_opt_set(rk_encodec_ctx->priv_data, "qp_init", "23", 0);
	av_opt_set_int(rk_encodec_ctx->priv_data, "rc_mode", 0, 0);

	ret = avcodec_open2(rk_encodec_ctx, rk_hevc_encodec, nullptr);
	if (ret < 0)
	{
		std::cerr << "Could not open rk_hevc_encodec: " << std::endl;
		exit(1);
	}

	mp4_video_pkt = av_packet_alloc();
	if (!mp4_video_pkt)
		exit(1);

	hevc_pkt = av_packet_alloc();
	if (!hevc_pkt)
		exit(1);

	ouput_file = fopen(filename, "wb");
	if (!ouput_file)
	{
		std::cerr << "Could not open " << filename << std::endl;
		exit(1);
	}

	frame = av_frame_alloc();
	if (!frame)
	{
		std::cerr << "Could not allocate video frame" << std::endl;
		exit(1);
	}

	while (true)
	{
		ret = av_read_frame(mp4_fmt_ctx, mp4_video_pkt);
		if (ret < 0)
		{
			std::cerr << "Could not read frame" << std::endl;
			break;
		}

		if (mp4_video_pkt->stream_index == video_in_stream_idx)
		{
			std::cout << "mp4_video_pkt->pts: " << mp4_video_pkt->pts << std::endl;

			decode(mp4_video_pkt, frame);
		}
		av_packet_unref(mp4_video_pkt);
		i++;
	}

	// 确保将所有帧写入
	av_packet_unref(mp4_video_pkt);
	decode(mp4_video_pkt, frame);
	encode(nullptr, mp4_video_pkt, ouput_file);

	fclose(ouput_file);

	avcodec_free_context(&rk_encodec_ctx);
	avformat_close_input(&mp4_fmt_ctx);
	avformat_free_context(mp4_fmt_ctx);
	av_frame_free(&frame);
	av_packet_free(&mp4_video_pkt);
	av_packet_free(&hevc_pkt);

	return 0;
}
