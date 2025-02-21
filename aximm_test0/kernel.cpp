#include <stdio.h>

#ifndef __SYNTHESIS__
#include <iostream>
#include <iomanip>
#include <cstdlib>
#endif

#include "aximm_test0.h"

void aximm_test0(
	dst_pixel_t* pDstY0,
	dst_pixel_t* pDstUV0,
	dst_pixel_t* pDstY1,
	dst_pixel_t* pDstUV1,
	ap_uint<32> nDstStrideY,
	ap_uint<32> nDstStrideUV,
	ap_uint<32> nWidth,
	ap_uint<32> nHeight,
	ap_uint<32> nControl) {
#pragma HLS INTERFACE m_axi port=pDstY0 offset=slave bundle=mm_video0
#pragma HLS INTERFACE m_axi port=pDstUV0 offset=slave bundle=mm_video0
#pragma HLS INTERFACE m_axi port=pDstY1 offset=slave bundle=mm_video0
#pragma HLS INTERFACE m_axi port=pDstUV1 offset=slave bundle=mm_video0
#pragma HLS INTERFACE s_axilite port=nDstStrideY
#pragma HLS INTERFACE s_axilite port=nDstStrideUV
#pragma HLS INTERFACE s_axilite port=nWidth
#pragma HLS INTERFACE s_axilite port=nHeight
#pragma HLS INTERFACE s_axilite port=nControl
#pragma HLS INTERFACE s_axilite port=return

	ap_uint<32> nWidthPC = nWidth >> DST_BITSHIFT;
	ap_uint<32> nWidth_2 = (nWidth >> 1);
	ap_uint<32> nHeight_2 = (nHeight >> 1);
	ap_uint<32> nDstYSkip = (nDstStrideY >> DST_BITSHIFT) - nWidthPC;
	ap_uint<32> nDstUVSkip = (nDstStrideUV >> DST_BITSHIFT) - nWidthPC;

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << std::endl;
	std::cout << "SRC_WORD_WIDTH=" << SRC_WORD_WIDTH << std::endl;
	std::cout << "DST_PTR_WIDTH=" << DST_PTR_WIDTH << std::endl;
	std::cout << "SRC_BITSHIFT=" << SRC_BITSHIFT << std::endl;
	std::cout << "DST_BITSHIFT=" << DST_BITSHIFT << std::endl;
	std::cout << "DST_TO_SRC_NPPC=" << DST_TO_SRC_NPPC << std::endl;
	std::cout << nWidth << 'x' << nHeight << ',' << nDstStrideY << ',' << nDstStrideUV << std::endl;
	std::cout << nWidthPC << 'x' << nHeight_2 << ',' << nDstYSkip << ',' << nDstUVSkip << std::endl;
#endif

	ap_uint<32> nPos_dstY0 = 0, nPos_dstY1 = 0, nPos_dstUV0 = 0, nPos_dstUV1 = 0;

loop_height:
	for (ap_uint<32> i = 0; i < nHeight_2;i++, nPos_dstY0 += nDstYSkip, nPos_dstY1 += nDstYSkip, nPos_dstUV0 += nDstUVSkip, nPos_dstUV1 += nDstUVSkip) {

loop_width_even:
		for (ap_uint<32> j = 0;j < nWidthPC;j++, nPos_dstY0++, nPos_dstUV0++) {
#pragma HLS PIPELINE II=2

			pDstY0[nPos_dstY0] = dst_pixel_t(i) << 24 | dst_pixel_t(j);
			pDstUV0[nPos_dstUV0] = dst_pixel_t(i) << 24 | dst_pixel_t(j);
		}

loop_width_odd:
		for (ap_uint<32> j = 0;j < nWidthPC;j++, nPos_dstY1++, nPos_dstUV1++) {
#pragma HLS PIPELINE II=2

			pDstY1[nPos_dstY1] = dst_pixel_t(nHeight_2 + i) << 24 | dst_pixel_t(j);
			pDstUV1[nPos_dstUV1] = dst_pixel_t(nHeight_2 + i) << 24 | dst_pixel_t(j);
		}
	}
}
