#ifndef __AXIMM_TEST0_H__
#define __AXIMM_TEST0_H__

#include "ap_axi_sdata.h"
#include "ap_int.h"
#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"

#define MAX_WIDTH					4096
#define MAX_HEIGHT					2160

#define SRC_PIXEL_TYPE				XF_8UC3
#define SRC_NPPC					XF_NPPC2

#define DST_PIXEL_TYPE				XF_8UC1
#define DST_NPPC					XF_NPPC16

#define SRC_WORD_WIDTH				XF_WORDDEPTH(XF_WORDWIDTH(SRC_PIXEL_TYPE, SRC_NPPC))
#define DST_PTR_WIDTH				XF_WORDDEPTH(XF_WORDWIDTH(DST_PIXEL_TYPE, DST_NPPC))
#define SRC_BITSHIFT				XF_BITSHIFT(SRC_NPPC)
#define DST_BITSHIFT				XF_BITSHIFT(DST_NPPC)

#define DST_TO_SRC_NPPC				(XF_NPIXPERCYCLE(DST_NPPC) / XF_NPIXPERCYCLE(SRC_NPPC))

#define CTRL_FORMAT_BITSHIFT		(0)
#define CTRL_FORMAT_MASK			(0xFFFF)
#define FORMAT_NV12					0
#define FORMAT_NV16					1

typedef ap_axiu<SRC_WORD_WIDTH, 1, 1, 1> axis_pixel_t;
typedef hls::stream<axis_pixel_t> axis_pixel_stream_t;

typedef ap_uint<SRC_WORD_WIDTH> src_pixel_t;
typedef hls::stream<src_pixel_t> src_pixel_stream_t;

typedef ap_uint<DST_PTR_WIDTH> dst_pixel_t;
typedef hls::stream<dst_pixel_t> dst_pixel_stream_t;

extern void aximm_test0(
	dst_pixel_t* pDstY0,
	dst_pixel_t* pDstUV0,
	dst_pixel_t* pDstY1,
	dst_pixel_t* pDstUV1,
	ap_uint<32> nDstStrideY,
	ap_uint<32> nDstStrideUV,
	ap_uint<32> nWidth,
	ap_uint<32> nHeight,
	ap_uint<32> nControl);

#endif // __AXIMM_TEST0_H__
