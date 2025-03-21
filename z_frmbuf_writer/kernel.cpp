#include <stdio.h>

#ifndef __SYNTHESIS__
#include <iostream>
#include <iomanip>
#include <cstdlib>
#endif

#include "z_frmbuf_writer.h"

#define ERROR_IO_EOL_EARLY  (1 << 0)
#define ERROR_IO_EOL_LATE   (1 << 1)
#define ERROR_IO_SOF_EARLY  (1 << 0)
#define ERROR_IO_SOF_LATE   (1 << 1)

struct page_info_t {
	ap_int<64> nOffset;
	ap_uint<32> nSize;
};
typedef hls::stream<page_info_t> page_info_stream_t;

void src_axis_to_strm(
	src_axi_stream_t& strmSrcAxi,
	src_pixel_stream_t& strmSrc,
	ap_uint<32> nWidth_8,
	ap_uint<32> nHeight,
	hls::stream<ap_uint<32> >& strmRes)
{
    ap_uint<32> res = 0;
	src_axi_t axi;

	bool sof = 0;
loop_wait_for_start:
	while (!sof) {
#pragma HLS PIPELINE II=1
#pragma HLS loop_tripcount avg=0 max=0
		strmSrcAxi.read(axi);
		sof = axi.user.to_int();
	}

loop_height:
	for (ap_uint<32> i = 0; i < nHeight; i++) {
#pragma HLS loop_tripcount avg=0 max=0
		bool eol = 0;
loop_width:
		for (ap_uint<32> j = 0; j < nWidth_8; j++) {
#pragma HLS loop_flatten off
#pragma HLS pipeline
#pragma HLS loop_tripcount avg=0 max=0
			if (sof || eol) {
				sof = 0;
				eol = axi.last.to_int();
			} else {
				// If we didn't reach EOL, then read the next pixel
				strmSrcAxi.read(axi);
				eol = axi.last.to_int();
				bool user = axi.user.to_int();
				if(user) {
					res |= ERROR_IO_SOF_EARLY;
				}
			}
			if (eol && (j != nWidth_8-1)) {
				res |= ERROR_IO_EOL_EARLY;
			}

			strmSrc.write(axi.data);
		}

loop_wait_for_eol:
		while (!eol) {
#pragma HLS pipeline II=1
#pragma HLS loop_tripcount avg=0 max=0
			// Keep reading until we get to EOL
			strmSrcAxi.read(axi);
			eol = axi.last.to_int();
			res |= ERROR_IO_EOL_LATE;
		}
	}

	strmRes.write(res);
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

inline void src_to_luma_chroma(const src_pixel_t& src0, const src_pixel_t& src1, dst_pixel_t& luma, dst_pixel_t& chroma) {
	// first 8 tuples
	for(int i = 0;i < 4;i++) {
		luma(i*16 + 8*1-1, i*16 + 8*0) = src0(i*60 + 10*1-1, i*60 + 10*0+2); // Y0
		luma(i*16 + 8*2-1, i*16 + 8*1) = src0(i*60 + 10*4-1, i*60 + 10*3+2); // Y1
		chroma(i*16 + 8*1-1, i*16 + 8*0) = (src0(i*60 + 10*2-1, i*60 + 10*1+2) + src0(i*60 + 10*5-1, i*60 + 10*4+2)) >> 1; // (U0 + U1) / 2
		chroma(i*16 + 8*2-1, i*16 + 8*1) = (src0(i*60 + 10*3-1, i*60 + 10*2+2) + src0(i*60 + 10*6-1, i*60 + 10*5+2)) >> 1; // (V0 + V1) / 2
	}

	// next 8 tuples
	for(int i = 0;i < 4;i++) {
		luma(4*16+i*16 + 8*1-1, 4*16+i*16 + 8*0) = src1(i*60 + 10*1-1, i*60 + 10*0+2); // Y0
		luma(4*16+i*16 + 8*2-1, 4*16+i*16 + 8*1) = src1(i*60 + 10*4-1, i*60 + 10*3+2); // Y1
		chroma(4*16+i*16 + 8*1-1, 4*16+i*16 + 8*0) = (src1(i*60 + 10*2-1, i*60 + 10*1+2) + src1(i*60 + 10*5-1, i*60 + 10*4+2)) >> 1; // (U0 + U1) / 2
		chroma(4*16+i*16 + 8*2-1, 4*16+i*16 + 8*1) = (src1(i*60 + 10*3-1, i*60 + 10*2+2) + src1(i*60 + 10*6-1, i*60 + 10*5+2)) >> 1; // (V0 + V1) / 2
	}
}

void src_strm_to_luma_chroma(
	src_pixel_stream_t& strmSrc,
	dst_pixel_stream_t& strmLuma,
	dst_pixel_stream_t& strmChroma,
	ap_uint<32> nWidth_16,
	ap_uint<32> nHeight) {
	src_pixel_t src[2];
	dst_pixel_t luma[MAX_WIDTH >> DST_BITSHIFT], chroma[MAX_WIDTH >> DST_BITSHIFT];

loop_height:
	for (ap_uint<32> i = 0; i < nHeight; i++) {
loop_width:
		for (ap_uint<32> j = 0; j < nWidth_16; j++) {
#pragma HLS PIPELINE II=2
			strmSrc >> src[0];
			strmSrc >> src[1];

			src_to_luma_chroma(src[0], src[1], luma[j], chroma[j]);
		}

loop_width_gen:
		for (ap_uint<32> j = 0; j < nWidth_16; j++) {
			strmLuma << luma[j];
			strmChroma << chroma[j];
		}
	}
}

void src_strm_to_mem_v0(
	dst_pixel_stream_t& strmDst,
	page_info_stream_t& strmPageInfo,
	hls::burst_maxi<dst_pixel_t> pDst,
	ap_uint<32> nDstStride,
	ap_uint<32> nWidth,
	ap_uint<32> nHeight,
	ap_uint<32> nFormat) {
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

			pDst.write_request(nOffset >> DST_BITSHIFT, nBurst >> DST_BITSHIFT);
loop_batch:
			for(ap_uint<32> j = 0;j < (nBurst >> DST_BITSHIFT);j++) {
				pDst.write(strmDst.read());
			}
			pDst.write_response();

			nBytesLeft -= nBurst;
			nSize -= nBurst;
			nOffset += nBurst;

#ifndef __SYNTHESIS__
			std::cout << "(nBytesLeft=" << nBytesLeft << ", nSize=" << nSize << ", nOffset=" << nOffset << ")";
#endif

		} while(nBytesLeft > 0);

		nSize -= (nDstStride - nWidth);
		nOffset += (nDstStride - nWidth);

#ifndef __SYNTHESIS__
		std::cout << "(nSize=" << nSize << ", nOffset=" << nOffset << ")" << std::endl;
#endif
	}
