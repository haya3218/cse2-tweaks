// THIS IS DECOMPILED PROPRIETARY CODE - USE AT YOUR OWN RISK.
//
// The original code belongs to Daisuke "Pixel" Amaya.
//
// Modifications and custom code are under the MIT licence.
// See LICENCE.txt for details.

#include "Back.h"

#include <stddef.h>
#include <stdio.h>
#include <string>

#include "WindowsWrapper.h"

#include "CommonDefines.h"
#include "Draw.h"
#include "File.h"
#include "Main.h"

BACK gBack;
int gWaterY;
static unsigned long color_black;

// TODO - Another function that has an incorrect stack frame
BOOL InitBack(const char *fName, int type)
{
	std::string path;
	FILE *fp;

	color_black = GetCortBoxColor(RGB(0, 0, 0x10));	// Unused. This may have once been used by background type 4 (the solid black background)

	// Get width and height
	fp = NULL;
	const char *bmp_file_extensions[] = {"pbm", "bmp"};
	for (size_t i = 0; i < sizeof(bmp_file_extensions) / sizeof(bmp_file_extensions[0]) && fp == NULL; ++i)
	{
		path = gSpritePath + '/' + fName + '.' + bmp_file_extensions[i];
		fp = fopen(path.c_str(), "rb");
	}

	if (fp != NULL)
	{
		if (fgetc(fp) != 'B' || fgetc(fp) != 'M')
		{
		#ifdef FIX_MAJOR_BUGS
			// The original game forgets to close fp
			fclose(fp);
		#endif
			return FALSE;
		}

		fseek(fp, 18, SEEK_SET);

		gBack.partsW = File_ReadLE32(fp);
		gBack.partsH = File_ReadLE32(fp);
		fclose(fp);
	}
	else
	{
		path = gSpritePath + '/' + fName + ".png";
		fp = fopen(path.c_str(), "rb");

		if (fp == NULL)
			return FALSE;

		if (fgetc(fp) != 0x89 || fgetc(fp) != 'P' || fgetc(fp) != 'N' || fgetc(fp) != 'G')
		{
			fclose(fp);
			return FALSE;
		}

		fseek(fp, 16, SEEK_SET);

		gBack.partsW = File_ReadBE32(fp);
		gBack.partsH = File_ReadBE32(fp);
		fclose(fp);
	}

	// Adjust background sizes to account for higher sprite resolutions
	gBack.partsW /= (SPRITE_SCALE*gSpriteScale);
	gBack.partsH /= (SPRITE_SCALE*gSpriteScale);

	gBack.flag = TRUE;	// This variable is otherwise unused

	// *Now* we actually load the bitmap
	if (!ReloadBitmap_File(fName, SURFACE_ID_LEVEL_BACKGROUND, 2, TRUE))
		return FALSE;

	gBack.type = type;
	gWaterY = 240 * 0x10 * 0x200;
	return TRUE;
}

void ActBack(void)
{
	switch (gBack.type)
	{
		case BACKGROUND_TYPE_AUTOSCROLL:
			gBack.fx += 6 * 0x200;
			break;

		case BACKGROUND_TYPE_CLOUDS_WINDY:
		case BACKGROUND_TYPE_CLOUDS:
			++gBack.fx;
			gBack.fx %= 640;
			break;
	}
}

