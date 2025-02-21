#include <stdio.h>

#ifndef __SYNTHESIS__
#include <iostream>
#include <iomanip>
#include <cstdlib>
#endif

#include "lbl_wr.h"

#define ERROR_IO_EOL_EARLY  (1 << 0)
#define ERROR_IO_EOL_LATE   (1 << 1)
#define ERROR_IO_SOF_EARLY  (1 << 0)
#define ERROR_IO_SOF_LATE   (1 << 1)

void axis_to_src_strm(
	axis_pixel_stream_t& s_axis,
	src_pixel_stream_t& strm0,
	ap_uint<32> nWidth_2,
	ap_uint<32> nHeight)
{
    ap_uint<32> res = 0;
	axis_pixel_t axi;

	bool sof = 0;
loop_wait_for_start:
	while (!sof) {
#pragma HLS PIPELINE II=1
#pragma HLS loop_tripcount avg=0 max=0
		s_axis.read(axi);
		sof = axi.user.to_int();
	}

loop_height:
	for (ap_uint<32> i = 0; i < nHeight; i++) {
#pragma HLS loop_tripcount avg=0 max=0
		bool eol = 0;
loop_width:
		for (ap_uint<32> j = 0; j < nWidth_2; j++) {
#pragma HLS loop_flatten off
#pragma HLS pipeline
#pragma HLS loop_tripcount avg=0 max=0
			if (sof || eol) {
				sof = 0;
				eol = axi.last.to_int();
			} else {
				// If we didn't reach EOL, then read the next pixel
				s_axis.read(axi);
				eol = axi.last.to_int();
				bool user = axi.user.to_int();
				if(user) {
					res |= ERROR_IO_SOF_EARLY;
				}
			}
			if (eol && (j != nWidth_2-1)) {
				res |= ERROR_IO_EOL_EARLY;
			}

			strm0.write(axi.data);
		}

loop_wait_for_eol:
		while (!eol) {
#pragma HLS pipeline II=1
#pragma HLS loop_tripcount avg=0 max=0
			// Keep reading until we get to EOL
			s_axis.read(axi);
			eol = axi.last.to_int();
			res |= ERROR_IO_EOL_LATE;
		}
	}
}

void src_strm_sink(
	src_pixel_stream_t& strm0,
	ap_uint<32> nWidth_2,
	ap_uint<32> nHeight) {
	src_pixel_t src;

loop_height:
	for (ap_uint<32> i = 0; i < nHeight;i++) {

loop_width:
		for (ap_uint<32> j = 0;j < nWidth_2;j++) {
			strm0 >> src;
		}
	}
}

void dst_strm_sink(
	dst_pixel_stream_t& strm1,
	ap_uint<32> nWidthPC,
	ap_uint<32> nHeight) {
	dst_pixel_t dst;

loop_height:
	for (ap_uint<32> i = 0; i < nHeight;i++) {

loop_width:
		for (ap_uint<32> j = 0;j < nWidthPC;j++) {
			strm1 >> dst;
		}
	}
}

void src_strm_to_dst_strm(
	src_pixel_stream_t& strm0,
	dst_pixel_stream_t& strm1,
	dst_pixel_stream_t& strm2,
	ap_uint<32> nWidthPC,
	ap_uint<32> nHeight) {
	src_pixel_t src;
	dst_pixel_t luma;
	dst_pixel_t chroma;

loop_height:
	for (ap_uint<32> i = 0; i < nHeight;i++) {

loop_width:
		for (ap_uint<32> j = 0;j < nWidthPC;j++) {
#pragma HLS PIPELINE II=8 rewind

			for(int t = 0;t < DST_TO_SRC_NPPC;t++) {
#pragma HLS UNROLL factor=8
				strm0 >> src;

				luma.range(t*16 + 8*1-1, t*16 + 8*0) = src.range(8*1-1, 8*0); // Y0
				luma.range(t*16 + 8*2-1, t*16 + 8*1) = src.range(8*4-1, 8*3); // Y1

#if 1
				chroma.range(t*16 + 8*1-1, t*16 + 8*0) =
					(src.range(8*2-1, 8*1) + src.range(8*5-1, 8*4)) >> 1; // (U0 + U1) / 2
				chroma.range(t*16 + 8*2-1, t*16 + 8*1) =
					(src.range(8*3-1, 8*2) + src.range(8*6-1, 8*5)) >> 1; // (V0 + V1) / 2
#else
				chroma.range(t*16 + 8*1-1, t*16 + 8*0) = src.range(8*2-1, 8*1); // U0
				chroma.range(t*16 + 8*2-1, t*16 + 8*1) = src.range(8*3-1, 8*2); // V0
#endif
			}

			strm1 << luma;
			strm2 << chroma;
		}
	}
}

