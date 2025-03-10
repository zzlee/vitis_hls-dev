#include <stdio.h>
#include <vector>
#include <iostream>
#include "aximm_test1.h"

int main () {
	int nWidth = 64;
	int nHeight = 16;
	int nStride = nWidth + 32;

	std::vector<uint8_t> vDstBuf(nStride * nHeight);
	std::vector<desc_item_t> vDescItems;

	for(int i = 0;i < nHeight / 2;i++) {
		ap_uint<64> nOffset = i * nStride * 2;

		vDescItems.push_back(
			(desc_item_t) {
				.nFlags = 0,
				.nOffsetHigh = ((nOffset >> 32) & 0xFFFFFFFF),
				.nOffsetLow = (nOffset & 0xFFFFFFFF),
				.nSize = nStride * 2,
			}
		);
	}

	std::cout << "desc_items:" << std::endl;
	for(size_t i = 0;i < vDescItems.size();i++) {
		const desc_item_t& oDescItem = vDescItems[i];
		ap_int<64> nOffset = ((int64_t)oDescItem.nOffsetHigh << 32) | (int64_t)oDescItem.nOffsetLow;

		std::cout << '[' << i << "] +" << nOffset << " size=" << oDescItem.nSize << std::endl;
	}

#if 1
	aximm_test1(
		&vDescItems[0],
		vDescItems.size(),
		(ap_uint<DST_PTR_WIDTH>*)&vDstBuf[0],
		nStride,
		nWidth,
		nHeight);
#endif

	for(int i = 0;i < nHeight;i++) {
		std::cout << std::setw(2) << i << ": ";
		for(int j = 0;j < nStride;j++) {
			int nIndex = i * nStride + j;

			std::cout << std::setw(2) << std::hex << (int)vDstBuf[nIndex] << std::dec << " ";
		}
		std::cout << std::endl;
	}

	return 0;
}
