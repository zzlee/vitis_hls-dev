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

struct burst_info_t {
	ap_int<64> nOffset;
	ap_uint<32> nBurst;
};
typedef hls::stream<burst_info_t> burst_info_stream_t;

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

void desc_item_to_strm_v0(
	hls::burst_maxi<desc_item_t> pDescItem,
	ap_uint<32> nDescItemCount,
	page_info_stream_t& strmPageInfo) {
	ap_int<64> nDescOffset = 0;
	ap_uint<32> nItems = nDescItemCount;

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ": ++++" << std::endl;
#endif

loop_all_desc_items:
	while(nItems > 0) {
#pragma HLS PIPELINE
		pDescItem.read_request(nDescOffset >> DESC_BITSHIFT, nItems + 1);

loop_desc_item_pages:
		for(ap_uint<32> i = 0;i < nItems;i++) {
#pragma HLS PIPELINE
			desc_item_t oDescItem = pDescItem.read();

			page_info_t oPageInfo = {
				.nOffset = offset_from_desc_item(oDescItem),
				.nSize = oDescItem.nSize,
			};

#ifndef __SYNTHESIS__
			std::cout << "p[" << i << "] +" << oPageInfo.nOffset << " size=" << oPageInfo.nSize << std::endl;
#endif

			strmPageInfo << oPageInfo;
		}

		// next desc items
		desc_item_t oNextDesc = pDescItem.read();
		nDescOffset = offset_from_desc_item(oNextDesc);
		nItems = oNextDesc.nSize;

#ifndef __SYNTHESIS__
		std::cout << "(nDescOffset=" << nDescOffset << ", nItems=" << nItems << ")" << std::endl;
#endif
	}

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ": ----" << std::endl;
#endif
}

void desc_item_to_strm_v1(
	desc_item_t* pDescItem,
	ap_uint<32> nDescItemCount,
	page_info_stream_t& strmPageInfo) {
	ap_int<64> nDescOffset = 0;
	ap_uint<32> nItems = nDescItemCount;

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ": ++++" << std::endl;
#endif

loop_all_desc_items:
	while(nItems > 0) {
loop_desc_item_pages:
		for(ap_uint<32> i = 0;i < nItems;i++) {
#pragma HLS PIPELINE II=2
			desc_item_t oDescItem = pDescItem[nDescOffset + i];

			page_info_t oPageInfo = {
				.nOffset = offset_from_desc_item(oDescItem),
				.nSize = oDescItem.nSize,
			};

#ifndef __SYNTHESIS__
			std::cout << "p[" << i << "] +" << oPageInfo.nOffset << " size=" << oPageInfo.nSize << std::endl;
#endif

			strmPageInfo << oPageInfo;
		}

		// next desc items
		desc_item_t oNextDesc = pDescItem[nDescOffset + nItems];
		nDescOffset = offset_from_desc_item(oNextDesc);
		nItems = oNextDesc.nSize;

#ifndef __SYNTHESIS__
		std::cout << "(nDescOffset=" << nDescOffset << ", nItems=" << nItems << ")" << std::endl;
#endif
	}

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ": ----" << std::endl;
#endif
}

void page_to_burst_strm(
	page_info_stream_t& strmPageInfo,
	burst_info_stream_t& strmBurstInfo,
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

			burst_info_t oBurstInfo = {
				.nOffset = nOffset >> DST_BITSHIFT,
				.nBurst = nBurst >> DST_BITSHIFT
			};
			strmBurstInfo.write(oBurstInfo);

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

	// end of stream
	burst_info_t oBurstInfo = {
		.nOffset = 0,
		.nBurst = 0
	};
	strmBurstInfo.write(oBurstInfo);

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ": ----" << std::endl;
#endif
}

void fill_dst_pxl_burst_v0(
	dst_pixel_stream_t& strmDstPxl,
	burst_info_stream_t& strmBurstInfo,
	hls::burst_maxi<dst_pixel_t> pDstPxl) {
	while(true) {
		burst_info_t oBurstInfo = strmBurstInfo.read();

		// end of stream
		if(oBurstInfo.nBurst == 0)
			break;

		pDstPxl.write_request(oBurstInfo.nOffset, oBurstInfo.nBurst);
loop_burst:
		for(ap_uint<32> j = 0;j < oBurstInfo.nBurst;j++) {
			pDstPxl.write(strmDstPxl.read());
		}
		pDstPxl.write_response();
	}
}

void fill_dst_pxl_burst_v1(
	dst_pixel_stream_t& strmDstPxl,
	burst_info_stream_t& strmBurstInfo,
	dst_pixel_t* pDstPxl) {
	while(true) {
		burst_info_t oBurstInfo = strmBurstInfo.read();

		// end of stream
		if(oBurstInfo.nBurst == 0)
			break;

loop_burst:
		for(ap_uint<32> j = 0;j < oBurstInfo.nBurst;j++) {
			pDstPxl[oBurstInfo.nOffset + j] = strmDstPxl.read();
		}
	}
}

void fill_dst_pxl_v0(
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
#pragma HLS PIPELINE
		ap_uint<32> nBytesLeft = nWidth;

#ifndef __SYNTHESIS__
		std::cout << "[" << i << "] ";
#endif

loop_width:
		do {
#pragma HLS PIPELINE
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
#pragma HLS PIPELINE
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

	dst_pixel_stream_t strmDstPxl("strmDstPxl");
	page_info_stream_t strmPageInfo("strmPageInfo");
	burst_info_stream_t strmBurstInfo("strmBurstInfo");

#pragma HLS DATAFLOW
	data_gen(nWidth, nHeight, strmDstPxl);

#if 1
	desc_item_to_strm_v0(pDescItem, nDescItemCount, strmPageInfo);
#endif

#if 0
	desc_item_to_strm_v1(pDescItem, nDescItemCount, strmPageInfo);
#endif

#if 1
	fill_dst_pxl_v0(strmDstPxl, strmPageInfo, pDstPxl, nDstPxlStride, nWidth, nHeight);
#endif

#if 0
	page_to_burst_strm(strmPageInfo, strmBurstInfo, nDstPxlStride, nWidth, nHeight);

#if 0
	fill_dst_pxl_burst_v0(strmDstPxl, strmBurstInfo, pDstPxl);
#endif

#if 1
	fill_dst_pxl_burst_v1(strmDstPxl, strmBurstInfo, pDstPxl);
#endif

#endif

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << "... DONE" << std::endl;
#endif
}