inline void mem_write_request(
	hls::burst_maxi<dst_pixel_t>& pDstY0,
	hls::burst_maxi<dst_pixel_t>& pDstUV0,
	hls::burst_maxi<dst_pixel_t>& pDstY1,
	hls::burst_maxi<dst_pixel_t>& pDstUV1,
	ap_uint<32> nPos_dstY0,
	ap_uint<32> nPos_dstUV0,
	ap_uint<32> nPos_dstY1,
	ap_uint<32> nPos_dstUV1,
	ap_uint<32> nWidthPC,
	bool bEven,
	int nFormat) {
	if(bEven) {
		pDstY0.write_request(nPos_dstY0, nWidthPC);
		pDstUV0.write_request(nPos_dstUV0, nWidthPC);
	} else {
		pDstY1.write_request(nPos_dstY1, nWidthPC);
		switch(nFormat) {
		case FORMAT_NV16:
			pDstUV1.write_request(nPos_dstUV1, nWidthPC);
			break;
		}
	}
}

inline void mem_write_response(
	hls::burst_maxi<dst_pixel_t>& pDstY0,
	hls::burst_maxi<dst_pixel_t>& pDstUV0,
	hls::burst_maxi<dst_pixel_t>& pDstY1,
	hls::burst_maxi<dst_pixel_t>& pDstUV1,
	bool bEven,
	int nFormat) {
	if(bEven) {
		pDstY0.write_response();
		pDstUV0.write_response();
	} else {
		pDstY1.write_response();
		switch(nFormat) {
		case FORMAT_NV16:
			pDstUV1.write_response();
			break;
		}
	}
}

inline void src_to_luma_chroma(const src_pixel_t& src, int t, dst_pixel_t& luma, dst_pixel_t& chroma) {
	luma(t*16 + 8*1-1, t*16 + 8*0) = src(8*1-1, 8*0); // Y0
	luma(t*16 + 8*2-1, t*16 + 8*1) = src(8*4-1, 8*3); // Y1
	chroma(t*16 + 8*1-1, t*16 + 8*0) = (src(8*2-1, 8*1) + src(8*5-1, 8*4)) >> 1; // (U0 + U1) / 2
	chroma(t*16 + 8*2-1, t*16 + 8*1) = (src(8*3-1, 8*2) + src(8*6-1, 8*5)) >> 1; // (V0 + V1) / 2
}

inline void src_strm_to_luma_chroma(src_pixel_stream_t& strm0, dst_pixel_t& luma, dst_pixel_t& chroma) {
#pragma HLS PIPELINE II=8

	src_pixel_t src[DST_TO_SRC_NPPC];

	strm0 >> src[0];
	for(int t = 0;t < DST_TO_SRC_NPPC - 1;t++) {
#pragma HLS UNROLL factor=7 skip_exit_check
		strm0 >> src[t + 1];
		src_to_luma_chroma(src[t], t, luma, chroma);
	}
	src_to_luma_chroma(src[DST_TO_SRC_NPPC - 1], DST_TO_SRC_NPPC - 1, luma, chroma);
}

