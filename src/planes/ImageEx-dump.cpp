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
#include "dump/DumpPlane.hpp"

#include <vector>

using namespace std;


template<typename T> void copyLine( color_type* out, const T* in, unsigned width, double scale ){
	for( unsigned ix=0; ix<width; ++ix )
		out[ix] = color::fromDouble( in[ix] / scale );
}

void process_dump_line( color_type *out, const uint8_t* in, unsigned width, uint16_t depth ){
	double scale = pow( 2.0, depth ) - 1.0;
	
	if( depth <= 8 )
		copyLine( out, in, width, scale );
	else
		copyLine( out, reinterpret_cast<const uint16_t*>(in), width, scale );
}

bool ImageEx::read_dump_plane( QIODevice &dev ){
	DumpPlane dump_plane;
	if( !dump_plane.readHeader( dev ) )
		return false;
	
	planes.emplace_back( Size<unsigned>{ dump_plane.getWidth(), dump_plane.getHeight() } );
	//TODO: add assertion that width == line_width !!
	dump_plane.readData( dev, (uint16_t*)planes.back().scan_line(0), 14 ); //TODO: constant with bit depth
	
	
	//Convert data
//	for( auto&& row : planes.back() )
//		process_dump_line( row.line(), dump_plane.constScanline( row.y() ), row.width(), dump_plane.getDepth() );
	
	return true;
}

bool ImageEx::from_dump( QIODevice& dev ){
	Timer t( "from_dump" );
	planes.reserve( 3 );
	while( read_dump_plane( dev ) ); //Load all planes
//	planes.reserve( 1 ); //For benchmarking other stuff
//	read_dump_plane( dev );
	
	//Use last plane as Alpha
	auto amount = planes.size();
	if( amount == 2 || amount == 4 ){
		alpha = std::move( planes.back() );
		planes.pop_back();
		amount--;
	}
	
	//Find type and validate
	type = (amount == 1) ? GRAY : YUV;
	return amount < 4;
}

static DumpPlane toDumpPlane( const Plane& plane, unsigned depth ){
	bool multi_byte = depth > 8;
	auto power = std::pow( 2, depth ) - 1;
	vector<uint8_t> data;
	data.reserve( plane.get_width() * plane.get_height() * (multi_byte?2:1) );
	
	for( auto row : plane )
		for( auto pixel : row ){
			if( multi_byte ){
				uint16_t val = color::asDouble( pixel ) * power;
				data.push_back( val & 0x00FF );
				data.push_back( (val & 0xFF00) >> 8 );
			}
			else
				data.push_back( color::asDouble( pixel ) * power );
	}
	
	return DumpPlane( plane.get_width(), plane.get_height(), depth, data );
}

bool ImageEx::saveDump( QIODevice& dev, unsigned depth, bool compression ) const{
	Timer t( "saveDump" );
	auto method = compression ? DumpPlane::LZMA : DumpPlane::NONE;
	for( auto& plane : planes )
		if( !toDumpPlane( plane, depth ).write( dev, method ) )
			return false;
	
	if( alpha )
		if( !toDumpPlane( alpha, depth ).write( dev, method ) )
			return false;
	
	return true;
}

bool ImageEx::saveDump( QString path, unsigned depth ) const{
	QFile f( path );
	if( !f.open( QIODevice::WriteOnly ) )
		return false;
	
	return saveDump( f, depth, true );
}

