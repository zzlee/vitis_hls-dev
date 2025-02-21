#include <stdio.h>
#include <vector>
#include <iostream>
#include "lbl_wr.h"

int main () {
	int nWidth = 64;
	int nHeight = 16;
	int nDstStride = nWidth + 32;

	std::cout << "MAX_WIDTH=" << MAX_WIDTH << std::endl;
	std::cout << "MAX_HEIGHT=" << MAX_HEIGHT << std::endl;
	std::cout << "XF_NPIXPERCYCLE(DST_NPPC)=" << XF_NPIXPERCYCLE(DST_NPPC) << std::endl;
	std::cout << "XF_NPIXPERCYCLE(SRC_NPPC)=" << XF_NPIXPERCYCLE(SRC_NPPC) << std::endl;
	std::cout << "SRC_BITSHIFT=" << SRC_BITSHIFT << std::endl;
	std::cout << "DST_BITSHIFT=" << DST_BITSHIFT << std::endl;
	std::cout << "SRC_WORD_WIDTH=" << SRC_WORD_WIDTH << std::endl;
	std::cout << "DST_PTR_WIDTH=" << DST_PTR_WIDTH << std::endl;
	std::cout << "DST_TO_SRC_NPPC=" << DST_TO_SRC_NPPC << std::endl;
	std::cout << "FORMAT_NV12=" << FORMAT_NV12 << std::endl;
	std::cout << "FORMAT_NV16=" << FORMAT_NV16 << std::endl;
	std::cout << "ImgRes: " << nWidth << 'x' << nHeight << ',' << nDstStride << std::endl;

	int nWidthPCSrc = (nWidth >> SRC_BITSHIFT);

	std::cout << "input: (" << nWidthPCSrc << 'x' << nHeight << ")" << std::endl;
	axis_pixel_stream_t in_Axis("in_Axis");
	axis_pixel_t axi;
	uint8_t nVal = 0xAA;
	for(int i = 0;i < nHeight;i++) {
		std::cout << std::setw(2) << i << ": ";
		for(int j = 0;j < nWidthPCSrc;j++) {
			axi.keep = -1;
			axi.strb = -1;
			axi.id = 0;
			axi.dest = 0;
			axi.user = (i == 0 && j == 0);
			axi.last = (j == (nWidthPCSrc - 1));

			for(int t = 0;t < 6;t++) {
				axi.data(t*8 + 8-1, t*8 + 0) = nVal++;

				std::cout << std::setw(2) << std::hex << (int)axi.data(t*8 + 8-1, t*8 + 0) << ' ' << std::dec;
			}
			std::cout << '(' << axi.user << ',' << axi.last << ')' << ' ';

			in_Axis << axi;
		}

		std::cout << std::dec << std::endl;
	}

	int nDstStridePC = (nDstStride >> DST_BITSHIFT);
	std::vector<ap_uint<DST_PTR_WIDTH> > vDstBuf(nDstStridePC * nHeight * 2); // NV16
	uint8_t* pDstBuf = (uint8_t*)&vDstBuf[0];

	ap_uint<DST_PTR_WIDTH>* pDstY0 = &vDstBuf[0];
	ap_uint<DST_PTR_WIDTH>* pDstUV0 = &vDstBuf[nDstStridePC * nHeight];
	ap_uint<DST_PTR_WIDTH>* pDstY1 = pDstY0 + ((nDstStridePC * nHeight) >> 1);
	ap_uint<DST_PTR_WIDTH>* pDstUV1 = pDstUV0 + ((nDstStridePC * nHeight) >> 1);
	ap_uint<32> nControl = FORMAT_NV16;

	lbl_wr(
		in_Axis,
		pDstY0,
		pDstUV0,
		pDstY1,
		pDstUV1,
		nDstStride,
		nDstStride,
		nWidth,
		nHeight,
		nControl);

	std::cout << "output: (" << nDstStride << 'x' << nHeight*2 << ")" << std::endl;
	for(int i = 0;i < nHeight * 2;i++) {
		std::cout << std::setw(2) << i << ": ";
		for(int j = 0;j < nDstStride;j++) {
			int nIndex = i * nDstStride + j;

			std::cout << std::setw(2) << std::hex << (int)pDstBuf[nIndex] << ' ' << std::dec;
		}
		std::cout << std::dec << std::endl;
	}

	return 0;
}
