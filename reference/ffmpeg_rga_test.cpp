#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>
}

#define ENCODER_NAME "hevc_rkmpp"

const char *filter_descr =
	"hwupload,scale_rkrga=w=640:h=360:format=nv12,hwdownload";

static AVFormatContext *fmt_ctx;
static AVCodecContext *decodec_ctx;
static AVCodecContext *encodec_ctx;
static int video_stream_index = -1;
static AVStream *video_stream;
static AVFilterContext *buffersink_ctx;
static AVFilterContext *buffersrc_ctx;
static AVFilterContext *hwupload_ctx;
static AVFilterGraph *filter_graph;
static const AVCodec *decodec;
static const AVCodec *encodec;

static int open_input_file(const char *filename)
{
	int ret;

	if ((ret = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
		return ret;
	}

	if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
		return ret;
	}

	ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decodec, 0);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR,
			   "Cannot find a video stream in the input file\n");
		return ret;
	}

	video_stream_index = ret;
	video_stream = fmt_ctx->streams[video_stream_index];

	decodec_ctx = avcodec_alloc_context3(decodec);
	if (!decodec_ctx)
		return AVERROR(ENOMEM);
	avcodec_parameters_to_context(decodec_ctx, video_stream->codecpar);

	if ((ret = avcodec_open2(decodec_ctx, decodec, NULL)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n");
		return ret;
	}

	return 0;
}

static int init_filters(const char *filters_descr)
{
	char args[512];
	int ret = 0;
	const AVFilter *buffersrc = avfilter_get_by_name("buffer");
	const AVFilter *buffersink = avfilter_get_by_name("buffersink");

	AVFilterInOut *outputs = avfilter_inout_alloc();
	AVFilterInOut *inputs = avfilter_inout_alloc();
	AVRational time_base = video_stream->time_base;
	enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_NV12, AV_PIX_FMT_NONE};

	if (!buffersrc)
	{
		std::cout << "buffersrc not found" << std::endl;
		return -1;
	}

	if (!buffersink)
	{
		std::cout << "buffersink not found" << std::endl;
		return -1;
	}

	// 创建 rkmpp 硬件上下文
	AVBufferRef *hw_device_ctx;
	if (av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_RKMPP, "rkmpp",
							   NULL, 0) < 0)
	{
		std::cout << "Failed to create hardware frames context" << std::endl;
		return -1;
	}

	filter_graph = avfilter_graph_alloc();
	if (!outputs || !inputs || !filter_graph)
	{
		ret = AVERROR(ENOMEM);
		goto end;
	}

	// 给 buffer 滤镜设置参数
	snprintf(args, sizeof(args),
			 "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
			 decodec_ctx->width, decodec_ctx->height, decodec_ctx->pix_fmt,
			 time_base.num, time_base.den, decodec_ctx->sample_aspect_ratio.num,
			 decodec_ctx->sample_aspect_ratio.den);

	std::cout << "args: " << args << std::endl;

	ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args,
									   NULL, filter_graph);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
		goto end;
	}

	ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL,
									   NULL, filter_graph);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
		goto end;
	}

	// 设置 buffersink 滤镜要输出的格式
	ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
							  AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
		goto end;
	}

	outputs->name = av_strdup("in");
	outputs->filter_ctx = buffersrc_ctx;
	outputs->pad_idx = 0;
	outputs->next = NULL;

	inputs->name = av_strdup("out");
	inputs->filter_ctx = buffersink_ctx;
	inputs->pad_idx = 0;
	inputs->next = NULL;

	// 用于根据给定的字符串描述解析并构建一个滤镜图（filter graph）。
	// inputs 指向输入滤镜链表的指针。
	// outputs 指向输出滤镜链表的指针。
	// 优势就是它简化了滤镜图的构建过程。通过传入一个描述滤镜图的字符串，你不需要手动去分配每个滤镜并手动连接它们。这意味着你可以通过字符串形式的描述快速构建复杂的滤镜图。
	if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr, &inputs,
										&outputs, NULL)) < 0)
	{
		std::cout << "avfilter_graph_parse_ptr failed" << std::endl;
		goto end;
	}

	std::cout << "avfilter_graph_parse_ptr success" << std::endl;

	avfilter_graph_set_auto_convert(filter_graph, AVFILTER_AUTO_CONVERT_ALL);

	hwupload_ctx = avfilter_graph_get_filter(filter_graph, "Parsed_hwupload_0");
	if (hwupload_ctx)
	{
		std::cout << "filter name: " << hwupload_ctx->name
				  << ", type: " << hwupload_ctx->filter->name << std::endl;
		hwupload_ctx->hw_device_ctx = hw_device_ctx;
	}

	// 打印滤镜图有多少个滤镜
	std::cout << "nb_filters: " << filter_graph->nb_filters << std::endl;

	// 打印滤镜图
	std::cout << avfilter_graph_dump(filter_graph, NULL) << std::endl;

	// 用于检查和配置滤镜图的有效性及其连接和格式的函数。它主要用于在滤镜图创建和连接之后，验证图的合法性并最终配置滤镜图中的所有滤镜之间的连接和格式设置。
	if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
	{
		std::cout << "avfilter_graph_config failed" << std::endl;
		goto end;
	}