#endif

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ": ----" << std::endl;
#endif
}

void z_frmbuf_writer(
	src_axi_stream_t& strmSrcAxi,

	hls::burst_maxi<desc_item_t> pDescLuma,
	ap_uint<32> nDescLumaCount,
	hls::burst_maxi<dst_pixel_t> pDstLuma,
	ap_uint<32> nDstLumaStride,

	hls::burst_maxi<desc_item_t> pDescChroma,
	ap_uint<32> nDescChromaCount,
	hls::burst_maxi<dst_pixel_t> pDstChroma,
	ap_uint<32> nDstChromaStride,

	ap_uint<32> nWidth,
	ap_uint<32> nHeight,
	ap_uint<32> nFormat,
	ap_uint<32>& nAxisRes) {
#pragma HLS INTERFACE axis port=strmSrcAxi register_mode=both
#pragma HLS INTERFACE m_axi port=pDescLuma offset=slave bundle=desc_luma depth=0x1000
#pragma HLS INTERFACE m_axi port=pDstLuma offset=slave bundle=luma depth=0x1000
#pragma HLS INTERFACE m_axi port=pDescChroma offset=slave bundle=desc_chroma depth=0x1000
#pragma HLS INTERFACE m_axi port=pDstChroma offset=slave bundle=chroma depth=0x1000
#pragma HLS INTERFACE s_axilite port=nDescLumaCount
#pragma HLS INTERFACE s_axilite port=nDstLumaStride
#pragma HLS INTERFACE s_axilite port=nDescChromaCount
#pragma HLS INTERFACE s_axilite port=nDstChromaStride
#pragma HLS INTERFACE s_axilite port=nWidth
#pragma HLS INTERFACE s_axilite port=nHeight
#pragma HLS INTERFACE s_axilite port=nFormat
#pragma HLS INTERFACE s_axilite port=nAxisRes
#pragma HLS INTERFACE s_axilite port=return

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << std::endl;
	std::cout << "MAX_WIDTH=" << MAX_WIDTH << std::endl;
	std::cout << "MAX_HEIGHT=" << MAX_HEIGHT << std::endl;
	std::cout << "SRC_WORD_WIDTH=" << SRC_WORD_WIDTH << std::endl;
	std::cout << "DST_PTR_WIDTH=" << DST_PTR_WIDTH << std::endl;
	std::cout << "DST_BITSHIFT=" << DST_BITSHIFT << std::endl;

	std::cout << "nDescLumaCount=" << nDescLumaCount << std::endl;
	std::cout << "nDstLumaStride=" << nDstLumaStride << std::endl;
	std::cout << "nDescChromaCount=" << nDescChromaCount << std::endl;
	std::cout << "nDstChromaStride=" << nDstChromaStride << std::endl;
	std::cout << "nWidth=" << nWidth << std::endl;
	std::cout << "nHeight=" << nHeight << std::endl;
	std::cout << "nFormat=" << nFormat << std::endl;
#endif

	ap_uint<32> nWidth_8 = (nWidth >> 3);
	ap_uint<32> nWidth_16 = (nWidth >> 4);

	src_pixel_stream_t strmSrc("strmSrc");
	hls::stream<ap_uint<32> > strmAxisRes("strmAxisRes");
	page_info_stream_t strmLumaPageInfo("strmLumaPageInfo");
	page_info_stream_t strmChromaPageInfo("strmChromaPageInfo");
	dst_pixel_stream_t strmLuma("strmLuma");
	dst_pixel_stream_t strmChroma("strmChroma");

#pragma HLS DATAFLOW
	src_axis_to_strm(strmSrcAxi, strmSrc, nWidth_8, nHeight, strmAxisRes);
	src_strm_to_luma_chroma(strmSrc, strmLuma, strmChroma, nWidth_16, nHeight);
	desc_item_to_strm(pDescLuma, nDescLumaCount, strmLumaPageInfo);
	desc_item_to_strm(pDescChroma, nDescChromaCount, strmChromaPageInfo);
	src_strm_to_mem_v0(strmLuma, strmLumaPageInfo, pDstLuma, nWidth, nHeight, nDstLumaStride, nFormat);
	src_strm_to_mem_v0(strmChroma, strmChromaPageInfo, pDstChroma, nWidth, nHeight, nDstLumaStride, nFormat);
	strmAxisRes.read(nAxisRes);

#ifndef __SYNTHESIS__
	std::cout << "nAxisRes=" << nAxisRes << std::endl;
	std::cout << __FUNCTION__ << "... DONE" << std::endl;
#endif
}
