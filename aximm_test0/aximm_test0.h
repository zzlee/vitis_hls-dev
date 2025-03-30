#ifndef __AXIMM_TEST0_H__
#define __AXIMM_TEST0_H__

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

#define DST_PIXEL_TYPE				XF_8UC1
#define DST_NPPC					XF_NPPC16
#define DST_PTR_WIDTH				XF_WORDDEPTH(XF_WORDWIDTH(DST_PIXEL_TYPE, DST_NPPC))
#define DST_BITSHIFT				XF_BITSHIFT(DST_NPPC)

typedef ap_uint<DST_PTR_WIDTH> dst_pixel_t;
typedef hls::stream<dst_pixel_t> dst_pixel_stream_t;

extern void aximm_test0(
	dst_pixel_t* pDstPxl,
	ap_uint<32> nSize,
	ap_uint<32> nTimes);

#endif // __AXIMM_TEST0_H__