inline void luma_chroma_to_mem(
	const dst_pixel_t& luma,
	const dst_pixel_t& chroma,
	hls::burst_maxi<dst_pixel_t>& pDstY0,
	hls::burst_maxi<dst_pixel_t>& pDstUV0,
	hls::burst_maxi<dst_pixel_t>& pDstY1,
	hls::burst_maxi<dst_pixel_t>& pDstUV1,
	bool bEven,
	int nFormat) {
	if(bEven) {
		pDstY0.write(luma);
		pDstUV0.write(chroma);
	} else {
		pDstY1.write(luma);
		switch(nFormat) {
		case FORMAT_NV16:
			pDstUV1.write(chroma);
			break;
		}
	}
}

void src_strm_to_mem_v1(
	src_pixel_stream_t& strm0,
	hls::burst_maxi<dst_pixel_t>& pDstY0,
	hls::burst_maxi<dst_pixel_t>& pDstUV0,
	hls::burst_maxi<dst_pixel_t>& pDstY1,
	hls::burst_maxi<dst_pixel_t>& pDstUV1,
	ap_uint<32> nWidthPC,
	ap_uint<32> nHeight,
	ap_uint<32> nDstStrideY_PC,
	ap_uint<32> nDstStrideUV_PC,
	ap_uint<32> nFormat) {
	dst_pixel_t luma[MAX_WIDTH >> DST_BITSHIFT];
	dst_pixel_t chroma[MAX_WIDTH >> DST_BITSHIFT];
	ap_uint<32> nPos_dstY0 = 0, nPos_dstUV0 = 0;
	ap_uint<32> nPos_dstY1 = 0, nPos_dstUV1 = 0;
	bool bEven = true;
	ap_uint<32> i, j;

loop_height:
	for(i = 0; i < nHeight;i++) {
		mem_write_request(pDstY0, pDstUV0, pDstY1, pDstUV1,
			nPos_dstY0, nPos_dstUV0, nPos_dstY1, nPos_dstUV1, nWidthPC, bEven, nFormat);
		src_strm_to_luma_chroma(strm0, luma[0], chroma[0]);

loop_width:
		for(j = 0;j < nWidthPC - 1;j++) {
			src_strm_to_luma_chroma(strm0, luma[j + 1], chroma[j + 1]);
			luma_chroma_to_mem(luma[j], chroma[j], pDstY0, pDstUV0, pDstY1, pDstUV1, bEven, nFormat);
		}
		luma_chroma_to_mem(luma[j], chroma[j], pDstY0, pDstUV0, pDstY1, pDstUV1, bEven, nFormat);

		mem_write_response(pDstY0, pDstUV0, pDstY1, pDstUV1, bEven, nFormat);

		if(bEven) {
			nPos_dstY0 += nDstStrideY_PC;
			nPos_dstUV0 += nDstStrideUV_PC;
		} else {
			nPos_dstY1 += nDstStrideY_PC;
			switch(nFormat) {
			case FORMAT_NV16:
				nPos_dstUV1 += nDstStrideUV_PC;
				break;
			}
		}

		bEven = bEven ? false : true;
	}
}

