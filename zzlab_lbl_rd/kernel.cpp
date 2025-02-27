#include <stdio.h>

#ifndef __SYNTHESIS__
#include <iostream>
#include <iomanip>
#include <cstdlib>
#endif

#include "lbl_rd.h"

void to_pixels_stream(
	ap_uint<SRC_PTR_WIDTH>* pSrcY0,
	ap_uint<SRC_PTR_WIDTH>* pSrcY1,
	ap_uint<SRC_PTR_WIDTH>* pSrcUV0,
	ap_uint<SRC_PTR_WIDTH>* pSrcUV1,
	src_pixel_stream_t& strm0,
	src_pixel_stream_t& strm1,
	ap_uint<32> nWidthPC,
	ap_uint<32> nHeight_2,
	ap_uint<32> nSrcYSkip,
	ap_uint<32> nSrcUVSkip) {
	ap_uint<32> nPos_srcY0 = 0, nPos_srcY1 = 0, nPos_srcUV0 = 0, nPos_srcUV1 = 0;

loop_height:
	for (ap_uint<32> i = 0; i < nHeight_2;i++, nPos_srcY0 += nSrcYSkip, nPos_srcY1 += nSrcYSkip, nPos_srcUV0 += nSrcUVSkip, nPos_srcUV1 += nSrcUVSkip) {

loop_width_even:
		for (ap_uint<32> j = 0;j < nWidthPC;j++, nPos_srcY0++, nPos_srcUV0++) {
#pragma HLS PIPELINE II=2

			strm0 << pSrcY0[nPos_srcY0];
			strm1 << pSrcUV0[nPos_srcUV0];
		}

loop_width_odd:
		for (ap_uint<32> j = 0;j < nWidthPC;j++, nPos_srcY1++, nPos_srcUV1++) {
#pragma HLS PIPELINE II=2

			strm0 << pSrcY1[nPos_srcY1];
			strm1 << pSrcUV1[nPos_srcUV1];
		}
	}
}

void to_axis(
	src_pixel_stream_t& strm0,
	src_pixel_stream_t& strm1,
	axis_pixel_stream_t& s_axis,
	ap_uint<32> nWidthPC,
	ap_uint<32> nHeight) {
	src_pixel_t luma, chroma;
	axis_pixel_t axi[SRC_TO_DST_NPPC];

loop_height:
	for (ap_uint<32> i = 0; i < nHeight; i++) {
loop_width:
		for (ap_uint<32> j = 0; j < nWidthPC; j++) {
#pragma HLS PIPELINE II=8
			strm0.read(luma);
			strm1.read(chroma);

			for(int t = 0;t < SRC_TO_DST_NPPC;t++) {
#pragma HLS UNROLL
				axi[t].keep = -1;
				axi[t].strb = -1;
				axi[t].id = 0;
				axi[t].dest = 0;
				axi[t].user = (t == 0 && i == 0 && j == 0);
				axi[t].last = (t == (SRC_TO_DST_NPPC - 1) && j == (nWidthPC - 1));

				axi[t].data.range(8*1-1, 8*0) = luma.range(t*16 + 8*1-1, t*16 + 8*0); // Y0
				axi[t].data.range(8*2-1, 8*1) = chroma.range(t*16 + 8*1-1, t*16 + 8*0); // U0
				axi[t].data.range(8*3-1, 8*2) = chroma.range(t*16 + 8*2-1, t*16 + 8*1); // V0
				axi[t].data.range(8*4-1, 8*3) = luma.range(t*16 + 8*2-1, t*16 + 8*1); // Y1
				axi[t].data.range(8*5-1, 8*4) = chroma.range(t*16 + 8*1-1, t*16 + 8*0); // U1
				axi[t].data.range(8*6-1, 8*5) = chroma.range(t*16 + 8*2-1, t*16 + 8*1); // V1
			}

			for(int t = 0;t < SRC_TO_DST_NPPC;t++) {
#pragma HLS UNROLL
				s_axis.write(axi[t]);
			}
		}
	}
}

inline void mem_to_luma_chroma(
	hls::burst_maxi<src_pixel_t> pSrcY0,
	hls::burst_maxi<src_pixel_t> pSrcY1,
	hls::burst_maxi<src_pixel_t> pSrcUV0,
	hls::burst_maxi<src_pixel_t> pSrcUV1,
	src_pixel_t& luma,
	src_pixel_t& chroma,
	bool bEven) {
	if(bEven) {
		luma = pSrcY0.read();
		chroma = pSrcUV0.read();
	} else {
		luma = pSrcY1.read();
		chroma = pSrcUV1.read();
	}
}

