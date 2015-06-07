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
	//ImageEx( p ).to_qimage( ImageEx::SYSTEM_KEEP ).save( name + ".png" );
	return p;
}

double truncate( double in ){
	return (in < color::BLACK ? color::BLACK : in);
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
	
	out.crop( (para.container.pos(para.index)-para.min_point)*upscale_factor
		, para.container.image(para.index)[para.channel].getSize()*upscale_factor );
	out = out.scale_cubic( out.getSize() / upscale_factor ); //TODO: offset
	
	return out;
}

static float signFloat( float a, float b, float c ){ return (a>b) ? c : (a<b) ? -c : 0.0f; }

void sign( Plane& out, const Plane& p1, const Plane& p2, Point<double> offset, double beta, int scale ){
	Point<int> pos = offset * scale;
	qDebug() << "pos: " << pos.x << "x" << pos.y;
	unsigned total_change = 0;
	for( unsigned iy=0; iy<p1.get_height()*scale; iy++ ){
		auto row_out = out.scan_line( iy+pos.y );
		auto row_p1  = p1.const_scan_line( iy/scale );
		auto row_p2  = p2.const_scan_line( iy/scale );
		for( unsigned ix=0; ix<p1.get_width()*scale; ix++ ){
			auto sign = signFloat( row_p1[ix/scale], row_p2[ix/scale], beta );
			row_out[ix+pos.x] = truncate( row_out[ix+pos.x] - sign );
			total_change += abs( sign );
		}
	}
	qDebug() << "Change: " << total_change;
}

void regularize( Plane& input, const Plane& copy, int p, double alpha, double beta, double lambda ){
	vector<double> alphas;
//	for( int i=0; i<=(p+p); i++ )
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
			
			output_row[ix] = truncate( output_row[ix] - lambda * sum );
		}
	}
}


ImageEx EstimatorRender::render(const AContainer &group, AProcessWatcher *watcher) const {
//	if( group.count() != 2 )
//		return {};
	
	auto planes_amount = group.image(0).size();
	auto min_point = group.minPoint();
	ImageEx img( planes_amount!=1 ? group.image(0).get_system() : ImageEx::GRAY );
	ProgressWrapper( watcher ).setTotal( planes_amount * iterations );
	
	auto est = AverageRender().render( group ); //Starting estimate
	est.scaleFactor( {upscale_factor,upscale_factor} );
//	est[0].fill( color::WHITE / 2 );
//	est[1].fill( color::WHITE / 2 );
//	est[2].fill( color::WHITE / 2 );
	auto beta = color::WHITE * (1.3/255);
	for( unsigned c=0; c<planes_amount; ++c ){
		auto output = save( est[c], "est" + QString::number(c) );
		save( group.image(1)[c], "back" + QString::number(c) );

		for( int i=0; i<iterations; i++, ProgressWrapper( watcher ).add() ){
			qDebug() << "Starting iteration " << i;
			auto output_copy = output;
			//for( const auto& lr : lowres )
			//	sign( output, degrade( output_copy ), lr, beta );
			//	sign( output, degrade( output_copy, {group.image(1)[c]} ), group.image(0)[c], beta );
				//output -= (sign( output_copy * lr.dhf, lr.img ) * lr.dhf.transpose()) * beta;
			
			for( unsigned j=0; j<group.count(); j++ )
				sign( output, degrade( output_copy, {group, j, c} ), group.image(j)[c], group.pos(j)-min_point
					, beta, upscale_factor );
		//	regularize( output, output_copy, 7, 0.7, beta, 0.03 );
		}
		//save( degrade(output, group, c), "deg" + QString::number(c) );
		//save( output, "end" + QString::number(c) );
		img.addPlane( std::move(output) );
	}

	return img;
}