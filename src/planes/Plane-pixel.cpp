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

#include "Plane.hpp"
#include "../color.hpp"
#include "../debug.hpp"

#include <QtConcurrent>

using namespace Overmix;

void Plane::for_each_pixel( Plane::PixelFunc1 f, void* data ){
	struct Block{
		RowIt<color_type> row;
		void* data;
		PixelFunc1 f;
	};
	std::vector<Block> lines;
	for( auto row : *this ) lines.push_back( { row, data, f } );
	
	QtConcurrent::blockingMap( lines, [](auto& block){
			for( auto& val : block.row )
				val = block.f( val, block.data );
		} );
}

void Plane::for_each_pixel( const Plane& p, Plane::PixelFunc2 f, void* data ){
	struct Block{
		ZipRowIt<color_type, const color_type> row;
		void* data;
		PixelFunc2 f;
	};
	std::vector<Block> lines;
	for( unsigned iy=0; iy<get_height(); iy++ )
		lines.push_back( { { scan_line(iy), p.scan_line(iy) }, data, f } );
	
	QtConcurrent::blockingMap( lines, [](auto& block){
			for( auto val : block.row )
				val.first = block.f( val.first, val.second, block.data );
		} );
}

void Plane::add( Plane &p ){
	for_each_pixel( p, []( color_type a, color_type b, void* )
		{ return color::truncateFullRange( precision_color_type(a) + b ); } );
}
void Plane::substract( Plane &p ){
	for_each_pixel( p, []( color_type a, color_type b, void* )
		{ return color::truncateFullRange( precision_color_type(b) - a ); } );
}
void Plane::difference( Plane &p ){
	for_each_pixel( p, []( color_type a, color_type b, void* )
		{ return (color_type)std::abs( precision_color_type(b) - a ); } );
}
void Plane::divide( Plane &p ){
	for_each_pixel( p, []( color_type a, color_type b, void* )
		{ return color::fromDouble( color::asDouble( b ) / color::asDouble( a ) ); } );
}
void Plane::multiply( Plane &p ){
	for_each_pixel( p, []( color_type a, color_type b, void* )
		{ return color::fromDouble( color::asDouble( b ) * color::asDouble( a ) ); } );
}


struct LevelOptions{
	color_type limit_min;
	color_type limit_max;
	color_type output_min;
	color_type output_max;
	double gamma;
};

static color_type level_pixel( color_type in, void* data ){
	LevelOptions *opt = static_cast<LevelOptions*>(data);
	
	//Limit
	double scale = 1.0 / ( opt->limit_max - opt->limit_min );
	double val = ( (precision_color_type)in - opt->limit_min ) * scale;
	val = std::min( std::max( val, 0.0 ), 1.0 );
	
	//Gamma
	if( opt->gamma != 1.0 )
		val = std::pow( val, opt->gamma );
	
	//Output
	scale = opt->output_max - opt->output_min;
	return opt->output_min + std::round( val * scale );
}

Plane Plane::level(
		color_type limit_min
	,	color_type limit_max
	,	color_type output_min
	,	color_type output_max
	,	double gamma
	) const{
	Timer t( "levels" );
	Plane out( *this );
	
	//Don't do anything if nothing will change
	if( limit_min == output_min
		&&	limit_max == output_max
		&&	limit_min == color::BLACK
		&&	limit_max == color::WHITE
		&&	gamma == 1.0
		)
		return out;
	
	LevelOptions options = {
				limit_min
			,	limit_max
			,	output_min
			,	output_max
			,	gamma
		};
	
	out.for_each_pixel( &level_pixel, &options );
	
	return out;
}



