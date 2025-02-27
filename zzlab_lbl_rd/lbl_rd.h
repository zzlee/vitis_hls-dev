#include "ap_axi_sdata.h"
#include "ap_int.h"
#include "hls_stream.h"
#include "hls_burst_maxi.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"

#define MAX_WIDTH					4096
#define MAX_HEIGHT					2160

#define SRC_PIXEL_TYPE				XF_8UC1
#define SRC_NPPC					XF_NPPC16

#define DST_PIXEL_TYPE				XF_8UC3
#define DST_NPPC					XF_NPPC2

#define SRC_PTR_WIDTH				XF_WORDDEPTH(XF_WORDWIDTH(SRC_PIXEL_TYPE, SRC_NPPC))
#define DST_WORD_WIDTH				XF_WORDDEPTH(XF_WORDWIDTH(DST_PIXEL_TYPE, DST_NPPC))
#define SRC_BITSHIFT				XF_BITSHIFT(SRC_NPPC)
#define DST_BITSHIFT				XF_BITSHIFT(DST_NPPC)

#define SRC_TO_DST_NPPC				(XF_NPIXPERCYCLE(SRC_NPPC) / XF_NPIXPERCYCLE(DST_NPPC))

typedef ap_axiu<DST_WORD_WIDTH, 1, 1, 1> axis_pixel_t;
typedef hls::stream<axis_pixel_t> axis_pixel_stream_t;

typedef ap_uint<SRC_PTR_WIDTH> src_pixel_t;
typedef hls::stream<src_pixel_t> src_pixel_stream_t;

typedef ap_uint<DST_WORD_WIDTH> dst_pixel_t;
typedef hls::stream<dst_pixel_t> dst_pixel_stream_t;

extern void lbl_rd(
#if 0
	ap_uint<SRC_PTR_WIDTH>* pSrcY0,
	ap_uint<SRC_PTR_WIDTH>* pSrcY1,
	ap_uint<SRC_PTR_WIDTH>* pSrcUV0,
	ap_uint<SRC_PTR_WIDTH>* pSrcUV1,
#else
	hls::burst_maxi<src_pixel_t> pSrcY0,
	hls::burst_maxi<src_pixel_t> pSrcUV0,
	hls::burst_maxi<src_pixel_t> pSrcY1,
	hls::burst_maxi<src_pixel_t> pSrcUV1,
#endif
	ap_uint<32> nSrcStrideY,
	ap_uint<32> nSrcStrideUV,
	axis_pixel_stream_t& s_axis,
	ap_uint<32> nWidth,
	ap_uint<32> nHeight,
	ap_uint<32> nControl);
