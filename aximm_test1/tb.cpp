#include <stdio.h>
#include <vector>
#include <iostream>
#include "aximm_test1.h"

namespace tb {
	ap_int<64> offset_from_desc_item(const desc_item_t& oDescItem) {
		return (((ap_int<64>(oDescItem.nOffsetHigh) << 32) | ap_int<64>(oDescItem.nOffsetLow)));
	}
}

using namespace tb;

int main () {
	int nWidth = 128;
	int nHeight = 64;
	int nStride = nWidth + 16;

	std::vector<uint8_t> vSysMem(PAGE_SIZE << 16);

	uint8_t* pDstBuf = &vSysMem[0]; // nStride * nHeight;
	int64_t nDstBufSize = nStride * nHeight;

	desc_item_t* pDescBase = (desc_item_t*)&vSysMem[PAGE_SIZE << 4];
	ap_uint<32> nDescItemCount = 1;
	{
		desc_item_t* pDescItems = pDescBase;
		int64_t nOffset = 0;
		int64_t nSizeLeft = nDstBufSize;
		while(true) {
			int nSize = (nSizeLeft > PAGE_SIZE ? PAGE_SIZE : nSizeLeft);

			pDescItems->nOffsetHigh = ((nOffset >> 32) & 0xFFFFFFFF);
			pDescItems->nOffsetLow = (nOffset & 0xFFFFFFFF);
			pDescItems->nSize = nSize;
			pDescItems++;

			nOffset += nSize;
			nSizeLeft -= nSize;

			if(nSizeLeft <= 0) {
				pDescItems->nOffsetHigh = 0;
				pDescItems->nOffsetLow = 0;
				pDescItems->nSize = 0;
				break;
			}

			int64_t nDescOffset = (int64_t)((uintptr_t)&pDescItems[1] - (uintptr_t)pDescBase);
			pDescItems->nOffsetHigh = ((nDescOffset >> 32) & 0xFFFFFFFF);
			pDescItems->nOffsetLow = (nDescOffset & 0xFFFFFFFF);
			pDescItems->nSize = nDescItemCount;

			pDescItems++;
		}
	}

#if 1
	aximm_test1(
		pDescBase,
		nDescItemCount,
		(ap_uint<DST_PTR_WIDTH>*)pDstBuf,
		nStride,
		nWidth,
		nHeight);
#endif

	{
		for(int i = 0;i < nHeight;i++) {
			std::cout << std::setw(2) << i << ": ";
			for(int j = 0;j < nStride;j++) {
				int nIndex = i * nStride + j;

				std::cout << std::setw(2) << std::hex << (int)pDstBuf[nIndex] << std::dec << " ";
			}
			std::cout << std::endl;
		}
	}

	return 0;
}
