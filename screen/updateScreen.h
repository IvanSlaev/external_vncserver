/*
droid vnc server - Android VNC server
Copyright (C) 2011 Jose Pereira <onaips@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#define OUT_T CONCAT3E(uint,OUT,_t)
#define FUNCTION CONCAT2E(update_screen_,OUT)

void FUNCTION(void)
{
	idle = 0;
	OUT_T* b = (OUT_T*) readBuffer();

	/*
	int i,j;
	int offset=0;
	int pixelToVirtual;

	int max_x=-1,max_y=-1, min_x=999999, min_y=999999;
	int h;

	if (rotation == 0)
	{
		for (j = 0; j < vncscr->height; j++) {
			for (i = 0; i < vncscr->width; i++) {
				offset = j * vncscr->width;

				if (method==FRAMEBUFFER)
					pixelToVirtual = PIXEL_TO_VIRTUALPIXEL_FB(i,j);
				else
					pixelToVirtual = PIXEL_TO_VIRTUALPIXEL(i,j);

				if (a[i + offset]!=b[pixelToVirtual]) {
					a[i + offset]=b[pixelToVirtual];
					
					if (i>max_x)
						max_x=i;
						
					if (i<min_x)
						min_x=i;

					if (j>max_y)
						max_y=j;
						
					if (j<min_y)
						min_y=j;

					idle=0;
				}
			}
		}
	}
	else if (rotation==90)
	{
		for (j = 0; j < vncscr->width; j++) {
			for (i = 0; i < vncscr->height; i++) {
				offset = i * vncscr->width;

				if (method==FRAMEBUFFER)
					pixelToVirtual = PIXEL_TO_VIRTUALPIXEL_FB(i,j);
				else
					pixelToVirtual = PIXEL_TO_VIRTUALPIXEL(i,j);			

				if (a[(vncscr->width - 1 - j + offset)] != b[pixelToVirtual])
				{
					a[(vncscr->width - 1 - j + offset)] = b[pixelToVirtual];

					if (i>max_y)
						max_y=i;
						
					if (i<min_y)
						min_y=i;

					h=vncscr->width-j;

					if (h < min_x)
						min_x=vncscr->width-j;
					if (h > max_x)
						max_x=vncscr->width-j;

					idle=0;
				}
			}
		}
	}
	else if (rotation==180)
	{
		for (j = 0; j < vncscr->height; j++) {
			for (i = 0; i < vncscr->width; i++) {
				offset = (vncscr->height - 1 - j) * vncscr->width;

				if (method==FRAMEBUFFER)
					pixelToVirtual = PIXEL_TO_VIRTUALPIXEL_FB(i,j);
				else
					pixelToVirtual = PIXEL_TO_VIRTUALPIXEL(i,j);

				if (a[((vncscr->width - 1 - i) + offset )]!=b[pixelToVirtual]) {
					a[((vncscr->width - 1 - i) + offset )]=b[pixelToVirtual];
					
					if (i>max_x)
						max_x=i;
						
					if (i<min_x)
						min_x=i;

					h=vncscr->height-j;

					if (h < min_y)
						min_y=vncscr->height-j;
						
					if (h > max_y)
						max_y=vncscr->height-j;

					idle=0;
				}
			}
		}
	}
	else if (rotation==270)
	{
		for (j = 0; j < vncscr->width; j++) {
			for (i = 0; i < vncscr->height; i++) {
				offset = (vncscr->height - 1 - i) * vncscr->width;

				if (method==FRAMEBUFFER)
					pixelToVirtual = PIXEL_TO_VIRTUALPIXEL_FB(i,j);
				else
					pixelToVirtual = PIXEL_TO_VIRTUALPIXEL(i,j);

				if(a[j + offset] != b[pixelToVirtual]) {
					a[j + offset] = b[pixelToVirtual];

					if (i>max_y)
						max_y=i;
						
					if (i<min_y)
						min_y=i;

					if (j < min_x)
						min_x=j;
						
					if (j > max_x)
						max_x=j;

					idle=0;
				}
			}
		}
	}
	*/

	if (!idle)
	{
		memcpy(vncbuf, b, screenformat.width * screenformat.height * screenformat.bitsPerPixel / CHAR_BIT);
		rfbMarkRectAsModified(vncscr, 0, 0, screenformat.width, screenformat.height);
	}
}
