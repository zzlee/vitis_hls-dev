#include <stdio.h>
#include <vector>
#include <iostream>
#include "chroma_resamp.h"

int main () {
	std::cout << "MAX_WIDTH=" << MAX_WIDTH << std::endl;
	std::cout << "MAX_HEIGHT=" << MAX_HEIGHT << std::endl;
	std::cout << "SRC_WORD_WIDTH=" << SRC_WORD_WIDTH << std::endl;
	std::cout << "DST_WORD_WIDTH=" << DST_WORD_WIDTH << std::endl;
	std::cout << "XF_NPIXPERCYCLE(SRC_NPPC)=" << XF_NPIXPERCYCLE(SRC_NPPC) << std::endl;
	std::cout << "XF_NPIXPERCYCLE(DST_NPPC)=" << XF_NPIXPERCYCLE(DST_NPPC) << std::endl;
	std::cout << "XF_PIXELWIDTH(SRC_NPPC)=" << XF_PIXELWIDTH(SRC_PIXEL_TYPE, SRC_NPPC) << std::endl;
	std::cout << "XF_PIXELWIDTH(DST_NPPC)=" << XF_PIXELWIDTH(SRC_PIXEL_TYPE, DST_NPPC) << std::endl;
	std::cout << "XF_DTPIXELDEPTH(SRC_NPPC)=" << XF_DTPIXELDEPTH(SRC_PIXEL_TYPE, SRC_NPPC) << std::endl;
	std::cout << "XF_DTPIXELDEPTH(DST_NPPC)=" << XF_DTPIXELDEPTH(SRC_PIXEL_TYPE, DST_NPPC) << std::endl;

	src_axis_stream_t axis_in("axis_in");
	dst_axis_stream_t axis_out("axis_out");
	ap_uint<32> nWidth = 64;
	ap_uint<32> nHeight = 16;
	ap_uint<32> nFlags = FLAGS_YUV444_YUV444;
	ap_uint<32> nResult;

	int nWidth_nppc = (nWidth / XF_NPIXPERCYCLE(SRC_NPPC));

	std::cout << "axis_in:" << std::endl;
	{
		src_axis_t axi;
		axi.data = "0xAABBCCDDEEFFAABBCCDDEEFFAABBCCDDEEFFAABBCCDDEEFFAABBCCDDEEFF";
		for(int i = 0;i < nHeight;i++) {
			std::cout << i << ": ";
			for(int j = 0;j < nWidth_nppc;j++) {
				axi.keep = -1;
				axi.strb = -1;
				axi.id = 0;
				axi.dest = 0;
				axi.user = (i == 0 && j == 0);
				axi.last = (j == (nWidth_nppc - 1));
				axi.data++;

				std::cout << std::setw(SRC_WORD_WIDTH / 8) << std::hex << axi.data <<
					'(' << axi.user << ',' << axi.last << ')' << ' ';

				axis_in << axi;
			}

			std::cout << std::dec << std::endl;
		}
	}

	chroma_resamp(axis_in, axis_out, nWidth, nHeight, nFlags, nResult);

	std::cout << "nResult=" << nResult << std::endl;
	std::cout << "axis_out:" << std::endl;
	{
		dst_axis_t axi;
		for(int i = 0;i < nHeight;i++) {
			std::cout << i << ": ";
			for(int j = 0;j < nWidth_nppc;j++) {
				axi = axis_out.read();

				std::cout << std::setw(DST_WORD_WIDTH / 8) << std::hex << axi.data <<
					'(' << axi.user << ',' << axi.last << ')' << ' ';
			}
			std::cout << std::dec << std::endl;
		}
	}

	return 0;
}
