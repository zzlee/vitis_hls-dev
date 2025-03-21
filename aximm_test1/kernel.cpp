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

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ": ++++" << std::endl;
#endif

loop_height:
	for(ap_uint<32> i = 0;i < nHeight;i++) {

#ifndef __SYNTHESIS__
		std::cout << "[" << i << "] ";
#endif

loop_width:
		for(ap_uint<32> j = 0;j < nWidthPC;j++) {
#pragma HLS PIPELINE II=1
			dst_pixel_t oDstPxl;
			for(ap_uint<8> t = 0;t < XF_NPIXPERCYCLE(DST_NPPC);t++) {
				oDstPxl(8*(t+1)-1, 8*t) = s + t;
			}
			s += XF_NPIXPERCYCLE(DST_NPPC);

#ifndef __SYNTHESIS__
			std::cout << std::hex << std::setw(16) << oDstPxl << ' ';
#endif

			strmDstPxl << oDstPxl;
		}

#ifndef __SYNTHESIS__
		std::cout << std::dec << std::endl;
#endif
	}

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ": ----" << std::endl;
#endif
}

ap_int<64> offset_from_desc_item(const desc_item_t& oDescItem) {
#pragma HLS INLINE
	return (((ap_int<64>(oDescItem.nOffsetHigh) << 32) | ap_int<64>(oDescItem.nOffsetLow)));
}

void desc_item_to_strm(
	hls::burst_maxi<desc_item_t> pDescItem,
	ap_uint<32> nDescItemCount,
	page_info_stream_t& strmPageInfo) {
	ap_int<64> nDescOffset = 0;

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ": ++++" << std::endl;
#endif

loop_desc_item:
	for(ap_uint<32> i = 0;i < nDescItemCount;) {
		// read desc header
		pDescItem.read_request(nDescOffset >> DESC_BITSHIFT, 1);
		desc_item_t oDescHeader = pDescItem.read();
		nDescOffset = offset_from_desc_item(oDescHeader);

#ifndef __SYNTHESIS__
		std::cout << "d[" << i << "] +" << nDescOffset << " size=" << oDescHeader.nSize << std::endl;
#endif

		i++;

		// read desc items
		pDescItem.read_request(nDescOffset >> DESC_BITSHIFT, oDescHeader.nSize >> DESC_BITSHIFT);
		ap_uint<32> nItems = (oDescHeader.nSize >> DESC_BITSHIFT);


loop_desc_item_pages:
		for(ap_uint<32> j = 0;j < nItems;j++) {
			desc_item_t oDescItem = pDescItem.read();

			page_info_t oPageInfo = {
				.nOffset = offset_from_desc_item(oDescItem),
				.nSize = oDescItem.nSize,
			};

#ifndef __SYNTHESIS__
			std::cout << "p[" << (i+j) << "] +" << oPageInfo.nOffset << " size=" << oPageInfo.nSize << std::endl;
#endif

			strmPageInfo << oPageInfo;
		}

		i += nItems;
		nDescOffset += nItems << DESC_BITSHIFT;

#ifndef __SYNTHESIS__
		std::cout << "(nDescOffset=" << nDescOffset << ")" << std::endl;
#endif
}

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ": ----" << std::endl;
#endif
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

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ": ++++" << std::endl;
#endif

#if 1
loop_height:
	for(ap_uint<32> i = 0;i < nHeight;i++) {
		ap_uint<32> nBytesLeft = nWidth;

#ifndef __SYNTHESIS__
		std::cout << "[" << i << "] ";
#endif

loop_width:
		do {
			if(nSize <= 0) {
				page_info_t oPageInfo = strmPageInfo.read();

#ifndef __SYNTHESIS__
				std::cout << "(page +" << oPageInfo.nOffset << " size=" << oPageInfo.nSize << ')';
#endif

				nOffset = (oPageInfo.nOffset - nSize);
				nSize += oPageInfo.nSize;
			}

			ap_uint<32> nBurst;
			if(nSize > PAGE_SIZE) {
				if(PAGE_SIZE > nBytesLeft) nBurst = nBytesLeft;
				else nBurst = PAGE_SIZE;
			} else {
				if(nSize > nBytesLeft) nBurst = nBytesLeft;
				else nBurst = nSize;
			}

#ifndef __SYNTHESIS__
			std::cout << "(nBurst=" << nBurst << ")";
#endif

			pDstPxl.write_request(nOffset >> DST_BITSHIFT, nBurst >> DST_BITSHIFT);
loop_batch:
			for(ap_uint<32> j = 0;j < (nBurst >> DST_BITSHIFT);j++) {
				pDstPxl.write(strmDstPxl.read());
			}
			pDstPxl.write_response();

			nBytesLeft -= nBurst;
			nSize -= nBurst;
			nOffset += nBurst;

#ifndef __SYNTHESIS__
			std::cout << "(nBytesLeft=" << nBytesLeft << ", nSize=" << nSize << ", nOffset=" << nOffset << ")";
#endif

		} while(nBytesLeft > 0);

		nSize -= (nDstPxlStride - nWidth);
		nOffset += (nDstPxlStride - nWidth);

#ifndef __SYNTHESIS__
		std::cout << "(nSize=" << nSize << ", nOffset=" << nOffset << ")" << std::endl;
#endif
	}
#endif

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ": ----" << std::endl;
#endif
}

void aximm_test1(
	hls::burst_maxi<desc_item_t> pDescItem,
	ap_uint<32> nDescItemCount,
	hls::burst_maxi<dst_pixel_t> pDstPxl,
	ap_uint<32> nDstPxlStride,
	ap_uint<32> nWidth,
	ap_uint<32> nHeight) {
#pragma HLS INTERFACE m_axi port=pDescItem offset=slave bundle=desc depth=0x20000
#pragma HLS INTERFACE m_axi port=pDstPxl offset=slave bundle=video depth=0x20000
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
	desc_item_to_strm(pDescItem, nDescItemCount, strmPageInfo);
	fill_dst_pxl(strmDstPxl, strmPageInfo, pDstPxl, nDstPxlStride, nWidth, nHeight);

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << "... DONE" << std::endl;
#endif
}
