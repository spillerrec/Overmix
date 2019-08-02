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

#include <QImage>

using namespace Overmix;

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

///@param img The image to be pixelated in place
///@param offset The global position of the upper left corner of img
///@param pos The global position of the censored area (upper left corner)
///@param view_size The size of the censored area
///@param pixel_size The size of each mosaic in the censored area
void pixelate( ImageEx& img, Point<double> offset, Point<double> pos, Size<double> view_size, Size<int> pixel_size ){
	//TODO: Support subpixel offsets?
	auto area = img.getArea().intersection(Rectangle<double>(pos-offset, view_size).to<int>());
	
	//Ignore invalid pixel_size
	if( pixel_size.x <= 0 || pixel_size.y <= 0 )
		return;
	//Skip if the area is outside the image
	if( area.size.width()==0 || area.size.height()==0 )
		return;
	
	//Censor the entire image, then crop the wanted part out of it
	auto copy = img;
	for( int y=0; y<(int)img.get_height(); y+=pixel_size.height() )
		for( int x=0; x<(int)img.get_width(); x+=pixel_size.width() )
			blend( copy, {x, y}, pixel_size );
	//TODO: Optimize it so we only censor slightly more than needed.
	//We need a slightly larger area so we get everything properly
	
	img.copyFrom( copy, area.pos, area.size, area.pos );
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
	//	pixelate( copy, pixor_offset, pixor_pos - pos, pixor_size, pixor_pixel );
		copy.to_qimage().save( "out2/" + QString::number( id ) + ".jpg", "JPG", 25 );
	}
}