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


#include "PixelatorRender.hpp"
#include "AverageRender.hpp"
#include "../color.hpp"
#include "../planes/PlaneBase.hpp"

#include "../aligners/AImageAligner.hpp"
#include "../planes/ImageEx.hpp"

#include <QTime>
#include <vector>
using namespace std;
using namespace Overmix;

using Color = int;
using Histogram = vector<Color>;

void normalize( Histogram& h ){
	Color sum = 0;
	for( auto val : h )
		sum += val;
	auto avg = sum / (Color)h.size();
	for( auto& val : h )
		val = val < avg ? color::BLACK : color::WHITE/2;
}

struct Result{
	//Settings
	int offset;
	int spacing;
	
	//Results
	Color w{ color::BLACK };
	int amount{ 0 };
	
	Result( const Histogram& h, int offset, int spacing ) : offset(offset), spacing(spacing) {
		for( unsigned i=offset; i<h.size(); i+=spacing, amount++ )
			w += h[i];
	}
	
	Color weight() const{ return amount != 0 ? w / amount : color::BLACK; }
	Color calc( Color avg ) const{ return w - amount * avg; }
};

struct ResultsVector{
	vector<Result> results;
	
	ResultsVector( const Histogram& h, unsigned io=8 ){
		//TODO: throw exception on h<=2
		for( unsigned is=2; is<h.size()/4; is++ ) //Pixels at least 2 wide, up-scaling it with 2x to 4px
			for( unsigned io=0; io<is; io++ )
				results.emplace_back( h, io, is );
	}
	
	Color average() const{
		Color sum = color::BLACK;
		for( auto& val : results )
			sum += val.weight();
		return sum / results.size();
	}
	
	Result maximum() const{
		auto avg = average();
		Result best = results[0];
		for( auto result : results )
			if( result.calc(avg) > best.calc(avg) )
				best = result;
		return best;
	}
};


ImageEx PixelatorRender::render( const AContainer& aligner, AProcessWatcher* watcher ) const{
	double upscale = 2.0;
	auto avg = AverageRender().render( aligner );
	if( !avg.is_valid() )
		return ImageEx();
	
	//Get edge detected image
	ImageEx gray( avg );
	gray.to_grayscale();
	auto edges = gray[0].edge_sobel().scale_cubic( gray[0].getSize()*upscale );
	edges.binarize_threshold( edges.mean_value() );
	
	Histogram histo_h( edges.get_width() , color::BLACK );
	Histogram histo_v( edges.get_height(), color::BLACK );
	
	//Get histogram for vertical/horizontal lines in the image
	for( unsigned iy=0; iy<edges.get_height(); iy++ )
		for( unsigned ix=0; ix<edges.get_width(); ix++ ){
			histo_v[iy] += edges.pixel( {ix, iy} );
			histo_h[ix] += edges.pixel( {ix, iy} );
		}
	
	normalize( histo_v );
	normalize( histo_h );
	
	auto v = ResultsVector( histo_v, 8 ).maximum();
	auto h = ResultsVector( histo_h, 5 ).maximum();
	
	for( unsigned i=0; i<avg.size(); i++ ){
		PlaneBase<precision_color_type> pixels( avg.getSize() );
		PlaneBase<int> amounts( avg.getSize() );
		pixels.fill( 0 );
		amounts.fill( 0 );
		auto& p = avg[i];
		
		auto scale = p.getSize().to<double>() / avg.getSize() * upscale;
		
		for( unsigned iy=0; iy<p.get_height(); iy++ )
			for( unsigned ix=0; ix<p.get_width(); ix++ ){
				auto jy = unsigned(v.spacing+(iy*scale.y)-v.offset)/v.spacing;
				auto jx = unsigned(h.spacing+(ix*scale.x)-h.offset)/h.spacing;
				pixels.setPixel( {jx, jy}, pixels.pixel( {jx, jy} ) + p.pixel( { ix, iy } ) );
				amounts.setPixel( {jx, jy}, amounts.pixel( {jx, jy} ) + 1 );
			}
		
		for( unsigned iy=0; iy<p.get_height(); iy++ )
			for( unsigned ix=0; ix<p.get_width(); ix++ ){
				auto jy = unsigned(v.spacing+(iy*scale.y)-v.offset)/v.spacing;
				auto jx = unsigned(h.spacing+(ix*scale.x)-h.offset)/h.spacing;
				p.setPixel( {ix, iy}, pixels.pixel( {jx, jy} ) / amounts.pixel( {jx, jy} ) );
			}
		
		/*
		Plane out( Size<int>{ p.get_width()*2/h.spacing + 1, p.get_height()*2/v.spacing + 1 } );
		for( unsigned iy=0; iy<out.get_height(); iy++ )
			for( unsigned ix=0; ix<out.get_width(); ix++ )
				out.setPixel( {ix,iy}, amounts.pixel( {ix, iy} ) != 0 ? pixels.pixel( {ix, iy} ) / amounts.pixel( {ix, iy} ) : color::BLACK );
		return { out };
		avg[i] = out;
		*/
	}
	
	return avg;
}



