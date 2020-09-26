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

#include <numeric>

#include <boost/math/constants/constants.hpp>

using namespace std;
using namespace Overmix;

Plane Plane::scale_nearest( Point<unsigned> wanted ) const{
	Timer t( "scale_nearest" );
	Plane scaled( wanted );
	
	auto scale = getSize().to<double>() / wanted;
	for( unsigned iy=0; iy<wanted.height(); iy++ ){
		auto row = scaled.scan_line( iy );
		for( unsigned ix=0; ix<wanted.width(); ix++ )
			row[ix] = pixel( (Size<double>( ix+0.5, iy+0.5 ) * scale).floor() );
	}
	
	return scaled;
}

double Plane::linear( double x ){
	x = abs( x );
	return ( x <= 1.0 ) ? 1.0 - x : 0.0;
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


Plane Plane::scale_generic( Point<unsigned> wanted, double window, Plane::Filter f, Point<double> offset ) const{
	Timer t( "scale_generic" );
	if( !*this || wanted == getSize() )
		return Plane(*this);
	
	Plane scaled( wanted );
	
	//Calculate all x-weights
	std::vector<ScalePoint> points;
	points.reserve( wanted.width() );
	for( unsigned ix=0; ix<wanted.width(); ++ix )
		points.emplace_back( ix, size.width(), wanted.width(), offset.x, window, f );
	
	#pragma omp parallel for
	for( unsigned iy=0; iy<wanted.height(); ++iy ){
		color_type *out = scaled.scan_line( iy ).begin();
		ScalePoint ver( iy, get_height(), scaled.get_height(), offset.y, window, f );
		
		for( auto& x : points ){
			double avg = 0;
			auto row = scan_line( ver.start ).begin() + x.start;
			
			for( auto wy : ver.weights ){
				auto row2 = row;
				
				double local_avg = 0;
				for( auto wx : x.weights )
					local_avg += *(row2++) * wx;
				avg += local_avg * wy;
				
				row += get_line_width();
			}
			
			*(out++) = color::truncate( avg + 0.5 );
		}
	}
	
	return scaled;
}


Plane Plane::scale_generic_alpha( const Plane& alpha, Point<unsigned> wanted, double window, Plane::Filter f, Point<double> offset ) const{
	//Do non-alpha version if alpha is not valid
	if( !alpha.valid() )
		return scale_generic( wanted, window, f, offset );
	//TODO: check alpha size
	
	Timer t( "scale_generic_alpha" );
	if( !*this || wanted == getSize() )
		return Plane(*this);
	
	Plane scaled( wanted );
	
	//Calculate all x-weights
	std::vector<ScalePoint> points;
	points.reserve( wanted.width() );
	for( unsigned ix=0; ix<wanted.width(); ++ix )
		points.emplace_back( ix, size.width(), wanted.width(), offset.x, window, f );
	
	#pragma omp parallel for
	for( unsigned iy=0; iy<wanted.height(); ++iy ){
		auto out = scaled[iy].begin();
		ScalePoint ver( iy, get_height(), scaled.get_height(), offset.y, window, f );
		
		for( auto& x : points ){
			double sum    = 0;
			double amount = 0;
			auto row   = (*this)[ver.start].begin() + x.start;
			auto row_a = alpha  [ver.start].begin() + x.start;
			
			for( auto wy : ver.weights ){
				auto row2   = row  ;
				auto row2_a = row_a;
				
				for( auto wx : x.weights ){
					auto alpha = color::asDouble(*(row2_a++)) * wx * wy;
					sum    += *(row2++) * alpha;
					amount += alpha;
				}
				
				row   +=       get_line_width();
				row_a += alpha.get_line_width();
			}
			
			*(out++) = color::truncate( sum / amount + 0.5 );
		}
	}
	
	return scaled;
}


