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

	desc_item_t* pDescItems = (desc_item_t*)&vSysMem[PAGE_SIZE << 1];
	ap_uint<32> nDescItemCount = 0;
	{
		int i = 0;
		int64_t nDescOffset;
		int nDescItems = 3;

		nDescOffset = int64_t((intptr_t)&pDescItems[i + 1] - (intptr_t)pDescItems);
		pDescItems[i].nOffsetHigh = ((nDescOffset >> 32) & 0xFFFFFFFF);
		pDescItems[i].nOffsetLow = (nDescOffset & 0xFFFFFFFF);
		pDescItems[i].nSize = nDescItems << DESC_BITSHIFT;
		i++;

		int64_t nOffset = 0;
		int64_t nSizeLeft = nDstBufSize;
		for(int j = 0;j < nDescItems;j++) {
			int nSize = (nSizeLeft > PAGE_SIZE ? PAGE_SIZE : nSizeLeft);

			pDescItems[i].nOffsetHigh = ((nOffset >> 32) & 0xFFFFFFFF);
			pDescItems[i].nOffsetLow = (nOffset & 0xFFFFFFFF);
			pDescItems[i].nSize = nSize;

			nOffset += nSize;
			nSizeLeft -= nSize;
			i++;
		}

		nDescItemCount = i;
	}

#if 1
	aximm_test1(
		pDescItems,
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
