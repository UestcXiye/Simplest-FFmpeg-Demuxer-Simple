// Simplest FFmpeg Demuxer Simple.cpp : �������̨Ӧ�ó������ڵ㡣
//

/**
* ��򵥵Ļ��� FFmpeg ������Ƶ���������򻯰棩
* Simplest FFmpeg Demuxer Simple
*
* Դ����
* ������ Lei Xiaohua
* leixiaohua1020@126.com
* �й���ý��ѧ/���ֵ��Ӽ���
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* �޸ģ�
* ���ĳ� Liu Wenchen
* 812288728@qq.com
* ���ӿƼ���ѧ/������Ϣ
* University of Electronic Science and Technology of China / Electronic and Information Science
* https://blog.csdn.net/ProgramNovice
*
* ��������Խ���װ��ʽ�е���Ƶ�������ݺ���Ƶ�������ݷ��������
* �ڸ������У� �� FLV ���ļ�����õ� H.264 ��Ƶ�����ļ��� MP3 ��Ƶ�����ļ���
*
* ע�⣺
* ����Ǽ򻯰������Ƶ��������
* ��ԭ��Ĳ�ͬ���ڣ�û�г�ʼ�������Ƶ������Ƶ���� AVFormatContext��
* ����ֱ�ӽ������ĵõ��� AVPacket �еĵ�����ͨ�� fwrite() д���ļ���
* �������ĺô������̱Ƚϼ򵥡�
* �����Ƕ�һЩ��ʽ������Ƶ�����ǲ����õģ�����˵ FLV/MP4/MKV �ȸ�ʽ�е� AAC ����
* ��������װ��ʽ�е� AAC �� AVPacket �е�����ȱʧ�� 7 �ֽڵ� ADTS �ļ�ͷ����
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

// �������'fopen': This function or variable may be unsafe.Consider using fopen_s instead.
#pragma warning(disable:4996)

// ��������޷��������ⲿ���� __imp__fprintf���÷����ں��� _ShowError �б�����
#pragma comment(lib, "legacy_stdio_definitions.lib")
extern "C"
{
	// ��������޷��������ⲿ���� __imp____iob_func���÷����ں��� _ShowError �б�����
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

	// ����
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
		// ��ȡһ�� AVPacket
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
