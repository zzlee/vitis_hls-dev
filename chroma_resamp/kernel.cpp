#include <stdio.h>

#ifndef __SYNTHESIS__
#include <iostream>
#include <iomanip>
#include <cstdlib>
#endif

#include "chroma_resamp.h"

#define ERROR_IO_EOL_EARLY  (1 << 0)
#define ERROR_IO_EOL_LATE   (1 << 1)
#define ERROR_IO_SOF_EARLY  (1 << 0)
#define ERROR_IO_SOF_LATE   (1 << 1)

void axis_to_src_strm(
	src_axis_stream_t& axis_in,
	src_pixel_stream_t& strm0,
	ap_uint<32> nWidth_nppc,
	ap_uint<32> nHeight,
	ap_uint<32>& nResult)
{
	ap_uint<32> res = 0;
	src_axis_t axi;

	bool sof = 0;
loop_wait_for_start:
	while (!sof) {
#pragma HLS PIPELINE II=1
#pragma HLS loop_tripcount avg=0 max=0
		axis_in.read(axi);
		sof = axi.user.to_int();
	}

loop_height:
	for (ap_uint<32> i = 0; i < nHeight; i++) {
#pragma HLS loop_tripcount avg=0 max=0
		bool eol = 0;
loop_width:
		for (ap_uint<32> j = 0; j < nWidth_nppc; j++) {
#pragma HLS loop_flatten off
#pragma HLS pipeline
#pragma HLS loop_tripcount avg=0 max=0
			if (sof || eol) {
				sof = 0;
				eol = axi.last.to_int();
			} else {
				// If we didn't reach EOL, then read the next pixel
				axis_in.read(axi);
				eol = axi.last.to_int();
				bool user = axi.user.to_int();
				if(user) {
					res |= ERROR_IO_SOF_EARLY;
				}
			}
			if (eol && (j != nWidth_nppc - 1)) {
				res |= ERROR_IO_EOL_EARLY;
			}

			strm0.write(axi.data);
		}

loop_wait_for_eol:
		while (!eol) {
#pragma HLS pipeline II=1
#pragma HLS loop_tripcount avg=0 max=0
			// Keep reading until we get to EOL
			axis_in.read(axi);
			eol = axi.last.to_int();
			res |= ERROR_IO_EOL_LATE;
		}
	}

	nResult = res;
}

void dst_strm_to_axis(
	dst_pixel_stream_t& strm1,
	dst_axis_stream_t& axis_out,
	ap_uint<32> nWidth_nppc,
	ap_uint<32> nHeight) {
	dst_pixel_t pix;
	dst_axis_t axi;

loop_height:
	for (ap_uint<32> i = 0; i < nHeight; i++) {
loop_width:
		for (ap_uint<32> j = 0; j < nWidth_nppc; j++) {
			strm1.read(pix);

			axi.keep = -1;
			axi.strb = -1;
			axi.id = 0;
			axi.dest = 0;
			axi.user = (i == 0 && j == 0);
			axi.last = (j == (nWidth_nppc - 1));
			axi.data = pix;

			axis_out.write(axi);
		}
	}
}

void yuv444_yuv444(const src_pixel_t& src_pix, dst_pixel_t& dst_pix) {
#pragma HLS inline
	dst_pix = src_pix;
}