void luma_chroma_to_axi(
	const src_pixel_t& luma,
	const src_pixel_t& chroma,
	axis_pixel_t& axi,
	int t, int user, int last) {
	axi.keep = -1;
	axi.strb = -1;
	axi.id = 0;
	axi.dest = 0;
	axi.user = user;
	axi.last = last;

	axi.data(8*1-1, 8*0) = luma(t*16 + 8*1-1, t*16 + 8*0); // Y0
	axi.data(8*2-1, 8*1) = chroma(t*16 + 8*1-1, t*16 + 8*0); // U0
	axi.data(8*3-1, 8*2) = chroma(t*16 + 8*2-1, t*16 + 8*1); // V0
	axi.data(8*4-1, 8*3) = luma(t*16 + 8*2-1, t*16 + 8*1); // Y1
	axi.data(8*5-1, 8*4) = chroma(t*16 + 8*1-1, t*16 + 8*0); // U1
	axi.data(8*6-1, 8*5) = chroma(t*16 + 8*2-1, t*16 + 8*1); // V1
}

void luma_chroma_to_axis(
	const src_pixel_t& luma,
	const src_pixel_t& chroma,
	axis_pixel_stream_t& s_axis,
	int user, int last) {
#pragma HLS PIPELINE II=8

	axis_pixel_t axi[SRC_TO_DST_NPPC];

	luma_chroma_to_axi(luma, chroma, axi[0], 0, user, 0);

	for(int t = 0;t < SRC_TO_DST_NPPC - 1;t++) {
		luma_chroma_to_axi(luma, chroma, axi[t + 1], t + 1, 0, (t + 1 == (SRC_TO_DST_NPPC - 1) && last));

		s_axis.write(axi[t]);
	}

	s_axis.write(axi[SRC_TO_DST_NPPC - 1]);
}

void mem_to_axis_v0(
	ap_uint<SRC_PTR_WIDTH>* pSrcY0,
	ap_uint<SRC_PTR_WIDTH>* pSrcY1,
	ap_uint<SRC_PTR_WIDTH>* pSrcUV0,
	ap_uint<SRC_PTR_WIDTH>* pSrcUV1,
	axis_pixel_stream_t& s_axis,
	ap_uint<32> nWidthPC,
	ap_uint<32> nHeight,
	ap_uint<32> nSrcStrideYPC,
	ap_uint<32> nSrcStrideUVPC) {
	ap_uint<32> nHeight_2 = (nHeight >> 1);
	ap_uint<32> nSrcYSkip = nSrcStrideYPC - nWidthPC;
	ap_uint<32> nSrcUVSkip = nSrcStrideUVPC - nWidthPC;

	src_pixel_stream_t strm0("strm0");
	src_pixel_stream_t strm1("strm1");

#pragma HLS DATAFLOW
	to_pixels_stream(pSrcY0, pSrcY1, pSrcUV0, pSrcUV1,
		strm0, strm1, nWidthPC, nHeight_2,
		nSrcYSkip, nSrcUVSkip);
	to_axis(strm0, strm1, s_axis, nWidthPC, nHeight);
}

void mem_to_axis_v1(
	hls::burst_maxi<src_pixel_t>& pSrcY0,
	hls::burst_maxi<src_pixel_t>& pSrcY1,
	hls::burst_maxi<src_pixel_t>& pSrcUV0,
	hls::burst_maxi<src_pixel_t>& pSrcUV1,
	axis_pixel_stream_t& s_axis,
	ap_uint<32> nWidthPC,
	ap_uint<32> nHeight,
	ap_uint<32> nSrcStrideYPC,
	ap_uint<32> nSrcStrideUVPC) {
	src_pixel_t luma[MAX_WIDTH >> DST_BITSHIFT];
	src_pixel_t chroma[MAX_WIDTH >> DST_BITSHIFT];
	ap_uint<32> nPos_srcY0 = 0, nPos_srcUV0 = 0, nPos_srcY1 = 0, nPos_srcUV1 = 0;
	bool bEven = true;
	ap_uint<32> i, j;

loop_height:
	for(i = 0; i < nHeight; i++) {
		if(bEven) {
			pSrcY0.read_request(nPos_srcY0, nWidthPC);
			pSrcUV0.read_request(nPos_srcUV0, nWidthPC);

			luma[0] = pSrcY0.read();
			chroma[0] = pSrcUV0.read();
		} else {
			pSrcY1.read_request(nPos_srcY1, nWidthPC);
			pSrcUV1.read_request(nPos_srcUV1, nWidthPC);

			luma[0] = pSrcY1.read();
			chroma[0] = pSrcUV1.read();
		}

loop_width:
		for(j = 0; j < nWidthPC - 1; j++) {
			if(bEven) {
				luma[j+1] = pSrcY0.read();
				chroma[j+1] = pSrcUV0.read();
			} else {
				luma[j+1] = pSrcY1.read();
				chroma[j+1] = pSrcUV1.read();
			}

			luma_chroma_to_axis(luma[j], chroma[j], s_axis, i == 0 && j == 0, 0);
		}

		luma_chroma_to_axis(luma[j], chroma[j], s_axis, 0, 1);

		if(bEven) {
			nPos_srcY0 += nSrcStrideYPC;
			nPos_srcUV0 += nSrcStrideUVPC;
		} else {
			nPos_srcY1 += nSrcStrideYPC;
			nPos_srcUV1 += nSrcStrideUVPC;
		}

		bEven = bEven ? false : true;
	}
}

