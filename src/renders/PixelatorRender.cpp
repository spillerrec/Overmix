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

using Color = precision_color_type;
using Histogram = vector<Color>;

Color spacing_eval( const Histogram& h, int offset, int spacing ){
	Color sum = color::BLACK;
	int amount=0;
	for( unsigned i=offset; i<h.size(); i+=spacing, amount++ )
		sum += h[i];
	return amount ? sum / amount : color::BLACK;
}

void normalize( Histogram& h ){
	Color sum = 0;
	for( auto val : h )
		sum += val;
	auto avg = sum / h.size();
	for( auto& val : h )
		val = val < avg ? color::BLACK : color::WHITE/2;
}

ImageEx PixelatorRender::render( const AContainer& aligner, AProcessWatcher* watcher ) const{
	auto avg = AverageRender().render( aligner );
	if( !avg.is_valid() )
		return ImageEx();
	
	//Get edge detected image
	ImageEx gray( avg );
	gray.to_grayscale();
	auto edges = gray[0].scale_cubic( gray[0].getSize()*2 ).edge_laplacian();
	
	Histogram histo_h( edges.get_width() , color::BLACK );
	Histogram histo_v( edges.get_height(), color::BLACK );
	
	//Get histogram for vertical/horizontal lines in the image
	for( unsigned iy=0; iy<histo_v.size(); iy++ )
		for( unsigned ix=0; ix<edges.get_width(); ix++ )
			histo_v[iy] += edges.pixel( {ix, iy} );
	for( unsigned ix=0; ix<histo_h.size(); ix++ )
		for( unsigned iy=0; iy<edges.get_height(); iy++ )
			histo_h[ix] += edges.pixel( {ix, iy} );
	
//	normalize( histo_v );
//	normalize( histo_h );
	
	for( unsigned iy=0; iy<histo_v.size(); iy++ )
		for( unsigned ix=0; ix<histo_h.size(); ix++ )
			edges.setPixel( {ix, iy}, histo_v[iy] + histo_h[ix] );
//	auto mean = edges.mean_value();
//	edges.binarize_threshold( mean );
	
//	for( auto& val : histo_v )
//		val = val < mean ? color::BLACK : color::WHITE;
//	for( auto& val : histo_h )
//		val = val < mean ? color::BLACK : color::WHITE;
	
	int v_offset=0, v_spacing=1;
	int h_offset=0, h_spacing=1;
	//*
	Color v_best=color::BLACK;
	Color h_best=color::BLACK;
	for( unsigned is=2; is<histo_v.size(); is++ )
		for( unsigned io=0; io<is; io++ ){
			auto v_current = spacing_eval( histo_v, io, is );
			auto h_current = spacing_eval( histo_h, io, is );
			
			/*
			if( v_current > v_best ){
				v_best = h_current;
				v_offset = io;
				v_spacing = is;
			}
			if( h_current > h_best ){
				h_best = h_current;
				h_offset = io;
				h_spacing = is;
			}
			/*/
			auto current = v_current + h_current;
			if( current > v_best ){
				v_best = current;
				v_offset = h_offset = io;
				v_spacing = h_spacing = is;
			}
			//*/
		}
	//*/
	
	v_offset=8, v_spacing=20;
	h_offset=5, h_spacing=17;
	//*
	edges.fill( color::BLACK );
	for( unsigned iy=0; iy<histo_v.size(); iy++ )
		for( unsigned ix=0; ix<histo_h.size(); ix++ ){
			int hits = 0;
			if( (v_spacing+iy-v_offset)%v_spacing == 0 ) hits++;
			if( (h_spacing+ix-h_offset)%h_spacing == 0 ) hits++;
			edges.setPixel( {ix, iy}, (hits / 2.0) * color::WHITE );
		}
	//*/
	//return ImageEx( edges );
	
	for( unsigned i=0; i<avg.size(); i++ ){
		PlaneBase<precision_color_type> pixels( avg.getSize() );
		PlaneBase<int> amounts( avg.getSize() );
		pixels.fill( 0 );
		amounts.fill( 0 );
		auto& p = avg[i];
		
		auto scale = p.getSize().to<double>() / avg.getSize() * 2;
		
		for( unsigned iy=0; iy<p.get_height(); iy++ )
			for( unsigned ix=0; ix<p.get_width(); ix++ ){
				auto jy = int(v_spacing+(iy*scale.y)-v_offset)/v_spacing;
				auto jx = int(h_spacing+(ix*scale.x)-h_offset)/h_spacing;
				pixels.setPixel( {jx, jy}, pixels.pixel( {jx, jy} ) + p.pixel( { ix, iy } ) );
				amounts.setPixel( {jx, jy}, amounts.pixel( {jx, jy} ) + 1 );
			}
		
		for( unsigned iy=0; iy<p.get_height(); iy++ )
			for( unsigned ix=0; ix<p.get_width(); ix++ ){
				auto jy = int(v_spacing+(iy*scale.y)-v_offset)/v_spacing;
				auto jx = int(h_spacing+(ix*scale.x)-h_offset)/h_spacing;
				p.setPixel( {ix, iy}, pixels.pixel( {jx, jy} ) / amounts.pixel( {jx, jy} ) );
			}
		
		/*
		Plane out( Size<int>{ p.get_width()*2/h_spacing + 1, p.get_height()*2/v_spacing + 1 } );
		for( unsigned iy=0; iy<out.get_height(); iy++ )
			for( unsigned ix=0; ix<out.get_width(); ix++ )
				out.setPixel( {ix,iy}, amounts.pixel( {ix, iy} ) != 0 ? pixels.pixel( {ix, iy} ) / amounts.pixel( {ix, iy} ) : color::BLACK );
		return { out };
		avg[i] = out;
		*/
	}
	
	return avg;
}