void yuv420_yuv444_even(const src_pixel_t& src_pix, dst_pixel_t& dst_pix) {
#pragma HLS inline
	const int pix_depth = XF_DTPIXELDEPTH(SRC_PIXEL_TYPE, SRC_NPPC);
	const int nppc_2 = XF_NPIXPERCYCLE(SRC_NPPC) >> 1;
	const int pix_width_2 = XF_PIXELWIDTH(SRC_PIXEL_TYPE, SRC_NPPC) << 1;

	for(int t = 0;t < nppc_2;t++) {
#pragma HLS UNROLL factor=nppc_2 skip_exit_check
		dst_pix(t*pix_width_2 + pix_depth*1-1, t*pix_width_2 + pix_depth*0) = src_pix(t*pix_width_2 + pix_depth*1-1, t*pix_width_2 + pix_depth*0); // Y0
		dst_pix(t*pix_width_2 + pix_depth*2-1, t*pix_width_2 + pix_depth*1) = src_pix(t*pix_width_2 + pix_depth*2-1, t*pix_width_2 + pix_depth*1); // U01
		dst_pix(t*pix_width_2 + pix_depth*3-1, t*pix_width_2 + pix_depth*2) = src_pix(t*pix_width_2 + pix_depth*4-1, t*pix_width_2 + pix_depth*3); // V01
		dst_pix(t*pix_width_2 + pix_depth*4-1, t*pix_width_2 + pix_depth*3) = src_pix(t*pix_width_2 + pix_depth*3-1, t*pix_width_2 + pix_depth*2); // Y1
		dst_pix(t*pix_width_2 + pix_depth*5-1, t*pix_width_2 + pix_depth*4) = src_pix(t*pix_width_2 + pix_depth*2-1, t*pix_width_2 + pix_depth*1); // U01
		dst_pix(t*pix_width_2 + pix_depth*6-1, t*pix_width_2 + pix_depth*4) = src_pix(t*pix_width_2 + pix_depth*4-1, t*pix_width_2 + pix_depth*3); // V01
	}
}

void yuv420_yuv444_odd(const src_pixel_t& src_pix, const src_pixel_t& src_pix_even, dst_pixel_t& dst_pix) {
#pragma HLS inline
	const int pix_depth = XF_DTPIXELDEPTH(SRC_PIXEL_TYPE, SRC_NPPC);
	const int nppc_2 = XF_NPIXPERCYCLE(SRC_NPPC) >> 1;
	const int pix_width_2 = XF_PIXELWIDTH(SRC_PIXEL_TYPE, SRC_NPPC) << 1;

	for(int t = 0;t < nppc_2;t++) {
#pragma HLS UNROLL factor=nppc_2 skip_exit_check
		dst_pix(t*pix_width_2 + pix_depth*1-1, t*pix_width_2 + pix_depth*0) = src_pix(t*pix_width_2 + pix_depth*1-1, t*pix_width_2 + pix_depth*0); // Y0
		dst_pix(t*pix_width_2 + pix_depth*2-1, t*pix_width_2 + pix_depth*1) = src_pix_even(t*pix_width_2 + pix_depth*2-1, t*pix_width_2 + pix_depth*1); // U01
		dst_pix(t*pix_width_2 + pix_depth*3-1, t*pix_width_2 + pix_depth*2) = src_pix_even(t*pix_width_2 + pix_depth*4-1, t*pix_width_2 + pix_depth*3); // V01
		dst_pix(t*pix_width_2 + pix_depth*4-1, t*pix_width_2 + pix_depth*3) = src_pix(t*pix_width_2 + pix_depth*3-1, t*pix_width_2 + pix_depth*2); // Y1
		dst_pix(t*pix_width_2 + pix_depth*5-1, t*pix_width_2 + pix_depth*4) = src_pix_even(t*pix_width_2 + pix_depth*2-1, t*pix_width_2 + pix_depth*1); // U01
		dst_pix(t*pix_width_2 + pix_depth*6-1, t*pix_width_2 + pix_depth*4) = src_pix_even(t*pix_width_2 + pix_depth*4-1, t*pix_width_2 + pix_depth*3); // V01
	}
}