void src_strm_to_mem_v3(
	src_pixel_stream_t& strm0,
	hls::burst_maxi<dst_pixel_t>& pDstY0,
	hls::burst_maxi<dst_pixel_t>& pDstUV0,
	hls::burst_maxi<dst_pixel_t>& pDstY1,
	hls::burst_maxi<dst_pixel_t>& pDstUV1,
	ap_uint<32> nWidthPC,
	ap_uint<32> nHeight,
	ap_uint<32> nDstStrideY_PC,
	ap_uint<32> nDstStrideUV_PC,
	ap_uint<32> nFormat) {
	dst_pixel_t luma;
	dst_pixel_t chroma;
	ap_uint<32> nPos_dstY0 = 0, nPos_dstUV0 = 0;
	ap_uint<32> nPos_dstY1 = 0, nPos_dstUV1 = 0;
	bool bEven = true;
	ap_uint<32> i, j;

loop_height:
	for(i = 0; i < nHeight;i++) {
		mem_write_request(pDstY0, pDstUV0, pDstY1, pDstUV1,
			nPos_dstY0, nPos_dstUV0, nPos_dstY1, nPos_dstUV1, nWidthPC, bEven, nFormat);

loop_width:
		for(j = 0;j < nWidthPC;j++) {
			src_strm_to_luma_chroma(strm0, luma, chroma);
			luma_chroma_to_mem(luma, chroma, pDstY0, pDstUV0, pDstY1, pDstUV1, bEven, nFormat);
		}

		mem_write_response(pDstY0, pDstUV0, pDstY1, pDstUV1, bEven, nFormat);

		if(bEven) {
			nPos_dstY0 += nDstStrideY_PC;
			nPos_dstUV0 += nDstStrideUV_PC;
		} else {
			nPos_dstY1 += nDstStrideY_PC;
			switch(nFormat) {
			case FORMAT_NV16:
				nPos_dstUV1 += nDstStrideUV_PC;
				break;
			}
		}

		bEven = bEven ? false : true;
	}
}

void luma_chroma_dst_strm_to_mem_even(
	dst_pixel_stream_t& strm0,
	dst_pixel_stream_t& strm1,
	hls::burst_maxi<dst_pixel_t>& pDstY,
	hls::burst_maxi<dst_pixel_t>& pDstUV,
	ap_uint<32> nWidthPC,
	ap_uint<32> nHeight_2,
	ap_uint<32> nDstStrideY_PC,
	ap_uint<32> nDstStrideUV_PC) {
	dst_pixel_t luma;
	dst_pixel_t chroma;
	ap_uint<32> nPos_dstY = 0, nPos_dstUV = 0;
	ap_uint<32> i, j;

loop_height:
	for(i = 0; i < nHeight_2;i++) {
		pDstY.write_request(nPos_dstY, nWidthPC);
		pDstUV.write_request(nPos_dstUV, nWidthPC);

loop_width:
		for(j = 0;j < nWidthPC;j++) {
			strm0 >> luma;
			strm1 >> chroma;

			pDstY.write(luma);
			pDstUV.write(chroma);
		}
		pDstY.write_response();
		pDstUV.write_response();

		nPos_dstY += nDstStrideY_PC;
		nPos_dstUV += nDstStrideUV_PC;
	}
}

void luma_chroma_dst_strm_to_mem_odd(
	dst_pixel_stream_t& strm0,
	dst_pixel_stream_t& strm1,
	hls::burst_maxi<dst_pixel_t>& pDstY,
	hls::burst_maxi<dst_pixel_t>& pDstUV,
	ap_uint<32> nWidthPC,
	ap_uint<32> nHeight_2,
	ap_uint<32> nDstStrideY_PC,
	ap_uint<32> nDstStrideUV_PC,
	ap_uint<32> nFormat) {
	dst_pixel_t luma;
	dst_pixel_t chroma;
	ap_uint<32> nPos_dstY = 0, nPos_dstUV = 0;
	ap_uint<32> i, j;

loop_height:
	for(i = 0; i < nHeight_2;i++) {
		pDstY.write_request(nPos_dstY, nWidthPC);
		switch(nFormat) {
		case FORMAT_NV16:
			pDstUV.write_request(nPos_dstUV, nWidthPC);
			break;
		}

loop_width:
		for(j = 0;j < nWidthPC - 1;j++) {
			strm0 >> luma;
			strm1 >> chroma;

			pDstY.write(luma);
			switch(nFormat) {
			case FORMAT_NV16:
				pDstUV.write(chroma);
				break;
			}
		}
		pDstY.write_response();
		switch(nFormat) {
		case FORMAT_NV16:
			pDstUV.write_response();
			break;
		}

		nPos_dstY += nDstStrideY_PC;
		nPos_dstUV += nDstStrideUV_PC;
	}
}

