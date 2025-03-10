#ifndef __Z_FRMBUF_WRITER_H__
#define __Z_FRMBUF_WRITER_H__

#include "ap_axi_sdata.h"
#include "ap_int.h"
#include "hls_stream.h"
#include "hls_burst_maxi.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"

#define MAX_WIDTH					4096
#define MAX_HEIGHT					2160
#define PAGE_BITSHIFT				12
#define PAGE_SIZE					(1 << PAGE_BITSHIFT)

#define SRC_PIXEL_TYPE						XF_10UC3
#define SRC_NPPC							XF_NPPC8
#define SRC_WORD_WIDTH						XF_WORDDEPTH(XF_WORDWIDTH(SRC_PIXEL_TYPE, SRC_NPPC))
typedef ap_uint<SRC_WORD_WIDTH>				src_pixel_t;
typedef hls::stream<src_pixel_t>			src_pixel_stream_t;
typedef ap_axiu<SRC_WORD_WIDTH, 1, 1, 1>	src_axi_t;
typedef hls::stream<src_axi_t>				src_axi_stream_t;

#define DST_PIXEL_TYPE				XF_8UC1
#define DST_NPPC					XF_NPPC16
#define DST_PTR_WIDTH				XF_WORDDEPTH(XF_WORDWIDTH(DST_PIXEL_TYPE, DST_NPPC))
#define DST_BITSHIFT				XF_BITSHIFT(DST_NPPC)
typedef ap_uint<DST_PTR_WIDTH>		dst_pixel_t;
typedef hls::stream<dst_pixel_t>	dst_pixel_stream_t;

#define DESC_BITSHIFT				4
#define DESC_SIZE					(1 << DESC_BITSHIFT)
struct desc_item_t {
	ap_int<32> nOffsetHigh;
	ap_uint<32> nOffsetLow;
	ap_uint<32> nSize;
	ap_uint<32> nFlags;
};
typedef hls::stream<desc_item_t> desc_item_stream_t;

extern void z_frmbuf_writer(
	src_axi_stream_t& strmSrcAxi,

	hls::burst_maxi<desc_item_t> pDescLuma,
	ap_uint<32> nDescLumaCount,
	hls::burst_maxi<dst_pixel_t> pDstLuma,
	ap_uint<32> nDstLumaStride,

	hls::burst_maxi<desc_item_t> pDescChroma,
	ap_uint<32> nDescChromaCount,
	hls::burst_maxi<dst_pixel_t> pDstChroma,
	ap_uint<32> nDstChromaStride,

	ap_uint<32> nWidth,
	ap_uint<32> nHeight);

#endif // __Z_FRMBUF_WRITER_H__