void yuv444_yuv422(const src_pixel_t& src_pix, dst_pixel_t& dst_pix) {
#pragma HLS inline
	const int pix_depth = XF_DTPIXELDEPTH(SRC_PIXEL_TYPE, SRC_NPPC);
	const int nppc_2 = XF_NPIXPERCYCLE(SRC_NPPC) >> 1;
	const int pix_width_2 = XF_PIXELWIDTH(SRC_PIXEL_TYPE, SRC_NPPC) << 1;

	for(int t = 0;t < nppc_2;t++) {
#pragma HLS UNROLL factor=nppc_2 skip_exit_check
		dst_pix(t*pix_width_2 + pix_depth*1-1, t*pix_width_2 + pix_depth*0) = src_pix(t*pix_width_2 + pix_depth*1-1, t*pix_width_2 + pix_depth*0); // Y0
		dst_pix(t*pix_width_2 + pix_depth*2-1, t*pix_width_2 + pix_depth*1) =
			(src_pix(t*pix_width_2 + pix_depth*2-1, t*pix_width_2 + pix_depth*1) + src_pix(t*pix_width_2 + pix_depth*3-1, t*pix_width_2 + pix_depth*4)) >> 1; // (U0 + U1) / 2
		dst_pix(t*pix_width_2 + pix_depth*3-1, t*pix_width_2 + pix_depth*2) = src_pix(t*pix_width_2 + pix_depth*4-1, t*pix_width_2 + pix_depth*3); // Y1
		dst_pix(t*pix_width_2 + pix_depth*4-1, t*pix_width_2 + pix_depth*3) =
			(src_pix(t*pix_width_2 + pix_depth*3-1, t*pix_width_2 + pix_depth*2) + src_pix(t*pix_width_2 + pix_depth*4-1, t*pix_width_2 + pix_depth*5)) >> 1; // (V0 + V1) / 2
		dst_pix(t*pix_width_2 + pix_depth*6-1, t*pix_width_2 + pix_depth*4) = 0; // X
	}
}

void resamp(
	src_pixel_stream_t& strm0,
	dst_pixel_stream_t& strm1,
	ap_uint<32> nWidth_nppc,
	ap_uint<32> nHeight,
	ap_uint<32> nFlags) {
	src_pixel_t src_pix;
	dst_pixel_t dst_pix;
	bool bEven = true;
	src_pixel_t even_pix[MAX_WIDTH * XF_PIXELWIDTH(SRC_PIXEL_TYPE, SRC_NPPC) / SRC_WORD_WIDTH];

loop_height:
	for (ap_uint<32> i = 0; i < nHeight; i++) {
loop_width:
		for (ap_uint<32> j = 0; j < nWidth_nppc; j++) {
			strm0.read(src_pix);

			switch(nFlags) {
			case FLAGS_YUV444_YUV444:
				yuv444_yuv444(src_pix, dst_pix);
				break;

			case FLAGS_YUV420_YUV444:
				if(bEven) {
					even_pix[j] = src_pix;
					yuv420_yuv444_even(src_pix, dst_pix);

					bEven = false;
				} else {
					yuv420_yuv444_odd(src_pix, even_pix[j], dst_pix);

					bEven = true;
				}

				break;

			case FLAGS_YUV444_YUV422:
				yuv444_yuv422(src_pix, dst_pix);
				break;
			}

			strm1.write(dst_pix);
		}
	}
}

void chroma_resamp(
	src_axis_stream_t& axis_in,
	dst_axis_stream_t& axis_out,
	ap_uint<32> nWidth,
	ap_uint<32> nHeight,
	ap_uint<32> nFlags,
	ap_uint<32>& nResult) {
#pragma HLS INTERFACE axis port=axis_in register_mode=both
#pragma HLS INTERFACE axis port=axis_out register_mode=both
#pragma HLS INTERFACE s_axilite port=nWidth
#pragma HLS INTERFACE s_axilite port=nHeight
#pragma HLS INTERFACE s_axilite port=nFlags
#pragma HLS INTERFACE s_axilite port=nResult
#pragma HLS INTERFACE s_axilite port=return

	ap_uint<32> nWidth_nppc = nWidth / XF_NPIXPERCYCLE(SRC_NPPC);

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ',' << nWidth << 'x' << nHeight << ',' << nFlags << ',' << nWidth_nppc << std::endl;
#endif

	src_pixel_stream_t strm0("strm0");
	dst_pixel_stream_t strm1("strm1");

#pragma HLS DATAFLOW
	axis_to_src_strm(axis_in, strm0, nWidth_nppc, nHeight, nResult);
	resamp(strm0, strm1, nWidth_nppc, nHeight, nFlags);
	dst_strm_to_axis(strm1, axis_out, nWidth_nppc, nHeight);
}
