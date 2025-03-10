#include <stdio.h>

#ifndef __SYNTHESIS__
#include <iostream>
#include <iomanip>
#include <cstdlib>
#endif

#include "aximm_test1.h"

struct page_info_t {
	ap_int<64> nOffset;
	ap_uint<32> nSize;
};
typedef hls::stream<page_info_t> page_info_stream_t;

void data_gen(
	ap_uint<32> nWidth,
	ap_uint<32> nHeight,
	dst_pixel_stream_t& strmDstPxl) {
	ap_uint<32> nWidthPC = nWidth >> DST_BITSHIFT;
	int s = 0;

loop_height:
	for(ap_uint<32> i = 0;i < nHeight;i++) {

loop_width:
		for(ap_uint<32> j = 0;j < nWidthPC;j++) {
			dst_pixel_t oDstPxl;
			for(ap_uint<8> t = 0;t < XF_NPIXPERCYCLE(DST_NPPC);t++) {
				oDstPxl(8*(t+1)-1, 8*t) = s + t;
			}
			s += XF_NPIXPERCYCLE(DST_NPPC);

			strmDstPxl << oDstPxl;
		}
	}
}

ap_int<64> offset_from_desc_item(const desc_item_t& oDescItem) {
#pragma HLS INLINE
	return (((ap_int<64>(oDescItem.nOffsetHigh) << 32) | ap_int<64>(oDescItem.nOffsetLow)));
}

void desm_item_to_strm(
	hls::burst_maxi<desc_item_t> pDescItem,
	ap_uint<32> nDescItemCount,
	page_info_stream_t& strmPageInfo) {
	ap_int<64> nOffset = 0;

loop_desc_item:
	for(ap_uint<32> i = 0;i < nDescItemCount;) {
		pDescItem.read_request(nOffset, DESC_SIZE);
		desc_item_t oDescItem = pDescItem.read();
		nOffset = offset_from_desc_item(oDescItem);

		pDescItem.read_request(nOffset, oDescItem.nSize);
		ap_uint<32> nItems = (oDescItem.nSize >> DESC_BITSHIFT);
loop_desc_item_pages:
		for(ap_uint<32> j = 0;j < nItems;j++) {
			desc_item_t oDescItem = pDescItem.read();
			page_info_t oPageInfo = {
				.nOffset = offset_from_desc_item(oDescItem),
				.nSize = oDescItem.nSize,
			};
			strmPageInfo << oPageInfo;
		}
		i += nItems;
	}
}

void fill_dst_pxl(
	dst_pixel_stream_t& strmDstPxl,
	page_info_stream_t& strmPageInfo,
	hls::burst_maxi<dst_pixel_t> pDstPxl,
	ap_uint<32> nDstPxlStride,
	ap_uint<32> nWidth,
	ap_uint<32> nHeight) {
	ap_int<64> nOffset;
	ap_int<32> nSize = 0;

#if 1
loop_height:
	for(ap_uint<32> i = 0;i < nHeight;i++) {
		ap_uint<32> nBytesLeft = nWidth;
loop_width:
		do {
			if(nSize <= 0) {
				page_info_t oPageInfo = strmPageInfo.read();
				nOffset = (oPageInfo.nOffset + nSize);
				nSize += oPageInfo.nSize;
			}

			ap_uint<32> nBatch;
			if(nSize > PAGE_SIZE) {
				if(PAGE_SIZE > nBytesLeft) nBatch = nBytesLeft;
				else nBatch = PAGE_SIZE;
			} else {
				if(nSize > nBytesLeft) nBatch = nBytesLeft;
				else nBatch = PAGE_SIZE;
			}

			pDstPxl.write_request(nOffset >> DST_BITSHIFT, nBatch >> DST_BITSHIFT);
loop_batch:
			for(ap_uint<32> j = 0;j < (nBatch >> DST_BITSHIFT);j++) {
				pDstPxl.write(strmDstPxl.read());
			}
			pDstPxl.write_response();

			nBytesLeft -= nBatch;
			nSize -= nBatch;
			nOffset += nBatch;
		} while(nBytesLeft > 0);

		nSize -= nDstPxlStride;
		nOffset += nDstPxlStride;
	}
#endif
}

void aximm_test1(
	hls::burst_maxi<desc_item_t> pDescItem,
	ap_uint<32> nDescItemCount,
	hls::burst_maxi<dst_pixel_t> pDstPxl,
	ap_uint<32> nDstPxlStride,
	ap_uint<32> nWidth,
	ap_uint<32> nHeight) {
#pragma HLS INTERFACE m_axi port=pDescItem offset=slave bundle=desc depth=8
#pragma HLS INTERFACE m_axi port=pDstPxl offset=slave bundle=video depth=96
#pragma HLS INTERFACE s_axilite port=nDescItemCount
#pragma HLS INTERFACE s_axilite port=nDstPxlStride
#pragma HLS INTERFACE s_axilite port=nWidth
#pragma HLS INTERFACE s_axilite port=nHeight
#pragma HLS INTERFACE s_axilite port=return

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << std::endl;
	std::cout << "MAX_WIDTH=" << MAX_WIDTH << std::endl;
	std::cout << "MAX_HEIGHT=" << MAX_HEIGHT << std::endl;
	std::cout << "DST_PTR_WIDTH=" << DST_PTR_WIDTH << std::endl;
	std::cout << "DST_BITSHIFT=" << DST_BITSHIFT << std::endl;

	std::cout << "nDescItemCount=" << nDescItemCount << std::endl;
	std::cout << "nDstPxlStride=" << nDstPxlStride << std::endl;
	std::cout << "nWidth=" << nWidth << std::endl;
	std::cout << "nHeight=" << nHeight << std::endl;
#endif

#pragma HLS DATAFLOW
	dst_pixel_stream_t strmDstPxl("strmDstPxl");
	page_info_stream_t strmPageInfo("strmPageInfo");

	data_gen(nWidth, nHeight, strmDstPxl);
	desm_item_to_strm(pDescItem, nDescItemCount, strmPageInfo);
	fill_dst_pxl(strmDstPxl, strmPageInfo, pDstPxl, nDstPxlStride, nWidth, nHeight);

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << "... DONE" << std::endl;
#endif
}
