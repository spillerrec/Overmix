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

#include <QtConcurrent>
#include <QDebug>
#include <QTime>

using namespace std;

Plane Plane::scale_nearest( Point<unsigned> wanted ) const{
	Plane scaled( wanted );
	
	for( unsigned iy=0; iy<wanted.height(); iy++ ){
		color_type* row = scaled.scan_line( iy );
		for( unsigned ix=0; ix<wanted.height(); ix++ )
			row[ix] = pixel( (Size<double>( ix, iy ) / (wanted-1) * (getSize()-1)).round() );
	}
	
	return scaled;
}

double Plane::linear( double x ){
	x = abs( x );
	if( x <= 1.0 )
		return x;
	return 0;
}

double Plane::cubic( double b, double c, double x ){
	x = abs( x );
	
	if( x < 1 )
		return
				(12 - 9*b - 6*c)/6 * x*x*x
			+	(-18 + 12*b + 6*c)/6 * x*x
			+	(6 - 2*b)/6
			;
	else if( x < 2 )
		return
				(-b - 6*c)/6 * x*x*x
			+	(6*b + 30*c)/6 * x*x
			+	(-12*b - 48*c)/6 * x
			+	(8*b + 24*c)/6
			;
	else
		return 0;
}


struct ScalePoint{
	unsigned start;
	vector<float> weights; //Size of this is end
	
	ScalePoint( unsigned index, unsigned width, unsigned wanted_width, double window, Plane::Filter f ){
      double pos = ((double)index / (wanted_width-1)) * (width-1);
		start = (unsigned)max( (int)ceil( pos-window ), 0 );
		unsigned end = min( (unsigned)floor( pos+window ), width-1 );
		
		weights.reserve( end - start + 1 );
		for( unsigned j=start; j<=end; ++j )
			weights.push_back( f( pos - j ) );
	}
};

struct ScaleLine{
	std::vector<ScalePoint>& points;
	Plane& wanted;
	unsigned index;
	unsigned width;
	
	const color_type* row; //at (0,index)
	unsigned line_width;
	
	ScalePoint ver;
	
	ScaleLine( std::vector<ScalePoint>& points, Plane &wanted, unsigned index, unsigned width
		,	const Plane& input, double window, Plane::Filter f )
		:	points(points), wanted(wanted), index(index), width(width)
		,	row( input.const_scan_line( index ) ), line_width( input.get_line_width() )
		,	ver( index, width, wanted.get_height(), window, f )
		{ }
};

void do_line( const ScaleLine& line ){
	color_type *out = line.wanted.scan_line( line.index );
	
	for( auto& x : line.points ){
		float avg = 0;
		float amount = 0;
      int offset = ((int)line.index-line.ver.start)*line.line_width;
      auto row = line.row - offset + x.start;
		
		for( auto wy : line.ver.weights ){
			auto row2 = row;
			for( auto wx : x.weights ){
				float weight = wy * wx;
				avg += *(row2++) * weight;
				amount += weight;
			}
			
			row += line.line_width;
		}
		
		*(out++) = amount != 0.0 ? color::truncate( avg / amount + 0.5 ) : color::BLACK;
	}
}

Plane Plane::scale_generic( Point<unsigned> wanted, double window, Plane::Filter f ) const{
	Plane scaled( wanted );
	
	QTime t;
	t.start();
	
	//Calculate all x-weights
	std::vector<ScalePoint> points;
	points.reserve( wanted.width() );
	for( unsigned ix=0; ix<wanted.width(); ++ix )
		points.emplace_back( ix, size.width(), wanted.width(), window, f );
	
	//Calculate all y-lines
	std::vector<ScaleLine> lines;
	lines.reserve( wanted.height() );
	for( unsigned iy=0; iy<wanted.height(); ++iy )
		lines.emplace_back( points, scaled, iy, size.height(), *this, window, f );
	
   QtConcurrent::blockingMap( lines, &do_line );
   //for( auto l : lines ) do_line( l );
	
	qDebug( "Resize took: %d msec", t.restart() );
	return scaled;
}



