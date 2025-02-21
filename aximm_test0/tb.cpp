#include <stdio.h>
#include <vector>
#include <iostream>
#include "aximm_test0.h"

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
	std::cout << "ImgRes: " << nWidth << 'x' << nHeight << ',' << nDstStride << std::endl;

	int nWidthPCSrc = (nWidth >> SRC_BITSHIFT);
	std::cout << "nWidthPCSrc=" << nWidthPCSrc << std::endl;

	int nDstStridePC = (nDstStride >> DST_BITSHIFT);
	std::cout << "nDstStridePC=" << nDstStridePC << std::endl;
	std::vector<ap_uint<DST_PTR_WIDTH> > vDstBuf(nDstStridePC * nHeight * 2); // NV16

	ap_uint<DST_PTR_WIDTH>* pDstY0 = &vDstBuf[0];
	ap_uint<DST_PTR_WIDTH>* pDstUV0 = &vDstBuf[nDstStridePC * nHeight];
	ap_uint<DST_PTR_WIDTH>* pDstY1 = pDstY0 + ((nDstStridePC * nHeight) >> 1);
	ap_uint<DST_PTR_WIDTH>* pDstUV1 = pDstUV0 + ((nDstStridePC * nHeight) >> 1);
	ap_uint<32> nControl = 0;

	aximm_test0(
		pDstY0,
		pDstUV0,
		pDstY1,
		pDstUV1,
		nDstStride,
		nDstStride,
		nWidth,
		nHeight,
		nControl);

	for(int i = 0;i < nHeight * 2;i++) {
		std::cout << i << ": ";
		for(int j = 0;j < nDstStridePC;j++) {
			int nIndex = i * nDstStridePC + j;

			std::cout << std::setw(32) << std::hex <<
				vDstBuf[nIndex] << '[' << std::dec << nIndex << "] ";
		}
		std::cout << std::dec << std::endl;
	}

	return 0;
}
