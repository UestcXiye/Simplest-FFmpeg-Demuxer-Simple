// Simplest FFmpeg Demuxer Simple.cpp : 定义控制台应用程序的入口点。
//

/**
* 最简单的基于 FFmpeg 的视音频分离器（简化版）
* Simplest FFmpeg Demuxer Simple
*
* 源程序：
* 雷霄骅 Lei Xiaohua
* leixiaohua1020@126.com
* 中国传媒大学/数字电视技术
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* 修改：
* 刘文晨 Liu Wenchen
* 812288728@qq.com
* 电子科技大学/电子信息
* University of Electronic Science and Technology of China / Electronic and Information Science
* https://blog.csdn.net/ProgramNovice
*
* 本程序可以将封装格式中的视频码流数据和音频码流数据分离出来。
* 在该例子中， 将 FLV 的文件分离得到 H.264 视频码流文件和 MP3 音频码流文件。
*
* 注意：
* 这个是简化版的视音频分离器。
* 与原版的不同在于，没有初始化输出视频流和音频流的 AVFormatContext，
* 而是直接将解码后的得到的 AVPacket 中的的数据通过 fwrite() 写入文件。
* 这样做的好处是流程比较简单。
* 坏处是对一些格式的视音频码流是不适用的，比如说 FLV/MP4/MKV 等格式中的 AAC 码流
* （上述封装格式中的 AAC 的 AVPacket 中的数据缺失了 7 字节的 ADTS 文件头）。
*
* This software split a media file (in Container such as MKV, FLV, AVI...)
* to video and audio bitstream.
* In this example, it demux a FLV file to H.264 bitstream and MP3 bitstream.
*
* Note:
* This is a simple version of "Simplest FFmpeg Demuxer".
* It is more simple because it doesn't init Output Video/Audio stream's AVFormatContext.
* It writes AVPacket's data to files directly.
* The advantages of this method is simple.
* The disadvantages of this method is it's not suitable for some kind of bitstreams.
* Forexample, AAC bitstream in FLV/MP4/MKV Container Format
* (data in AVPacket lack of 7 bytes of ADTS header).
*
*/

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>

// 解决报错：'fopen': This function or variable may be unsafe.Consider using fopen_s instead.
#pragma warning(disable:4996)

// 解决报错：无法解析的外部符号 __imp__fprintf，该符号在函数 _ShowError 中被引用
#pragma comment(lib, "legacy_stdio_definitions.lib")
extern "C"
{
	// 解决报错：无法解析的外部符号 __imp____iob_func，该符号在函数 _ShowError 中被引用
	FILE __iob_func[3] = { *stdin, *stdout, *stderr };
}

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
// Windows
extern "C"
{
#include "libavformat/avformat.h"
};
#else
// Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavformat/avformat.h>
#ifdef __cplusplus
};
#endif
#endif


// 1: Use H.264 Bitstream Filter 
#define USE_H264BSF 1

int main(int argc, char* argv[])
{
	AVFormatContext *ifmt_ctx = NULL;
	AVPacket pkt;

	int ret;
	int videoindex = -1, audioindex = -1;

	// Input file URL
	const char *in_filename = "cuc_ieschool.flv";
	// Output video file URL
	const char *out_video_filename = "cuc_ieschool.h264";
	// Output audio file URL
	const char *out_audio_filename = "cuc_ieschool.mp3";

	av_register_all();

	// 输入
	ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0);
	if (ret < 0)
	{
		printf("Could not open input file.\n");
		return -1;
	}

	ret = avformat_find_stream_info(ifmt_ctx, 0);
	if (ret < 0)
	{
		printf("Failed to retrieve input stream information.\n");
		return -1;
	}

	// Print some input information
	av_dump_format(ifmt_ctx, 0, in_filename, 0);

	for (size_t i = 0; i < ifmt_ctx->nb_streams; i++)
	{
		if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			videoindex = i;
		else if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
			audioindex = i;
	}

	FILE *fp_audio = fopen(out_audio_filename, "wb+");
	FILE *fp_video = fopen(out_video_filename, "wb+");

	/*
	FIX: H.264 in some container formats (FLV, MP4, MKV etc.)
	need "h264_mp4toannexb" bitstream filter (BSF).
	1. Add SPS,PPS in front of IDR frame
	2. Add start code ("0,0,0,1") in front of NALU
	H.264 in some containers (such as MPEG2TS) doesn't need this BSF.
	*/
#if USE_H264BSF
	AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
#endif

	while (1)
	{
		// 获取一个 AVPacket
		ret = av_read_frame(ifmt_ctx, &pkt);
		if (ret < 0)
		{
			break;
		}

		if (pkt.stream_index == videoindex)
		{
#if USE_H264BSF
			av_bitstream_filter_filter(h264bsfc, ifmt_ctx->streams[videoindex]->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif
			printf("Write a video packet. size:%d\tpts:%lld\n", pkt.size, pkt.pts);
			fwrite(pkt.data, 1, pkt.size, fp_video);
		}
		else if (pkt.stream_index == audioindex)
		{
			/*
			AAC in some container formats (FLV, MP4, MKV etc.) need to
			add 7 Bytes ADTS Header in front of AVPacket data manually.
			Other Audio Codec (MP3...) works well.
			*/
			printf("Write a audio packet. size:%d\tpts:%lld\n", pkt.size, pkt.pts);
			fwrite(pkt.data, 1, pkt.size, fp_audio);
		}
		av_free_packet(&pkt);
	}


#if USE_H264BSF
	av_bitstream_filter_close(h264bsfc);
#endif

	fclose(fp_video);
	fclose(fp_audio);

	avformat_close_input(&ifmt_ctx);

	system("pause");
	return 0;
}