void src_strm_to_luma_chroma_dst_strm(
	src_pixel_stream_t& strm0,
	dst_pixel_stream_t& strm1,
	dst_pixel_stream_t& strm2,
	dst_pixel_stream_t& strm3,
	dst_pixel_stream_t& strm4,
	ap_uint<32> nWidthPC,
	ap_uint<32> nHeight) {
	dst_pixel_t luma;
	dst_pixel_t chroma;
	ap_uint<32> i, j;
	bool bEven = true;

loop_height:
	for(i = 0; i < nHeight;i++) {
loop_width:
		for(j = 0;j < nWidthPC;j++) {
			src_strm_to_luma_chroma(strm0, luma, chroma);

			if(bEven) {
				strm1 << luma;
				strm2 << chroma;
			} else {
				strm3 << luma;
				strm4 << chroma;
			}
		}

		bEven = !bEven;
	}
}

void src_strm_to_mem_v2(
	src_pixel_stream_t& strm0,
	hls::burst_maxi<dst_pixel_t>& pDstY0,
	hls::burst_maxi<dst_pixel_t>& pDstUV0,
	hls::burst_maxi<dst_pixel_t>& pDstY1,
	hls::burst_maxi<dst_pixel_t>& pDstUV1,
	ap_uint<32> nWidthPC,
	ap_uint<32> nHeight,
	ap_uint<32> nDstStrideY_PC,
	ap_uint<32> nDstStrideUV_PC,
	ap_uint<32> nFormat) {
	ap_uint<32> nHeight_2 = nHeight >> 1;
	dst_pixel_stream_t strm1("strm1");
	dst_pixel_stream_t strm2("strm2");
	dst_pixel_stream_t strm3("strm3");
	dst_pixel_stream_t strm4("strm4");

#pragma HLS DATAFLOW
	src_strm_to_luma_chroma_dst_strm(strm0, strm1, strm2, strm3, strm4, nWidthPC, nHeight);
	luma_chroma_dst_strm_to_mem_even(strm1, strm2, pDstY0, pDstUV0, nWidthPC, nHeight_2, nDstStrideY_PC, nDstStrideUV_PC);
	luma_chroma_dst_strm_to_mem_odd(strm3, strm4, pDstY1, pDstUV1, nWidthPC, nHeight_2, nDstStrideY_PC, nDstStrideUV_PC, nFormat);
}

void dst_strm_to_mem_nv16(
	dst_pixel_stream_t& strm1,
	dst_pixel_stream_t& strm2,
	ap_uint<DST_PTR_WIDTH>* pDstY0,
	ap_uint<DST_PTR_WIDTH>* pDstY1,
	ap_uint<DST_PTR_WIDTH>* pDstUV0,
	ap_uint<DST_PTR_WIDTH>* pDstUV1,
	ap_uint<32> nWidthPC,
	ap_uint<32> nHeight_2,
	ap_uint<32> nDstYSkip,
	ap_uint<32> nDstUVSkip) {
	dst_pixel_t luma, chroma;
	ap_uint<32> nPos_dstY0 = 0, nPos_dstY1 = 0, nPos_dstUV0 = 0, nPos_dstUV1 = 0;

loop_height:
	for (ap_uint<32> i = 0; i < nHeight_2;i++, nPos_dstY0 += nDstYSkip, nPos_dstY1 += nDstYSkip, nPos_dstUV0 += nDstUVSkip, nPos_dstUV1 += nDstUVSkip) {

loop_width_even:
		for (ap_uint<32> j = 0;j < nWidthPC;j++, nPos_dstY0++, nPos_dstUV0++) {
#pragma HLS PIPELINE II=2

			strm1 >> luma;
			strm2 >> chroma;

			pDstY0[nPos_dstY0] = luma;
			pDstUV0[nPos_dstUV0] = chroma;
		}

loop_width_odd:
		for (ap_uint<32> j = 0;j < nWidthPC;j++, nPos_dstY1++, nPos_dstUV1++) {
#pragma HLS PIPELINE II=2

			strm1 >> luma;
			strm2 >> chroma;

			pDstY1[nPos_dstY1] = luma;
			pDstUV1[nPos_dstUV1] = chroma;
		}
	}
}

