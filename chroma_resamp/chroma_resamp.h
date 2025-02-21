#include "ap_axi_sdata.h"
#include "ap_int.h"
#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"

template <>
struct StreamType<XF_240UW> {
    typedef ap_uint<240> name;
    static const int bitdepth = 240;
};

#define MAX_WIDTH					4096
#define MAX_HEIGHT					2160

#define SRC_PIXEL_TYPE				XF_10UC3
#define SRC_NPPC					XF_NPPC8

#define DST_PIXEL_TYPE				XF_10UC3
#define DST_NPPC					XF_NPPC8

#define SRC_WORD_WIDTH				XF_WORDDEPTH(XF_WORDWIDTH(SRC_PIXEL_TYPE, SRC_NPPC))
#define DST_WORD_WIDTH				XF_WORDDEPTH(XF_WORDWIDTH(DST_PIXEL_TYPE, DST_NPPC))

typedef ap_axiu<DST_WORD_WIDTH, 1, 1, 1> src_axis_t;
typedef hls::stream<src_axis_t> src_axis_stream_t;
typedef ap_uint<DST_WORD_WIDTH> src_pixel_t;
typedef hls::stream<src_pixel_t> src_pixel_stream_t;

typedef ap_axiu<DST_WORD_WIDTH, 1, 1, 1> dst_axis_t;
typedef hls::stream<dst_axis_t> dst_axis_stream_t;
typedef ap_uint<DST_WORD_WIDTH> dst_pixel_t;
typedef hls::stream<dst_pixel_t> dst_pixel_stream_t;

#define FLAGS_YUV444_YUV444 0
#define FLAGS_YUV420_YUV444 1
#define FLAGS_YUV444_YUV422 2

extern void chroma_resamp(
	src_axis_stream_t& axis_in,
	dst_axis_stream_t& axis_out,
	ap_uint<32> nWidth,
	ap_uint<32> nHeight,
	ap_uint<32> nFlags,
	ap_uint<32>& nResult);
