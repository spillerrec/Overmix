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

#include "EstimatorRender.hpp"
#include "../planes/Plane.hpp"
#include "../color.hpp"
#include "AverageRender.hpp"
#include "../planes/ImageEx.hpp"
#include "../containers/AContainer.hpp"
#include <QDebug>


using namespace std;

static Plane save( Plane p, QString name ){
//	ImageEx( p ).to_qimage( ImageEx::SYSTEM_KEEP ).save( name + ".png" );
	return p;
}

double truncate( double in ){
	return (in < color::BLACK ? color::BLACK : in);
}

Point<double> channelScale( const AContainer& container, unsigned index, unsigned channel ){
	return container.image(index)[channel].getSize() / container.image(index).getSize().to<double>();
}

struct Parameters{
	//const Plane& background;
	//double overlay{ 0.85 };
	
	const AContainer& container;
	unsigned index;
	unsigned channel;
	Point<double> min_point;
	
	Parameters( const AContainer& container, unsigned index, unsigned channel /*const Plane& background*/ )
		: container(container), index(index), channel(channel) /*background(background)*/ {
			min_point = container.minPoint();
		}
};

Plane EstimatorRender::degrade( const Plane& original, const Parameters& para ) const{
	Plane out( original );
	
	/* Overlay
	for( unsigned iy=0; iy<out.get_height(); iy++ ){
		auto row_out = out.scan_line( iy );
		auto row_back  = para.background.const_scan_line( iy );
		for( unsigned ix=0; ix<out.get_width(); ix++ )
			row_out[ix] = row_out[ix] * (1-para.overlay) + row_back[ix] * (para.overlay);
	}
	*/
	
	//Crop to the area overlapping with our current image
	auto pos = (para.container.pos(para.index)-para.min_point)*upscale_factor*channelScale(para.container, para.index, para.channel);
	out.crop( pos, para.container.image(para.index)[para.channel].getSize()*upscale_factor );
	
	//Degrade - blur
	if( bluring > 0 )
		out = out.blur_gaussian( bluring*upscale_factor, bluring*upscale_factor );
	
	//Degrade - resolution
	out = out.scale_select( out.getSize() / upscale_factor, scale_method/*, pos - pos.to<int>().to<double>()*/ ); //TODO: offset
	
	
	return out;
}

static float signFloat( float a, float b, float c ){ return (a>b) ? c : (a<b) ? -c : 0.0f; }

void sign( Plane& out, const Plane& p1, const Plane& p2, Point<double> offset, double beta, Point<double> scale ){
	Point<int> pos = offset * scale;
	
	unsigned total_change = 0;
	for( unsigned iy=0; iy<p1.get_height()*scale.y; iy++ ){
		auto row_out = out.scan_line( iy+pos.y );
		auto row_p1  = p1.const_scan_line( iy/scale.y );
		auto row_p2  = p2.const_scan_line( iy/scale.y );
		for( unsigned ix=0; ix<p1.get_width()*scale.x; ix++ ){
			auto sign = signFloat( row_p1[int(ix/scale.x)], row_p2[int(ix/scale.x)], beta );
			row_out[ix+pos.x] = truncate( row_out[ix+pos.x] - sign );
			total_change += abs( sign );
		}
	}
	qDebug() << "Change: " << total_change;
}

void regularize( Plane& input, const Plane& copy, int p, double alpha, double beta, double lambda ){
	vector<double> alphas;
	for( int m=0; m<=p; m++ )
		for( int l=p; l+m>=0; l-- )
		alphas.push_back( pow( alpha, abs(m)+abs(l) ) );
	
	for( unsigned iy=p; iy < copy.get_height()-p; iy++ ){
		auto output_row = input.scan_line( iy );
		auto row_0  = copy.const_scan_line( iy );
		for( unsigned ix=p; ix < copy.get_width()-p; ix++ ){
			
			double sum = 0;
			for( int m=0, count=0; m<=p; m++ ){
			auto row_pl = copy.const_scan_line( iy + m );
			auto row_nl = copy.const_scan_line( iy - m );
				for( int l=p; l+m>=0; l-- ){
					sum += alphas[ count++ ]
						* (  signFloat( copy.pixel( {ix, iy} ),     copy.pixel( {ix+m, iy+l} ), beta )
						   - signFloat( copy.pixel( {ix-m, iy-l} ), copy.pixel( {ix, iy} ),     beta ) );
				//		* (signFloat( row_0[ix], row_pl[ix+l], beta )
				//			- signFloat( row_nl[ix-l], row_0[ix, iy], beta ) );
				}
			}
			
			output_row[ix] = truncate( row_0[ix] - lambda * sum );
		}
	}
}


ImageEx EstimatorRender::render(const AContainer &group, AProcessWatcher *watcher) const {
	auto planes_amount = group.image(0).size();
	auto min_point = group.minPoint();
	ProgressWrapper( watcher ).setTotal( planes_amount * iterations * group.count() );
	
	auto est = AverageRender().render( group ); //Starting estimate
	est.scaleFactor( {upscale_factor,upscale_factor} );
	auto beta = color::WHITE * this->beta / group.count();
	for( unsigned c=0; c<planes_amount; ++c ){
		for( int i=0; i<iterations; i++ ){
			auto output_copy = est[c];
			
			//Improve estimate
			for( unsigned j=0; j<group.count(); j++, ProgressWrapper( watcher ).add() )
				sign( output_copy, degrade( est[c], {group, j, c} ), group.image(j)[c], group.pos(j)-min_point
					, beta, channelScale(group, j, c)*upscale_factor );
			
			//Regularization
			if( lambda > 0.0 )
				regularize( est[c], output_copy, reg_size, lambda, beta, alpha );
			else
				est[c] = output_copy;
		}
		
		//DEBUG: See how close our model gets to the input data
		for( unsigned j=0; j<group.count(); j++ ){
			save( degrade( est[c], {group, j, c} ), "deg" + QString::number(c) + "-" + QString::number(j) );
			save( group.image(j)[c],                "low" + QString::number(c) + "-" + QString::number(j) );
		}
	}

	return est;
}