void dst_strm_to_mem_nv12(
	dst_pixel_stream_t& strm1,
	dst_pixel_stream_t& strm2,
	ap_uint<DST_PTR_WIDTH>* pDstY0,
	ap_uint<DST_PTR_WIDTH>* pDstY1,
	ap_uint<DST_PTR_WIDTH>* pDstUV0,
	ap_uint<32> nWidthPC,
	ap_uint<32> nHeight_2,
	ap_uint<32> nDstYSkip,
	ap_uint<32> nDstUVSkip) {
	dst_pixel_t luma, chroma;
	ap_uint<32> nPos_dstY0 = 0, nPos_dstY1 = 0, nPos_dstUV0 = 0;

loop_height:
	for (ap_uint<32> i = 0; i < nHeight_2;i++, nPos_dstY0 += nDstYSkip, nPos_dstY1 += nDstYSkip, nPos_dstUV0 += nDstUVSkip) {

loop_width_even:
		for (ap_uint<32> j = 0;j < nWidthPC;j++, nPos_dstY0++, nPos_dstUV0++) {
#pragma HLS PIPELINE II=2

			strm1 >> luma;
			strm2 >> chroma;

			pDstY0[nPos_dstY0] = luma;
			pDstUV0[nPos_dstUV0] = chroma;
		}

loop_width_odd:
		for (ap_uint<32> j = 0;j < nWidthPC;j++, nPos_dstY1++) {
#pragma HLS PIPELINE II=2

			strm1 >> luma;
			strm2 >> chroma;

			pDstY1[nPos_dstY1] = luma;
		}
	}
}

void dst_strm_to_mem(
	dst_pixel_stream_t& strm1,
	dst_pixel_stream_t& strm2,
	ap_uint<DST_PTR_WIDTH>* pDstY0,
	ap_uint<DST_PTR_WIDTH>* pDstY1,
	ap_uint<DST_PTR_WIDTH>* pDstUV0,
	ap_uint<DST_PTR_WIDTH>* pDstUV1,
	ap_uint<32> nWidthPC,
	ap_uint<32> nHeight_2,
	ap_uint<32> nDstYSkip,
	ap_uint<32> nDstUVSkip,
	ap_uint<32> nControl) {
	switch((nControl >> CTRL_FORMAT_BITSHIFT) & CTRL_FORMAT_MASK) {
	case FORMAT_NV12:
		dst_strm_to_mem_nv12(strm1, strm2, pDstY0, pDstY1, pDstUV0,
			nWidthPC, nHeight_2, nDstYSkip, nDstUVSkip);
		break;

	case FORMAT_NV16:
		dst_strm_to_mem_nv16(strm1, strm2, pDstY0, pDstY1, pDstUV0, pDstUV1,
			nWidthPC, nHeight_2, nDstYSkip, nDstUVSkip);
		break;

	default:
		break;
	}
}

