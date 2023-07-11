/*
	This file is part of Overmix.

	Overmix is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Overmix is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Overmix.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ImageEx.hpp"

#include "../color.hpp"
#include "../debug.hpp"
#include "../utils/PlaneUtils.hpp"

#include <QIODevice>

#include <libraw/libraw.h>

using namespace std;
using namespace Overmix;


bool ImageEx::from_libraw( QIODevice& dev ){
	Timer t( "from_libraw" );
	
	auto buffer = dev.readAll();
	
	LibRaw_buffer_datastream datastream(buffer.data(), buffer.count());
	
	LibRaw loader;
	
	if (loader.open_datastream(&datastream) != 0)
		return false;
	
	if (loader.unpack() != 0)
		return false;
	
	color_space = {Transform::BAYER, Transfer::LINEAR};
	
	unsigned width  = loader.imgdata.rawdata.sizes.width/2;
	unsigned height = loader.imgdata.rawdata.sizes.height/2;
	unsigned off_x = loader.imgdata.rawdata.sizes.left_margin/2;
	unsigned off_y = loader.imgdata.rawdata.sizes.top_margin/2;
	unsigned stride = loader.imgdata.rawdata.sizes.raw_pitch / sizeof(uint16_t);
	for( int i=0; i<4; i++ )
		planes.emplace_back( Plane{width, height} );
	
	struct Rggb{
		uint16_t r, g1, g2, b;
	};
	
	auto GetRggb = [&](int x, int y){
		return Rggb{loader.imgdata.rawdata.raw_image[((x+off_x)*2+0) + ((y+off_y)*2+0)*stride]
			,        loader.imgdata.rawdata.raw_image[((x+off_x)*2+1) + ((y+off_y)*2+0)*stride]
			,        loader.imgdata.rawdata.raw_image[((x+off_x)*2+0) + ((y+off_y)*2+1)*stride]
			,        loader.imgdata.rawdata.raw_image[((x+off_x)*2+1) + ((y+off_y)*2+1)*stride]
			};
	};
	
	Rggb max_values = {0, 0, 0, 0};
	for (int iy=0; iy<height; iy++)
		for (int ix=0; ix<width; ix++)
		{
			auto val = GetRggb(ix, iy );
			max_values.r  = max(max_values.r,  val.r );
			max_values.g1 = max(max_values.g1, val.g1);
			max_values.g2 = max(max_values.g2, val.g2);
			max_values.b  = max(max_values.b,  val.b );
		}
	max_values.r = 65535;
	max_values.g1 = 65535;
	max_values.g2 = 65535;
	max_values.b = 65535;
		
	Rggb black = {(uint16_t)loader.imgdata.color.cblack[0], (uint16_t)loader.imgdata.color.cblack[1], (uint16_t)loader.imgdata.color.cblack[2], (uint16_t)loader.imgdata.color.cblack[3]};
	auto sub = [](int val, int subtract){ return max(0, val - subtract); };
	max_values.r  = sub(max_values.r , black.r );
	max_values.g1 = sub(max_values.g1, black.g1);
	max_values.g2 = sub(max_values.g2, black.g2);
	max_values.b  = sub(max_values.b , black.b );
	
	for (int iy=0; iy<height; iy++)
		for (int ix=0; ix<width; ix++)
		{
			auto rggb = GetRggb(ix, iy);
			planes[0].p[iy][ix] = color::fromDouble(sub(rggb.r , black.r ) / (double)max_values.r );
			planes[1].p[iy][ix] = color::fromDouble(sub(rggb.g1, black.g1) / (double)max_values.g1);
			planes[2].p[iy][ix] = color::fromDouble(sub(rggb.g2, black.g2) / (double)max_values.g2);
			planes[3].p[iy][ix] = color::fromDouble(sub(rggb.b , black.b ) / (double)max_values.b );
		}
	
	return true;
}


