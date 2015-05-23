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
	return (in > color::WHITE) ? color::WHITE : (in < color::BLACK ? color::BLACK : in);
}

Plane EstimatorRender::degrade( const Plane& original, const AContainer &group, int channel ) const{
	Plane out( original );
	Plane back( group.image(1)[channel] );
	double amount = 0.95;
	
	for( unsigned iy=0; iy<out.get_height(); iy++ ){
		auto row_out = out.scan_line( iy );
		auto row_back  = back.const_scan_line( iy );
		for( unsigned ix=0; ix<out.get_width(); ix++ )
			row_out[ix] = truncate( (row_out[ix] * amount + row_back[ix]) / (1 + amount) );
	}
	
	return out;
}

static float signFloat( float a, float b ){ return (a>b) ? 1.0f : (a<b) ? -1.0f : 0.0f; }

void sign( Plane& out, const Plane& p1, const Plane& p2, double beta ){
	for( unsigned iy=0; iy<out.get_height(); iy++ ){
		auto row_out = out.scan_line( iy );
		auto row_p1  = p1.const_scan_line( iy );
		auto row_p2  = p2.const_scan_line( iy );
		for( unsigned ix=0; ix<out.get_width(); ix++ )
			row_out[ix] = truncate( row_out[ix] - signFloat( row_p1[ix], row_p2[ix] ) * beta );
	}
}


ImageEx EstimatorRender::render(const AContainer &group, AProcessWatcher *watcher) const {
	if( group.count() != 2 )
		return {};
	
	auto planes_amount = group.image(0).size();
	ImageEx img( planes_amount!=1 ? group.image(0).get_system() : ImageEx::GRAY );
	
	auto est = group.image(0); //Starting estimate
	for( unsigned c=0; c<planes_amount; ++c ){
		auto output = est[c];

		for( int i=0; i<iterations; i++ ){
			qDebug() << "Starting iteration " << i;
			auto output_copy = output;
			//for( const auto& lr : lowres )
			//	sign( output, degrade( output_copy ), lr, beta );
				sign( output, save( degrade( output_copy, group, c ), QString::number(i) ), group.image(0)[c], color::WHITE * (1.3/255) );
				//output -= (sign( output_copy * lr.dhf, lr.img ) * lr.dhf.transpose()) * beta;
		}
		
		img.addPlane( std::move(output) );
	}

	return img;
}