void mem_to_axis_v2(
	hls::burst_maxi<src_pixel_t>& pSrcY0,
	hls::burst_maxi<src_pixel_t>& pSrcY1,
	hls::burst_maxi<src_pixel_t>& pSrcUV0,
	hls::burst_maxi<src_pixel_t>& pSrcUV1,
	axis_pixel_stream_t& s_axis,
	ap_uint<32> nWidthPC,
	ap_uint<32> nHeight,
	ap_uint<32> nSrcStrideYPC,
	ap_uint<32> nSrcStrideUVPC) {
	src_pixel_t luma;
	src_pixel_t chroma;
	ap_uint<32> nPos_srcY0 = 0, nPos_srcUV0 = 0, nPos_srcY1 = 0, nPos_srcUV1 = 0;
	bool bEven = true;
	ap_uint<32> i, j;

loop_height:
	for(i = 0; i < nHeight; i++) {
		if(bEven) {
			pSrcY0.read_request(nPos_srcY0, nWidthPC);
			pSrcUV0.read_request(nPos_srcUV0, nWidthPC);
		} else {
			pSrcY1.read_request(nPos_srcY1, nWidthPC);
			pSrcUV1.read_request(nPos_srcUV1, nWidthPC);
		}

loop_width:
		for(j = 0; j < nWidthPC; j++) {
			if(bEven) {
				luma = pSrcY0.read();
				chroma = pSrcUV0.read();
			} else {
				luma = pSrcY1.read();
				chroma = pSrcUV1.read();
			}

			luma_chroma_to_axis(luma, chroma, s_axis, i == 0 && j == 0, 0);
		}

		if(bEven) {
			nPos_srcY0 += nSrcStrideYPC;
			nPos_srcUV0 += nSrcStrideUVPC;
		} else {
			nPos_srcY1 += nSrcStrideYPC;
			nPos_srcUV1 += nSrcStrideUVPC;
		}

		bEven = bEven ? false : true;
	}
}

void lbl_rd(
#if 0
	ap_uint<SRC_PTR_WIDTH>* pSrcY0,
	ap_uint<SRC_PTR_WIDTH>* pSrcY1,
	ap_uint<SRC_PTR_WIDTH>* pSrcUV0,
	ap_uint<SRC_PTR_WIDTH>* pSrcUV1,
#else
	hls::burst_maxi<src_pixel_t> pSrcY0,
	hls::burst_maxi<src_pixel_t> pSrcUV0,
	hls::burst_maxi<src_pixel_t> pSrcY1,
	hls::burst_maxi<src_pixel_t> pSrcUV1,
#endif
	ap_uint<32> nSrcStrideY,
	ap_uint<32> nSrcStrideUV,
	axis_pixel_stream_t& s_axis,
	ap_uint<32> nWidth,
	ap_uint<32> nHeight,
	ap_uint<32> nControl) {
#pragma HLS INTERFACE m_axi port=pSrcY0 depth=48 offset=slave bundle=mm_video0
#pragma HLS INTERFACE m_axi port=pSrcUV0 depth=48 offset=slave bundle=mm_video1
#pragma HLS INTERFACE m_axi port=pSrcY1 depth=48 offset=slave bundle=mm_video2
#pragma HLS INTERFACE m_axi port=pSrcUV1 depth=48 offset=slave bundle=mm_video3
#pragma HLS INTERFACE axis port=s_axis register_mode=both
#pragma HLS INTERFACE s_axilite port=nSrcStrideY
#pragma HLS INTERFACE s_axilite port=nSrcStrideUV
#pragma HLS INTERFACE s_axilite port=nWidth
#pragma HLS INTERFACE s_axilite port=nHeight
#pragma HLS INTERFACE s_axilite port=nControl
#pragma HLS INTERFACE s_axilite port=return

	ap_uint<32> nWidthPC = nWidth >> SRC_BITSHIFT;
	ap_uint<32> nSrcStrideYPC = nSrcStrideY >> SRC_BITSHIFT;
	ap_uint<32> nSrcStrideUVPC = nSrcStrideUV >> SRC_BITSHIFT;

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ',' << nWidthPC << ','
		<< nSrcStrideYPC << ',' << nSrcStrideUVPC << std::endl;
#endif

#if 0
	// 2,192,710 ns
	mem_to_axis_v0(pSrcY0, pSrcY1, pSrcUV0, pSrcUV1, s_axis, nWidthPC, nHeight, nSrcStrideYPC, nSrcStrideUVPC);
#endif

#if 0
	// 3,254,830 ns
	mem_to_axis_v1(pSrcY0, pSrcY1, pSrcUV0, pSrcUV1, s_axis, nWidthPC, nHeight, nSrcStrideYPC, nSrcStrideUVPC);
#endif

#if 1
	// 2,934,190 ns
	mem_to_axis_v2(pSrcY0, pSrcY1, pSrcUV0, pSrcUV1, s_axis, nWidthPC, nHeight, nSrcStrideYPC, nSrcStrideUVPC);
#endif
}