/// Draw background background elements
void PutBack(int fx, int fy)
{
	int x, y;
	RECT rect = {0, 0, gBack.partsW, gBack.partsH};

	switch (gBack.type)
	{
		case BACKGROUND_TYPE_STATIONARY:
			for (y = 0; y < WINDOW_HEIGHT; y += gBack.partsH)
				for (x = 0; x < WINDOW_WIDTH; x += gBack.partsW)
					PutBitmap4(&grcGame, PixelToScreenCoord(x), PixelToScreenCoord(y), &rect, SURFACE_ID_LEVEL_BACKGROUND);

			break;

		case BACKGROUND_TYPE_MOVE_DISTANT:
			for (y = -(fy / 2 % (gBack.partsH * 0x200)); y < WINDOW_HEIGHT * 0x200; y += gBack.partsH * 0x200)
				for (x = -(fx / 2 % (gBack.partsW * 0x200)); x < WINDOW_WIDTH * 0x200; x += gBack.partsW * 0x200)
					PutBitmap4(&grcGame, SubpixelToScreenCoord(x), SubpixelToScreenCoord(y), &rect, SURFACE_ID_LEVEL_BACKGROUND);

			break;

		case BACKGROUND_TYPE_MOVE_NEAR:
			for (y = -(fy % (gBack.partsH * 0x200)); y < WINDOW_HEIGHT * 0x200; y += gBack.partsH * 0x200)
				if(gMirrorMode){
					for (x = WINDOW_WIDTH * 0x200; x >= -(fx % (gBack.partsW * 0x200)) - gBack.partsW * 0x200; x -= gBack.partsW * 0x200)
						PutBitmap4(&grcGame, SubpixelToScreenCoord(x), SubpixelToScreenCoord(y), &rect, SURFACE_ID_LEVEL_BACKGROUND);
				}
				else{
					for (x = -(fx % (gBack.partsW * 0x200)); x < WINDOW_WIDTH * 0x200; x += gBack.partsW * 0x200)
						PutBitmap4(&grcGame, SubpixelToScreenCoord(x), SubpixelToScreenCoord(y), &rect, SURFACE_ID_LEVEL_BACKGROUND);
				}

			break;

		case BACKGROUND_TYPE_AUTOSCROLL:
			for (y = -gBack.partsH; y < WINDOW_HEIGHT; y += gBack.partsH)
				for (x = -(gBack.fx % (gBack.partsW * 0x200)); x < WINDOW_WIDTH * 0x200; x += gBack.partsW * 0x200)
					PutBitmap4(&grcGame, SubpixelToScreenCoord(x), PixelToScreenCoord(y), &rect, SURFACE_ID_LEVEL_BACKGROUND);

			break;

		case BACKGROUND_TYPE_CLOUDS_WINDY:
		case BACKGROUND_TYPE_CLOUDS:
			// Draw sky

			// Draw the top row (including the moon/sun)
			rect.top = 0;
			rect.bottom = 88;
			rect.left = 0;
			rect.right = 320;
			PutBitmap4(&grcGame, PixelToScreenCoord((WINDOW_WIDTH - 320) / 2), PixelToScreenCoord(0), &rect, SURFACE_ID_LEVEL_BACKGROUND);

			// Draw the repeating star/sky pattern on each side of the top row
			if (gBack.type == 6)
				rect.right = 255;
			else
				rect.left = 106;

			for (x = ((WINDOW_WIDTH - 320) / 2); x > 0; x -= (rect.right - rect.left))
				PutBitmap4(&grcGame, PixelToScreenCoord(x - (rect.right - rect.left)), PixelToScreenCoord(0), &rect, SURFACE_ID_LEVEL_BACKGROUND);
			for (x = ((WINDOW_WIDTH - 320) / 2) + 320; x < WINDOW_WIDTH; x += (rect.right - rect.left))
				PutBitmap4(&grcGame, PixelToScreenCoord(x), PixelToScreenCoord(0), &rect, SURFACE_ID_LEVEL_BACKGROUND);

			// Draw each cloud layer from top to bottom

			// Draw first cloud layer
			rect.top = 88;
			rect.bottom = 123;
			rect.left = 0;
			rect.right = 320;
			for (x = -((gBack.fx * 0x200) / 2); x < WINDOW_WIDTH * 0x200; x += 320 * 0x200)
				PutBitmap4(&grcGame, SubpixelToScreenCoord(x), PixelToScreenCoord(88), &rect, SURFACE_ID_LEVEL_BACKGROUND);

			// Draw second cloud layer
			rect.top = 123;
			rect.bottom = 146;
			rect.left = 0;
			rect.right = 320;
			for (x = -((gBack.fx % 320) * 0x200); x < WINDOW_WIDTH * 0x200; x += 320 * 0x200)
				PutBitmap4(&grcGame, SubpixelToScreenCoord(x), PixelToScreenCoord(123), &rect, SURFACE_ID_LEVEL_BACKGROUND);

			// Draw third cloud layer
			rect.top = 146;
			rect.bottom = 176;
			rect.left = 0;
			rect.right = 320;
			for (x = -(((gBack.fx * 2) % 320) * 0x200); x < WINDOW_WIDTH * 0x200; x += 320 * 0x200)
				PutBitmap4(&grcGame, SubpixelToScreenCoord(x), PixelToScreenCoord(146), &rect, SURFACE_ID_LEVEL_BACKGROUND);

			// Draw fourth cloud layer
			rect.top = 176;
			rect.bottom = 240;
			rect.left = 0;
			rect.right = 320;
			for (x = -(((gBack.fx * 4) % 320) * 0x200); x < WINDOW_WIDTH * 0x200; x += 320 * 0x200)
				PutBitmap4(&grcGame, SubpixelToScreenCoord(x), PixelToScreenCoord(176), &rect, SURFACE_ID_LEVEL_BACKGROUND);

			break;
	}
}

/// Draw background foreground elements - only the water background type makes use of this
void PutFront(int fx, int fy)
{
	int xpos, ypos;

	RECT rcWater[2] = {{0, 0, 32, 16}, {0, 16, 32, 48}};

	int x, y;
	int x_1, x_2;
	int y_1, y_2;

	switch (gBack.type)
	{
		case BACKGROUND_TYPE_WATER:
			x_1 = fx / (32 * 0x200);
			x_2 = x_1 + (((WINDOW_WIDTH + (32 - 1)) / 32) + 1);
			y_1 = 0;
			y_2 = y_1 + 32;

			for (y = y_1; y < y_2; ++y)
			{
				ypos = SubpixelToScreenCoord(y * 32 * 0x200) - SubpixelToScreenCoord(fy) + SubpixelToScreenCoord(gWaterY);

				if (ypos < PixelToScreenCoord(-32))
					continue;

				if (ypos > PixelToScreenCoord(WINDOW_HEIGHT))
					break;

				for (x = x_1; x < x_2; ++x)
				{
					xpos = SubpixelToScreenCoord((gMirrorMode? x + 1: x) * 32 * 0x200) - SubpixelToScreenCoord(fx);
					PutBitmap3(&grcGame, xpos, ypos, &rcWater[1], SURFACE_ID_LEVEL_BACKGROUND);
					if (y == 0)
						PutBitmap3(&grcGame, xpos, ypos, &rcWater[0], SURFACE_ID_LEVEL_BACKGROUND);
				}
			}

			break;
	}
}
