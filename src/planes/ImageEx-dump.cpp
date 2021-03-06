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
using namespace Overmix;


bool ImageEx::read_dump_plane( QIODevice &dev ){
	DumpPlane dump_plane;
	if( !dump_plane.readHeader( dev ) )
		return false;
	
	planes.emplace_back( Plane(Size<unsigned>{ dump_plane.getWidth(), dump_plane.getHeight() }) );
	//TODO: add assertion that width == line_width !!
	//TODO: add assertion that type-size and color depth matches with the shit we are doing here
	dump_plane.readData( dev, (uint16_t*)planes.back().p.scan_line(0).begin(), 14 ); //TODO: constant with bit depth
	
	return true;
}

bool ImageEx::from_dump( QIODevice& dev ){
	bool color_space_set = false;
	if( dev.peek(4) == "DMP0" )
	{
		dev.read(4);
		auto transform = static_cast<Transform>(dev.read(1).at(0));
		auto transfer  = static_cast<Transfer >(dev.read(1).at(0));
		color_space = {transform, transfer};
		color_space_set = true;
	}
	Timer t( "from_dump" );
	planes.reserve( 3 );
	while( read_dump_plane( dev ) ); //Load all planes
//	planes.reserve( 1 ); //For benchmarking other stuff
//	read_dump_plane( dev );
	
	//Use last plane as Alpha
	auto amount = planes.size();
	if( amount == 2 || amount == 4 ){
		alpha = std::move( planes.back().p );
		planes.pop_back();
		amount--;
	}
	
	//Find type and validate
	if( !color_space_set )
		//NOTE: we don't really know the YCbCr format!
		color_space = { (amount == 1) ? Transform::GRAY : Transform::YCbCr_709, Transfer::REC709 };
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
	dev.write( "DMP0" );
	char transform = static_cast<char>(color_space.transform());
	char transfer = static_cast<char>(color_space.transfer());
	dev.write(&transform, 1);
	dev.write(&transfer, 1);
	
	auto method = compression ? DumpPlane::LZMA : DumpPlane::NONE;
	for( auto& info : planes )
		if( !toDumpPlane( info.p, depth ).write( dev, method ) )
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

