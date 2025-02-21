#include <stdio.h>
#include <vector>
#include <iostream>
#include "lbl_rd.h"

int main () {
	int nWidth = 64;
	int nHeight = 16;
	int nWidth_pc = (nWidth >> SRC_BITSHIFT);
	int nSrcStride = nWidth + 32;
	int nSrcStride_pc = (nSrcStride >> SRC_BITSHIFT);

	std::cout << "MAX_WIDTH:" << MAX_WIDTH << std::endl;
	std::cout << "MAX_HEIGHT:" << MAX_HEIGHT << std::endl;
	std::cout << "SRC_PTR_WIDTH:" << SRC_PTR_WIDTH << std::endl;
	std::cout << "DST_WORD_WIDTH:" << DST_WORD_WIDTH << std::endl;
	std::cout << "SRC_BITSHIFT:" << SRC_BITSHIFT << std::endl;
	std::cout << "DST_BITSHIFT:" << DST_BITSHIFT << std::endl;
	std::cout << "SRC_TO_DST_NPPC:" << SRC_TO_DST_NPPC << std::endl;
	// std::cout << "FORMAT_NV12:" << FORMAT_NV12 << std::endl;
	// std::cout << "FORMAT_NV16:" << FORMAT_NV16 << std::endl;

	std::cout << nWidth << 'x' << nHeight << ',' << nSrcStride << std::endl;
	std::cout << nWidth_pc << ',' << nSrcStride_pc << std::endl;

	std::vector<ap_uint<SRC_PTR_WIDTH> > vSrcBuf(nSrcStride_pc * nHeight * 2); // NV16
	uint8_t* pSrcBuf = (uint8_t*)&vSrcBuf[0];

	uint8_t nVal = 0xAA;
	for(int i = 0;i < nHeight * 2;i++) {
		for(int j = 0;j < nWidth;j++) {
			int nIndex = i * nSrcStride + j;

			pSrcBuf[nIndex] = nVal;
			nVal++;
		}
	}

	std::cout << "input: (" << nSrcStride << 'x' << nHeight*2 << ")" << std::endl;
	for(int i = 0;i < nHeight * 2;i++) {
		std::cout << std::setw(2) << i << ": ";
		for(int j = 0;j < nSrcStride;j++) {
			int nIndex = i * nSrcStride + j;

			std::cout << std::setw(2) << std::hex << (int)pSrcBuf[nIndex] << ' ' << std::dec;
		}
		std::cout << std::dec << std::endl;
	}

	ap_uint<SRC_PTR_WIDTH>* pSrcY0 = &vSrcBuf[0];
	ap_uint<SRC_PTR_WIDTH>* pSrcUV0 = &vSrcBuf[nSrcStride_pc * nHeight];
	ap_uint<SRC_PTR_WIDTH>* pSrcY1 = pSrcY0 + nSrcStride_pc * nHeight / 2;
	ap_uint<SRC_PTR_WIDTH>* pSrcUV1 = pSrcUV0 + nSrcStride_pc * nHeight / 2;
	axis_pixel_stream_t out_Axis("out_Axis");
	ap_uint<32> nControl = 0;

#if 1
	lbl_rd(
		pSrcY0,
		pSrcUV0,
		pSrcY1,
		pSrcUV1,
		nSrcStride,
		nSrcStride,
		out_Axis,
		nWidth,
		nHeight,
		nControl);
#endif

	int nWidth_dst_pc = nWidth >> DST_BITSHIFT;

	std::cout << "output: (" << nWidth_dst_pc << 'x' << nHeight << ")" << std::endl;
	for(int i = 0;i < nHeight;i++) {
		std::cout << std::setw(2) << i << ": ";
		for(int j = 0;j < nWidth_dst_pc;j++) {
			axis_pixel_t axi = out_Axis.read();

			for(int t = 0;t < 6;t++) {
				std::cout << std::setw(2) << std::hex << (int)axi.data(t*8 + 8-1, t*8 + 0) << ' ' << std::dec;
			}
			std::cout << '(' << axi.user << ',' << axi.last << ')' << ' ';
		}
		std::cout << std::endl;
	}

	return 0;
}
