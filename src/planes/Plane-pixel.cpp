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

static void do_pixel_line_single( SimplePixel pix ){
	for( unsigned i=0; i<pix.width; ++i ){
		pix.f( pix );
		++pix.row1;
	}
}
static void do_pixel_line_double( SimplePixel pix ){
	for( unsigned i=0; i<pix.width; ++i ){
		pix.f( pix );
		++pix.row1;
		++pix.row2;
	}
}

void Plane::for_each_pixel( void (*f)( const SimplePixel& ), void *data ){
	std::vector<SimplePixel> lines;
	for( unsigned iy=0; iy<get_height(); ++iy ){
		SimplePixel pix = { scan_line( iy )
			,	nullptr
			,	get_width()
			,	f
			,	data
			};
		lines.push_back( pix );
	}
	
	QtConcurrent::blockingMap( lines, &do_pixel_line_single );
}

void Plane::for_each_pixel( Plane &p, void (*f)( const SimplePixel& ), void *data ){
	if( get_height() != p.get_height() || get_width() != p.get_width() ){
		qWarning( "Plane::for_each_pixel: Planes not equally sized, operation skipped!" );
		return;
	}
	
	std::vector<SimplePixel> lines;
	for( unsigned iy=0; iy<get_height(); ++iy ){
		SimplePixel pix = { scan_line( iy )
			,	p.scan_line( iy )
			,	get_width()
			,	f
			,	data
			};
		lines.push_back( pix );
	}
	
	QtConcurrent::blockingMap( lines, &do_pixel_line_double );
}


static void add_pixel( const SimplePixel& pix ){
	*pix.row1 = std::min( (int)*pix.row2 + (int)*pix.row1, (int)color::WHITE );
}
static void substract_pixel( const SimplePixel& pix ){
	*pix.row1 = std::max( (int)*pix.row2 - (int)*pix.row1, (int)color::BLACK );
}
static void divide_pixel( const SimplePixel& pix ){
	double val1 = color::asDouble( *pix.row1 );
	double val2 = color::asDouble( *pix.row2 );
	*pix.row1 = color::fromDouble( val2 / val1 );
}
static void multiply_pixel( const SimplePixel& pix ){
	double val1 = color::asDouble( *pix.row1 );
	double val2 = color::asDouble( *pix.row2 );
	*pix.row1 = color::fromDouble( val2 * val1 );
}

void Plane::add( Plane &p ){
	for_each_pixel( p, &add_pixel );
}
void Plane::substract( Plane &p ){
	for_each_pixel( p, &substract_pixel );
}
void Plane::difference( Plane &p ){
	for_each_pixel( p, []( const SimplePixel& pix ){
			*pix.row1 = std::abs( *pix.row2 - *pix.row1 );
		} );
}
void Plane::divide( Plane &p ){
	for_each_pixel( p, &divide_pixel );
}
void Plane::multiply( Plane &p ){
	for_each_pixel( p, &multiply_pixel );
}


struct LevelOptions{
	color_type limit_min;
	color_type limit_max;
	color_type output_min;
	color_type output_max;
	double gamma;
};

static void level_pixel( const SimplePixel& pix ){
	LevelOptions *opt = (LevelOptions*)pix.data;
	
	//Limit
	double scale = 1.0 / ( opt->limit_max - opt->limit_min );
	double val = ( (int)*pix.row1 - opt->limit_min ) * scale;
	val = std::min( std::max( val, 0.0 ), 1.0 );
	
	//Gamma
	if( opt->gamma != 1.0 )
		val = std::pow( val, opt->gamma );
	
	//Output
	scale = opt->output_max - opt->output_min;
	*pix.row1 = opt->output_min + std::round( val * scale );
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



