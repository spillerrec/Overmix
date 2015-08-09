// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "iodevicejpegsourcemanager.h"

// Qt
#include <QIODevice>
#include <QDebug>

// KDE

// libjpeg
#include <stdio.h>
#define XMD_H
extern "C" {
#include <jpeglib.h>
}

// Local

namespace Gwenview
{
namespace IODeviceJpegSourceManager
{

#define SOURCE_MANAGER_BUFFER_SIZE 4096
struct IODeviceJpegSourceManager : public jpeg_source_mgr
{
    QIODevice* mIODevice;
    JOCTET mBuffer[SOURCE_MANAGER_BUFFER_SIZE];
};

static boolean fill_input_buffer(j_decompress_ptr cinfo)
{
    IODeviceJpegSourceManager* src = static_cast<IODeviceJpegSourceManager*>(cinfo->src);
    Q_ASSERT(src->mIODevice);
    int readSize = src->mIODevice->read((char*)src->mBuffer, SOURCE_MANAGER_BUFFER_SIZE);
    if (readSize > 0) {
        src->next_input_byte = src->mBuffer;
        src->bytes_in_buffer = readSize;
    } else {
        /**
         * JPEG file is broken. We feed the decoder with fake EOI, as specified
         * in the libjpeg documentation.
         */
        static JOCTET fakeEOI[2] = { JOCTET(0xFF), JOCTET(JPEG_EOI)};
        qWarning() << "Image is incomplete";
        cinfo->src->next_input_byte = fakeEOI;
        cinfo->src->bytes_in_buffer = 2;
    }
    return true;
}

static void init_source(j_decompress_ptr cinfo)
{
    fill_input_buffer(cinfo);
}

static void skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
    if (num_bytes > 0) {
        while (num_bytes > (long) cinfo->src->bytes_in_buffer) {
            num_bytes -= (long) cinfo->src->bytes_in_buffer;
            fill_input_buffer(cinfo);
            /**
             * we assume that fill_input_buffer will never return FALSE, so
             * suspension need not be handled.
             */
        }
        cinfo->src->next_input_byte += (size_t) num_bytes;
        cinfo->src->bytes_in_buffer -= (size_t) num_bytes;
    }
}

static void term_source(j_decompress_ptr)
{
}

void setup(j_decompress_ptr cinfo, QIODevice* ioDevice)
{
    Q_ASSERT(!cinfo->src);
    IODeviceJpegSourceManager* src = (IODeviceJpegSourceManager*)
                                     (*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                             sizeof(IODeviceJpegSourceManager));
    cinfo->src = src;

    src->init_source = init_source;
    src->fill_input_buffer = fill_input_buffer;
    src->skip_input_data = skip_input_data;
    src->resync_to_restart = jpeg_resync_to_restart;
    src->term_source = term_source;

    src->mIODevice = ioDevice;
}

} // IODeviceJpegSourceManager namespace
} // Gwenview namespace
