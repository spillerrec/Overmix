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
#ifndef IODEVICEJPEGSOURCEMANAGER_H
#define IODEVICEJPEGSOURCEMANAGER_H

// Qt

// KDE

// Local

class QIODevice;
struct jpeg_decompress_struct;

namespace Gwenview
{

/**
 * This namespace provides a function which makes it possible to decode JPEG
 * files with libjpeg from a QIODevice instance.
 *
 * To use it, simply call setup() to initialize your jpeg_decompress_struct
 * with QIODevice-ready callbacks. The device should be opened for reading.
 */
namespace IODeviceJpegSourceManager
{

void setup(jpeg_decompress_struct* cinfo, QIODevice* ioDevice);

} // namespace
} // namespace

#endif /* IODEVICEJPEGSOURCEMANAGER_H */
