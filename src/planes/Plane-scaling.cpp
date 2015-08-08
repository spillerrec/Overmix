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

#include <boost/math/constants/constants.hpp>

using namespace std;

Plane Plane::scale_nearest( Point<unsigned> wanted ) const{
	Timer t( "scale_nearest" );
	Plane scaled( wanted );
	
	for( unsigned iy=0; iy<wanted.height(); iy++ ){
		color_type* row = scaled.scan_line( iy );
		for( unsigned ix=0; ix<wanted.width(); ix++ )
			row[ix] = pixel( (Size<double>( ix, iy ) / (wanted-1) * (getSize()-1)).round() );
	}
	
	return scaled;
}

double Plane::linear( double x ){
	x = abs( x );
	if( x <= 1.0 )
		return 1.0 - x;
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

double Plane::lancozs( double x, int a ){
	auto pi = boost::math::constants::pi<double>();
	if( x >= a )
		return 0;
	else if( std::abs(x) < std::numeric_limits<double>::epsilon() )
		return 1;
	else
		return (a*std::sin(pi*x)*std::sin(pi*x/2)) / (pi*pi*x*x);
}

template<typename T>
T sum( const std::vector<T>& arr )
	{ return std::accumulate( arr.begin(), arr.end(), T() ); }

struct ScalePoint{
	unsigned start;
	vector<float> weights; //Size of this is end
	
	ScalePoint( unsigned index, unsigned width, unsigned wanted_width, double offset, double window, Plane::Filter f ){
		auto new_window = (wanted_width > width) ? window : window * width / wanted_width;
		auto scale = window / new_window;
		double pos = (index+0.5+offset) / wanted_width * width;
		start = (unsigned)max( (int)ceil( pos-new_window ), 0 );
		unsigned end = min( (unsigned)floor( pos+new_window ), width );
		
		weights.reserve( end - start );
		for( unsigned j=start; j<end; ++j )
			weights.push_back( f( (pos - (j + 0.5))*scale ) );
		
		//Normalize so we don't have to divide later
		auto total = sum( weights );
		for( auto& w : weights )
			w /= total;
	}
};

struct ScaleLine{
	const std::vector<ScalePoint>& points;
	Plane& wanted;
	const Plane& input;
	
	Plane::Filter f;
	float offset;
	float window;
	
	unsigned index;
	
	ScaleLine( const std::vector<ScalePoint>& points, Plane &wanted, unsigned index
		,	const Plane& input, float offset, float window, Plane::Filter f )
		:	points(points), wanted(wanted), input(input), f(f), offset(offset), window(window), index(index)
		{ }
		
	void do_line() const;
};

void ScaleLine::do_line() const{
	color_type *out = wanted.scan_line( index );
	ScalePoint ver( index, input.get_height(), wanted.get_height(), offset, window, f );
	
	for( auto& x : points ){
		double avg = 0;
		auto row = input.const_scan_line( ver.start ) + x.start;
		
		for( auto wy : ver.weights ){
			auto row2 = row;
			
			double local_avg = 0;
			for( auto wx : x.weights )
				local_avg += *(row2++) * wx;
			avg += local_avg * wy;
			
			row += input.get_line_width();
		}
		
		//*(out++) = color::truncate( avg + 0.5 );
		*(out++) = std::min( std::max( avg, (double)color::BLACK ), (double)color::WHITE );
	}
}


static Plane scale2x( const Plane& p, int window, Plane::Filter f ){
	Timer t( "scale2x" );
	Plane scaled( p.getSize() * 2 );
	
	auto w1 = ScalePoint( 100, p.get_width(), scaled.get_width(), 0, 2.5, f ).weights;
	auto w2 = ScalePoint( 101, p.get_width(), scaled.get_width(), 0, 2.5, f ).weights;
	
	auto line_width = scaled.get_line_width();
	auto column = [&]( const color_type* row ){
			auto sum = std::make_pair( 0.0f, 0.0f );
			for( int i=0; i<4; i++ ){
				auto val = *(row + i*line_width);
				sum.first  += val * w1[i];
				sum.second += val * w2[i];
			}
			return sum;
		};
	
	for( unsigned iy=4; iy<scaled.get_height()-4; iy+=2 ){
		auto out1 = scaled.scan_line( iy   );
		auto out2 = scaled.scan_line( iy+1 );
		auto in  = p.const_scan_line( iy/2-2 );
		
		std::pair<float,float> c[4];
		c[0] = column( in++ );
		c[1] = column( in++ );
		c[2] = column( in++ );
		c[3] = column( in++ );
		auto move = [&](){
			c[0] = c[1];
			c[1] = c[2];
			c[2] = c[3];
			c[3] = column( in++ );
		};
		
		for( unsigned ix=2; ix<scaled.get_width()-4; ix+=2 ){
			out1[ix+0]=c[0].first*w1[0] + c[1].first*w1[1] + c[2].first*w1[2];
			out1[ix+1]=c[0].first*w2[0] + c[1].first*w2[1] + c[2].first*w2[2];
			
			out2[ix+0]=c[0].second*w1[0]+ c[1].second*w1[1]+c[2].second*w1[2];
			out2[ix+1]=c[0].second*w2[0]+ c[1].second*w2[1]+c[2].second*w2[2];
			move();
		}
	}
	
	return scaled;
}


Plane Plane::scale_generic( Point<unsigned> wanted, double window, Plane::Filter f, Point<double> offset ) const{
	Timer t( "scale_generic" );
	if( wanted == getSize() )
		return *this;
	
	Plane scaled( wanted );
	
	
	//Calculate all x-weights
	std::vector<ScalePoint> points;
	points.reserve( wanted.width() );
	for( unsigned ix=0; ix<wanted.width(); ++ix )
		points.emplace_back( ix, size.width(), wanted.width(), offset.x, window, f );
	
	//Calculate all y-lines
	std::vector<ScaleLine> lines;
	lines.reserve( wanted.height() );
	for( unsigned iy=0; iy<wanted.height(); ++iy )
		lines.emplace_back( points, scaled, iy, *this, offset.y, window, f );
	
   QtConcurrent::blockingMap( lines, []( ScaleLine& t ){ t.do_line(); } );
   //for( auto l : lines ) l.do_line();
	
	return scaled;
}



