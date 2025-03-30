#include <stdio.h>

#ifndef __SYNTHESIS__
#include <iostream>
#include <iomanip>
#include <cstdlib>
#endif

#include "aximm_test0.h"

void data_gen(
	ap_uint<32> nSize,
	ap_uint<32> nTimes,
	dst_pixel_stream_t& strmDstPxl) {
	ap_uint<32> nSizePC = nSize >> DST_BITSHIFT;
	int s = 0;

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ": ++++" << std::endl;
#endif

loop_times:
	for(ap_uint<32> i = 0;i < nTimes;i++) {

#ifndef __SYNTHESIS__
		std::cout << "[" << i << "] ";
#endif

loop_burst:
		for(ap_uint<32> j = 0;j < nSizePC;j++) {
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

void fill_data_times_v0(
	dst_pixel_stream_t& strmDstPxl,
	ap_uint<32> nSize,
	ap_uint<32> nTimes,
	hls::burst_maxi<dst_pixel_t> pDstPxl) {
	ap_uint<32> nSizePC = nSize >> DST_BITSHIFT;

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ": ++++" << std::endl;
#endif

loop_times:
	for(ap_uint<32> i = 0;i < nTimes;i++) {

#ifndef __SYNTHESIS__
		std::cout << "[" << i << "] ";
#endif

		pDstPxl.write_request(0, nSizePC);
loop_burst:
		for(ap_uint<32> j = 0;j < nSizePC;j++) {
#pragma HLS PIPELINE
			pDstPxl.write(strmDstPxl.read());
		}
		pDstPxl.write_response();

#ifndef __SYNTHESIS__
		std::cout << std::dec << std::endl;
#endif
	}

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ": ----" << std::endl;
#endif
}

void fill_data_times_v1(
	dst_pixel_stream_t& strmDstPxl,
	ap_uint<32> nSize,
	ap_uint<32> nTimes,
	dst_pixel_t* pDstPxl) {
	ap_uint<32> nSizePC = nSize >> DST_BITSHIFT;
	dst_pixel_t pBuf[MAX_WIDTH >> DST_BITSHIFT];

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ": ++++" << std::endl;
#endif

loop_preload:
	for(ap_uint<32> i = 0;i < nTimes;i++) {
loop_burst:
		for(ap_uint<32> j = 0;j < nSizePC;j++) {
			strmDstPxl >> pBuf[j];
		}

		memcpy(pDstPxl, pBuf, nSize);
	}

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ": ----" << std::endl;
#endif
}

void fill_data_times_v2(
	dst_pixel_stream_t& strmDstPxl,
	ap_uint<32> nSize,
	ap_uint<32> nTimes,
	dst_pixel_t* pDstPxl) {
	ap_uint<32> nSizePC = nSize >> DST_BITSHIFT;

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ": ++++" << std::endl;
#endif

loop_preload:
	for(ap_uint<32> i = 0;i < nTimes;i++) {
loop_burst:
		for(ap_uint<32> j = 0;j < nSizePC;j++) {
			strmDstPxl >> pDstPxl[j];
		}
	}

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ": ----" << std::endl;
#endif
}

void data_consume(
	dst_pixel_stream_t& strmDstPxl,
	ap_uint<32> nSize,
	ap_uint<32> nTimes) {
	ap_uint<32> nSizePC = nSize >> DST_BITSHIFT;
	int s = 0;

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ": ++++" << std::endl;
#endif

loop_times:
	for(ap_uint<32> i = 0;i < nTimes;i++) {

#ifndef __SYNTHESIS__
		std::cout << "[" << i << "] ";
#endif

loop_burst:
		for(ap_uint<32> j = 0;j < nSizePC;j++) {
			dst_pixel_t oDstPxl = strmDstPxl.read();

#ifndef __SYNTHESIS__
			std::cout << std::hex << std::setw(16) << oDstPxl << ' ';
#endif
		}

#ifndef __SYNTHESIS__
		std::cout << std::dec << std::endl;
#endif
	}

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << ": ----" << std::endl;
#endif
}

void aximm_test0(
	dst_pixel_t* pDstPxl,
	ap_uint<32> nSize,
	ap_uint<32> nTimes) {
#pragma HLS INTERFACE m_axi port=pDstPxl offset=slave bundle=mm_video
#pragma HLS INTERFACE s_axilite port=nSize
#pragma HLS INTERFACE s_axilite port=nTimes
#pragma HLS INTERFACE s_axilite port=return

#ifndef __SYNTHESIS__
	std::cout << __FUNCTION__ << std::endl;
	std::cout << "DST_PTR_WIDTH=" << DST_PTR_WIDTH << std::endl;
	std::cout << "DST_BITSHIFT=" << DST_BITSHIFT << std::endl;
	std::cout << nSize << 'x' << nTimes << std::endl;
#endif

	dst_pixel_stream_t strmDstPxl("strmDstPxl");

#pragma HLS DATAFLOW
	data_gen(nSize, nTimes, strmDstPxl);

#if 0
	fill_data_times_v0(strmDstPxl, nSize, nTimes, pDstPxl);
#endif

#if 0
	data_consume(strmDstPxl, nSize, nTimes);
#endif

#if 0
	fill_data_times_v1(strmDstPxl, nSize, nTimes, pDstPxl);
#endif

#if 1
	fill_data_times_v2(strmDstPxl, nSize, nTimes, pDstPxl);
#endif
}