end:
	avfilter_inout_free(&inputs);
	avfilter_inout_free(&outputs);

	return ret;
}

static void encode(AVCodecContext *enc_ctx, AVFrame *camera_frame,
				   AVPacket *hevc_pkt, FILE *outfile)
{
	int ret;

	/* send the frame to the encoder */
	if (camera_frame)
		printf("Send frame %3" PRId64 "\n", camera_frame->pts);

	ret = avcodec_send_frame(enc_ctx, camera_frame);
	if (ret < 0)
	{
		char err_str[1024];
		av_strerror(ret, err_str, sizeof(err_str));
		std::cout << "Error sending a frame for encoding: " << err_str << std::endl;
		exit(1);
	}

	while (ret >= 0)
	{
		ret = avcodec_receive_packet(enc_ctx, hevc_pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return;
		else if (ret < 0)
		{
			fprintf(stderr, "Error during encoding\n");
			exit(1);
		}

		printf("Write packet %3" PRId64 " (size=%5d)\n", hevc_pkt->pts,
			   hevc_pkt->size);

		fwrite(hevc_pkt->data, 1, hevc_pkt->size, outfile);

		av_packet_unref(hevc_pkt);
	}
}

int main(int argc, char **argv)
{
	int ret;
	AVPacket *packet;
	AVFrame *frame;
	AVFrame *filt_frame;
	AVPacket *hevc_pkt;

	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s input_file output_file\n", argv[0]);
		exit(1);
	}

	if ((ret = open_input_file(argv[1])) < 0)
	{
		std::cout << "open_input_file failed" << std::endl;
		return -1;
	}

	int i = 0;

	encodec = avcodec_find_encoder_by_name(ENCODER_NAME);
	if (!encodec)
	{
		std::cout << "avcodec_find_encoder_by_name failed" << std::endl;
		return -1;
	}

	encodec_ctx = avcodec_alloc_context3(encodec);
	if (!encodec_ctx)
	{
		std::cout << "avcodec_alloc_context3 failed" << std::endl;
		return -1;
	}

	if (avcodec_parameters_to_context(encodec_ctx, video_stream->codecpar) < 0)
	{
		std::cout << "avcodec_parameters_to_context failed" << std::endl;
		return -1;
	}

	encodec_ctx->width = 640;
	encodec_ctx->height = 360;
	encodec_ctx->pix_fmt = AV_PIX_FMT_NV12;
	encodec_ctx->time_base = video_stream->time_base;
	encodec_ctx->framerate = video_stream->r_frame_rate;

	if (avcodec_open2(encodec_ctx, encodec, NULL) < 0)
	{
		std::cout << "avcodec_open2 failed" << std::endl;
		return -1;
	}

	FILE *output_file = fopen(argv[2], "wb");
	if (!output_file)
	{
		std::cout << "fopen failed" << std::endl;
		return -1;
	}

	frame = av_frame_alloc();
	filt_frame = av_frame_alloc();
	packet = av_packet_alloc();
	hevc_pkt = av_packet_alloc();
	if (!frame || !filt_frame || !packet || !hevc_pkt)
	{
		fprintf(stderr, "Could not allocate frame or packet\n");
		exit(1);
	}

	if ((ret = init_filters(filter_descr)) < 0)
	{
		std::cout << "init_filters failed" << std::endl;
		return -1;
	}

	// 读取视频帧，依次进行解码 -> 滤镜处理 -> 编码 -> 写入文件
	while (1)
	{
		if ((ret = av_read_frame(fmt_ctx, packet)) < 0)
			break;

		if (packet->stream_index == video_stream_index)
		{
			ret = avcodec_send_packet(decodec_ctx, packet);
			if (ret < 0)
			{
				av_log(NULL, AV_LOG_ERROR,
					   "Error while sending a packet to the decoder\n");
				break;
			}

			while (ret >= 0)
			{
				ret = avcodec_receive_frame(decodec_ctx, frame);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				{
					break;
				}
				else if (ret < 0)
				{
					av_log(NULL, AV_LOG_ERROR,
						   "Error while receiving a frame from the decoder\n");
					return -1;
				}

				frame->pts = frame->best_effort_timestamp;

				std::cout << "frame->pts: " << frame->pts << std::endl;

				/* 将解码后的帧推入滤镜图 */
				if (av_buffersrc_write_frame(buffersrc_ctx, frame) < 0)
				{
					av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
					break;
				}

				/* 从过滤图中提取过滤后的帧 */
				while (1)
				{
					ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
					if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
					{
						break;
					}

					if (ret < 0)
						return -1;

					filt_frame->pts = filt_frame->best_effort_timestamp;
					encode(encodec_ctx, filt_frame, hevc_pkt, output_file);
					av_frame_unref(filt_frame);
				}
				av_frame_unref(frame);
			}
		}
		av_packet_unref(packet);
	}

	return 0;
}
