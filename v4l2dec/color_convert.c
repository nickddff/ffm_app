#include <stdio.h>

void tile420_y_uv_to_rgb888(unsigned char *src_y, unsigned char *src_uv, int src_width, int src_height,
	int dst_width, int dst_height, int dst_stride, unsigned char* dstAddr)
{

	int line = 0, col = 0, linewidth = 0;
	int y = 0, u = 0, v = 0, yy = 0, vr = 0, ug = 0, vg = 0, ub = 0;
	int r = 0, g = 0, b = 0;
	const unsigned char *py = NULL, *pu = NULL, *pv = NULL;
	int width = src_width;
	int height = src_height;
	int ySize = width * height;
	ySize = (ySize + 255)/256;

	unsigned int *dst = (unsigned int *)dstAddr;

	py = src_y;

	pu = src_uv;
	pv = pu + 8;

	int iLoop = 0, jLoop = 0, kLoop = 0, dxy = 0;
	int stride_y = width*16;
	int stride_uv = width*8;

	/* fix stride. */
	dst_stride = dst_stride == 0 ? dst_width:dst_stride;

	for (line = 0; line < height; line++) {
		/*ignore*/
		if(line >= dst_height) {
			line = height;
			continue;
		}
		for (col = 0; col < width; col++) {
			if ( iLoop > 0 && iLoop % 16 == 0 ) {
				jLoop++;
				iLoop = 0;
				dxy = jLoop*256;
				iLoop++;
			} else {
				dxy = iLoop + jLoop * 256;
				iLoop++;
			}

			y = *(py+dxy);
			yy = y << 8;
			u = *(pu+dxy/2) - 128;
			ug = 88 * u;
			ub = 454 * u;
			v = *(pv+dxy/2) - 128;
			vg = 183 * v;
			vr = 359 * v;

			r = (yy + vr) >> 8;
			g = (yy - ug - vg) >> 8;
			b = (yy + ub ) >> 8;

			if (r < 0) r = 0;
			if (r > 255) r = 255;
			if (g < 0) g = 0;
			if (g > 255) g = 255;
			if (b < 0) b = 0;
			if (b > 255) b = 255;
			/*ignore*/
			if(col >= dst_width) {
				col = width;
				dst += dst_stride - dst_width;
				continue;
			}

			*dst++ = ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);

		}
		if ( kLoop > 0 && kLoop % 15 == 0 ) {
			py += stride_y + 16 - 256;
			pu += stride_uv + 16 - 128;
			pv = pu + 8;
			iLoop = 0; jLoop = 0; kLoop = 0;
		}
		else if ( kLoop & 1 ) {
			py += 16;
			pu += 16;
			pv = pu + 8;
			iLoop = 0; jLoop = 0; kLoop++;
		}
		else {
			py += 16;
			iLoop = 0; jLoop = 0;
			kLoop++;
		}

	}
}