void lbl_wr(
	axis_pixel_stream_t& s_axis,
	hls::burst_maxi<dst_pixel_t> pDstY0,
	hls::burst_maxi<dst_pixel_t> pDstUV0,
	hls::burst_maxi<dst_pixel_t> pDstY1,
	hls::burst_maxi<dst_pixel_t> pDstUV1,
	ap_uint<32> nDstStrideY,
	ap_uint<32> nDstStrideUV,
	ap_uint<32> nWidth,
	ap_uint<32> nHeight,
	ap_uint<32> nControl) {
#pragma HLS INTERFACE axis port=s_axis register_mode=both
#pragma HLS INTERFACE m_axi port=pDstY0 depth=48 offset=slave bundle=mm_video0
#pragma HLS INTERFACE m_axi port=pDstUV0 depth=48 offset=slave bundle=mm_video1
#pragma HLS INTERFACE m_axi port=pDstY1 depth=48 offset=slave bundle=mm_video2
#pragma HLS INTERFACE m_axi port=pDstUV1 depth=48 offset=slave bundle=mm_video3
#pragma HLS INTERFACE s_axilite port=nDstStrideY
#pragma HLS INTERFACE s_axilite port=nDstStrideUV
#pragma HLS INTERFACE s_axilite port=nWidth
#pragma HLS INTERFACE s_axilite port=nHeight
#pragma HLS INTERFACE s_axilite port=nControl
#pragma HLS INTERFACE s_axilite port=return

	ap_uint<32> nWidthPC = nWidth >> DST_BITSHIFT;
	ap_uint<32> nWidth_2 = (nWidth >> 1);
	ap_uint<32> nHeight_2 = (nHeight >> 1);
	ap_uint<32> nDstStrideY_PC = nDstStrideY >> DST_BITSHIFT;
	ap_uint<32> nDstStrideUV_PC = nDstStrideUV >> DST_BITSHIFT;
	ap_uint<32> nDstYSkip = nDstStrideY_PC - nWidthPC;
	ap_uint<32> nDstUVSkip = nDstStrideUV_PC - nWidthPC;
	ap_uint<32> nFormat = (nControl >> CTRL_FORMAT_BITSHIFT) & CTRL_FORMAT_MASK;

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << std::endl;
	std::cout << "SRC_WORD_WIDTH=" << SRC_WORD_WIDTH << std::endl;
	std::cout << "DST_PTR_WIDTH=" << DST_PTR_WIDTH << std::endl;
	std::cout << "SRC_BITSHIFT=" << SRC_BITSHIFT << std::endl;
	std::cout << "DST_BITSHIFT=" << DST_BITSHIFT << std::endl;
	std::cout << "DST_TO_SRC_NPPC=" << DST_TO_SRC_NPPC << std::endl;
	std::cout << nWidth << 'x' << nHeight << ',' << nDstStrideY << ',' << nDstStrideUV << std::endl;
	std::cout << nWidthPC << 'x' << nHeight_2 << ',' << nDstYSkip << ',' << nDstUVSkip << std::endl;
#endif

	src_pixel_stream_t strm0("strm0");
	dst_pixel_stream_t strm1("strm1");
	dst_pixel_stream_t strm2("strm2");

#pragma HLS STREAM variable=strm0
#pragma HLS STREAM variable=strm1
#pragma HLS STREAM variable=strm2

#pragma HLS DATAFLOW
	axis_to_src_strm(s_axis, strm0, nWidth_2, nHeight);

#if 0
	src_strm_to_dst_strm(strm0, strm1, strm2, nWidthPC, nHeight);

#if 1
	dst_strm_to_mem(strm1, strm2, pDstY0, pDstY1, pDstUV0, pDstUV1,
		nWidthPC, nHeight_2, nDstYSkip, nDstUVSkip, nControl);
#else
	dst_strm_sink(strm1, nWidthPC, nHeight);
	dst_strm_sink(strm2, nWidthPC, nHeight);
#endif
#endif

#if 0
	src_strm_to_mem_v1(strm0, pDstY0, pDstUV0, pDstY1, pDstUV1, nWidthPC, nHeight, nDstStrideY_PC, nDstStrideUV_PC, nFormat);
#endif

#if 0
	src_strm_to_mem_v2(strm0, pDstY0, pDstUV0, pDstY1, pDstUV1, nWidthPC, nHeight, nDstStrideY_PC, nDstStrideUV_PC, nFormat);
#endif

#if 1
	src_strm_to_mem_v3(strm0, pDstY0, pDstUV0, pDstY1, pDstUV1, nWidthPC, nHeight, nDstStrideY_PC, nDstStrideUV_PC, nFormat);
#endif
}
