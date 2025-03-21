/*
     droid vnc server - Android VNC server
     Copyright (C) 2009 Jose Pereira <onaips@gmail.com>

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 3 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <binder/IPCThreadState.h>
#include <binder/IMemory.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>

#include "common.h"
#include "log.h"

#include "flinger.h"

using namespace android;

static uint32_t DEFAULT_DISPLAY_ID = ISurfaceComposer::eDisplayIdMain;
static const int COMPONENT_YUV = 0xFF;

extern screenFormat screenformat;
int32_t displayId = DEFAULT_DISPLAY_ID;
size_t Bpp = 32;
sp<IBinder> display = SurfaceComposerClient::getBuiltInDisplay(displayId);
ScreenshotClient *screenshotClient = NULL;

struct PixelFormatInformation {
    enum {
        INDEX_ALPHA   = 0,
        INDEX_RED     = 1,
        INDEX_GREEN   = 2,
        INDEX_BLUE    = 3
    };

    enum { // components
        ALPHA   = 1,
        RGB     = 2,
        RGBA    = 3,
        L       = 4,
        LA      = 5,
        OTHER   = 0xFF
    };

    struct szinfo {
        uint8_t h;
        uint8_t l;
    };

    inline PixelFormatInformation() : version(sizeof(PixelFormatInformation)) { }
    size_t getScanlineSize(unsigned int width) const;
    size_t getSize(size_t ci) const {
        return (ci <= 3) ? (cinfo[ci].h - cinfo[ci].l) : 0;
    }

    size_t      version;
    PixelFormat format;
    size_t      bytesPerPixel;
    size_t      bitsPerPixel;
    union {
        szinfo      cinfo[4];
        struct {
            uint8_t     h_alpha;
            uint8_t     l_alpha;
            uint8_t     h_red;
            uint8_t     l_red;
            uint8_t     h_green;
            uint8_t     l_green;
            uint8_t     h_blue;
            uint8_t     l_blue;
        };
    };
    uint8_t     components;
    uint8_t     reserved0[3];
    uint32_t    reserved1;
};

struct Info {
    size_t      size;
    size_t      bitsPerPixel;
    struct {
        uint8_t     ah;
        uint8_t     al;
        uint8_t     rh;
        uint8_t     rl;
        uint8_t     gh;
        uint8_t     gl;
        uint8_t     bh;
        uint8_t     bl;
    };
    uint8_t     components;
};

static Info const sPixelFormatInfos[] = {
         { 0,  0, { 0, 0,   0, 0,   0, 0,   0, 0 }, 0 },
         { 4, 32, {32,24,   8, 0,  16, 8,  24,16 }, PixelFormatInformation::RGBA },
         { 4, 24, { 0, 0,   8, 0,  16, 8,  24,16 }, PixelFormatInformation::RGB  },
         { 3, 24, { 0, 0,   8, 0,  16, 8,  24,16 }, PixelFormatInformation::RGB  },
         { 2, 16, { 0, 0,  16,11,  11, 5,   5, 0 }, PixelFormatInformation::RGB  },
         { 4, 32, {32,24,  24,16,  16, 8,   8, 0 }, PixelFormatInformation::RGBA },
         { 2, 16, { 1, 0,  16,11,  11, 6,   6, 1 }, PixelFormatInformation::RGBA },
         { 2, 16, { 4, 0,  16,12,  12, 8,   8, 4 }, PixelFormatInformation::RGBA },
         { 1,  8, { 8, 0,   0, 0,   0, 0,   0, 0 }, PixelFormatInformation::ALPHA},
         { 1,  8, { 0, 0,   8, 0,   8, 0,   8, 0 }, PixelFormatInformation::L    },
         { 2, 16, {16, 8,   8, 0,   8, 0,   8, 0 }, PixelFormatInformation::LA   },
         { 1,  8, { 0, 0,   8, 5,   5, 2,   2, 0 }, PixelFormatInformation::RGB  },
};

static const Info* gGetPixelFormatTable(size_t* numEntries) {
    if (numEntries) {
        *numEntries = sizeof(sPixelFormatInfos)/sizeof(Info);
    }
    return sPixelFormatInfos;
}

status_t getPixelFormatInformation(PixelFormat format, PixelFormatInformation* info)
{
    L("Retrieving pixel information with format %d\n", format);

    if (format <= 0)
        return BAD_VALUE;

    if (info->version != sizeof(PixelFormatInformation))
        return INVALID_OPERATION;

    switch (format) {
        case HAL_PIXEL_FORMAT_RGBA_8888: // 4x8-bit RGBA
            L("detected HAL_PIXEL_FORMAT_RGBA_8888\n");
            break;

        case HAL_PIXEL_FORMAT_RGBX_8888: // 4x8-bit RGB0
            L("detected HAL_PIXEL_FORMAT_RGBX_8888\n");
            break;

        case HAL_PIXEL_FORMAT_RGB_888: // 3x8-bit RGB
            L("detected HAL_PIXEL_FORMAT_RGB_888\n");
            break;

        case HAL_PIXEL_FORMAT_RGB_565: // 16-bit RGB
            L("detected HAL_PIXEL_FORMAT_RGB_565\n");
            break;

        case HAL_PIXEL_FORMAT_BGRA_8888: // 4x8-bit BGRA
            L("detected HAL_PIXEL_FORMAT_BGRA_8888\n");
            break;

        case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED: // 16-bit ARGB
            L("detected HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED\n");
            break;

        case HAL_PIXEL_FORMAT_BLOB: // 16-bit ARGB
            L("detected HAL_PIXEL_FORMAT_BLOB\n");
            break;

        case PIXEL_FORMAT_RGBA_5551: // 16-bit ARGB
            L("detected PIXEL_FORMAT_RGBA_5551\n");
            break;

        case PIXEL_FORMAT_RGBA_4444: // 16-bit ARGB
            L("detected PIXEL_FORMAT_RGBA_4444\n");
            break;

        default:
            L("unknown FORMAT!");
            break;
    }

    // YUV format from the HAL are handled here
    switch (format) {
    case HAL_PIXEL_FORMAT_YCbCr_422_SP:
    case HAL_PIXEL_FORMAT_YCbCr_422_I:
        L("detected HAL_PIXEL_FORMAT_YCbCr_422\n");
        info->bitsPerPixel = 16;
        goto done;
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_YV12:
        L("detected HAL_PIXEL_FORMAT_Y*\n");
        info->bitsPerPixel = 12;
     done:
        info->format = format;
        info->components = COMPONENT_YUV;
        info->bytesPerPixel = 1;
        info->h_alpha = 0;
        info->l_alpha = 0;
        info->h_red = info->h_green = info->h_blue = 8;
        info->l_red = info->l_green = info->l_blue = 0;
        return NO_ERROR;
    }

    size_t numEntries;
    const Info *i = gGetPixelFormatTable(&numEntries) + format;
    bool valid = uint32_t(format) < numEntries;
    if (!valid) {
        return BAD_INDEX;
    }

    L("Filling info struct\n");
    info->format = format;
    info->bytesPerPixel = i->size;
    info->bitsPerPixel  = i->bitsPerPixel;
    info->h_alpha       = i->ah;
    info->l_alpha       = i->al;
    info->h_red         = i->rh;
    info->l_red         = i->rl;
    info->h_green       = i->gh;
    info->l_green       = i->gl;
    info->h_blue        = i->bh;
    info->l_blue        = i->bl;
    info->components    = i->components;

    return NO_ERROR;
}

screenFormat getScreenFormat()
{
    // get format on PixelFormat struct
    PixelFormat f = screenshotClient->getFormat();
    PixelFormatInformation pf;
    getPixelFormatInformation(f, &pf);

    Bpp = bytesPerPixel(f);
    L("Bpp was set to %d\n", Bpp);

    screenFormat format;
    format.bitsPerPixel = bitsPerPixel(f);
    format.width        = screenshotClient->getWidth();
    format.height       = screenshotClient->getHeight();
    format.size         = format.bitsPerPixel * format.width * format.height / CHAR_BIT;
    format.redShift     = pf.l_red;
    format.redMax       = pf.h_red - pf.l_red;
    format.greenShift   = pf.l_green;
    format.greenMax     = pf.h_green - pf.l_green;
    format.blueShift    = pf.l_blue;
    format.blueMax      = pf.h_blue - pf.l_blue;
    format.alphaShift   = pf.l_alpha;
    format.alphaMax     = pf.h_alpha-pf.l_alpha;

    return format;
}

unsigned int* receiveFrameBuffer()
{
    screenshotClient->update(display, Rect(), false);
    void const* base = screenshotClient->getPixels();
    return (unsigned int*)base;
}

int initFlinger(void)
{
    L("Preparing thread pool for screen capturing\n");
    ProcessState::self()->startThreadPool();

    screenshotClient = new ScreenshotClient();
    L("Detected screen format: %d\n", screenshotClient->getFormat());

    int errcode = screenshotClient->update(display, Rect(), false);
    if (display == NULL || errcode != NO_ERROR)
    {
        L("Error: flinger initialization failed\n");
        return -1;
    }

    screenformat = getScreenFormat();
    if (screenformat.width <= 0) {
        L("Error: received a bad screen size from flinger\n");
        return -1;
    }

    unsigned int* buffer = receiveFrameBuffer();
    if (buffer == NULL) {
        L("Error: Could not read surfaceflinger buffer\n");
        return -1;
    }

    L("Initialization successful\n");
    return 0;
}

unsigned int* readBuffer()
{
    screenshotClient->update(display, Rect(), false);
    void const* base = 0;
    uint32_t w, h, s;

    base = screenshotClient->getPixels();
    w = screenshotClient->getWidth();
    h = screenshotClient->getHeight();
    s = screenshotClient->getStride();

    if (s > w)
    {
        // If stride is greater than width, then the image is non-contiguous in memory
        // so we have copy it into a new array such that it is
        void *new_base = malloc(w * h * Bpp);
        void *tmp_ptr = new_base;

        for (size_t y = 0; y < h; y++)
        {
            memcpy(tmp_ptr, base, w * Bpp);
            // Pointer arithmetic on void pointers is frowned upon, apparently.
            tmp_ptr = (void *)((char *)tmp_ptr + w * Bpp);
            base = (void *)((char *)base + s * Bpp);
        }

        return (unsigned int*) new_base;
    }

    return (unsigned int*) base;
}

void closeFlinger()
{
    delete screenshotClient;
}
