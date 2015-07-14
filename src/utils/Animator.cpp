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

#include "Animator.hpp"

#include "../planes/ImageEx.hpp"
#include "../color.hpp"

void blend( ImageEx& img, Point<int> pos, Point<int> size ){
	Point<int> end = img.getSize().min( pos+size );
	pos = pos.max( {0,0} );
	
	//Don't do anything if pixel-size is 0x0
	if( (end-pos) == Point<int>{ 0,0 } )
		return;
	
	for( unsigned c=0; c<img.size(); c++ ){
		auto& p = img[c];
		//Find average color
		/*
		precision_color_type avg = color::BLACK;
		for( int iy=pos.y; iy<end.y; iy++ ){
			auto row = p.scan_line( iy );
			for( int ix=pos.x; ix<end.x; ix++ )
				avg += row[ix];
		}
		avg /= (end-pos).x * (end-pos).y;
		*/
		auto avg = p.pixel( ( pos + (end-pos)/2 ).min( img.getSize() ) );
		
		
		//Fill with average color
		for( int iy=pos.y; iy<end.y; iy++ ){
			auto row = p.scan_line( iy );
			for( int ix=pos.x; ix<end.x; ix++ )
				row[ix] = avg;
		}
	}
}

void pixelate( ImageEx& img, Point<int> offset, Point<double> pos, Size<double> view_size, Size<int> pixel_size ){
	auto end = img.getSize().min( pos+view_size );
	
	//Skip unneeded pixels
	while( (pos+pixel_size).x < 0 )
		pos.x += pixel_size.x;
	while( (pos+pixel_size).y < 0 )
		pos.y += pixel_size.y;
	
	//Align
	pos.x = int(pos.x) / 11 * 11;
	pos.y = int(pos.y) / 11 * 11;
	
	//For each pixel
	for(    int iy=pos.y; iy<end.y; iy+=pixel_size.height() ){
		for( int ix=pos.x; ix<end.x; ix+=pixel_size.width()   ){
			Point<int> p( ix, iy );
			blend( img, p, pixel_size );
		}
	}
}

void Animator::render( const ImageEx& img ) const{
	int id=0;
	Point<int> pixor_offset{ 0,0 };
	Point<double> pixor_pos{ 410.0, 206.0 };
	Size<double> pixor_size{ 130.0, 224.0 };
	Size<int> pixor_pixel{ 11, 11 };
	
	//Area is valid if it is completely inside the area
	//TODO: rectangle contains?
	auto is_valid = [&]( auto pos ){
			return pos.x + view.x < img.get_width()
				&&	pos.y + view.y < img.get_height()
				&& pos.x >= 0 && pos.y >= 0;
		};
	
	for( auto pos=offset; is_valid( pos ); pos += movement, id++ ){
		ImageEx copy( img );
		copy.crop( pos, view );
		pixelate( copy, pixor_offset, pixor_pos - pos, pixor_size, pixor_pixel );
		copy.to_qimage( ImageEx::SYSTEM_REC709 ).save( "out/" + QString::number( id ) + ".bmp" );
	}
}