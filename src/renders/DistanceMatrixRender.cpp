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

#include "DistanceMatrixRender.hpp"

#include "../planes/ImageEx.hpp"
#include "../comparators/AComparator.hpp"
#include "../containers/DelegatedContainer.hpp"
#include "../utils/AProcessWatcher.hpp"
#include "../color.hpp"

#include <QColor>
#include <QRgb>

#include <numeric>
#include <utility>
#include <vector>

using namespace Overmix;

static std::pair<double,double> minmax_error( const AContainer& aligner ){
	double min = std::numeric_limits<double>::max();
	double max = std::numeric_limits<double>::min();
	for( unsigned i=0; i<aligner.count(); i++ )
		for( unsigned j=0; j<aligner.count(); j++ )
			if( aligner.hasCachedOffset( i, j ) ){
				auto offset = aligner.getCachedOffset( i, j );
				if( offset.error < min )
					min = offset.error;
				if( offset.error > max )
					max = offset.error;
			}
	
	return std::make_pair( min, max );
}


ImageEx DistanceMatrixRender::render( const AContainer& aligner, AProcessWatcher* watcher ) const{
	//Color scale
	std::vector<QRgb> palette;
	for( unsigned i=0; i<256; i++ )
		palette.push_back( QColor::fromHsvF( (1.0 - (i/256.0))*240/360, 1.0, 1.0 ).rgb() );
	
	auto minmax = minmax_error( aligner );
	auto min = minmax.first;
	auto max = minmax.second;
	auto index = [=](auto error){ return int((error-min)*255 / (max-min)); };
	auto color = []( QRgb rgb, unsigned c ){
		switch( c ){
			case 0: return qRed( rgb );
			case 1: return qGreen( rgb );
			case 2: return qBlue( rgb );
			default: return 0;
		};
	};
	
	Progress progress( "DistanceMatrix", aligner.count()*aligner.count(), watcher );
	ImageEx out( { Transform::RGB, Transfer::SRGB } );
	for( unsigned c=0; c<3; c++ ){
		Plane p( aligner.count(), aligner.count() );
		for( unsigned iy=0; iy<p.get_height(); iy++ ){
			auto row = p.scan_line(iy);
			for( unsigned ix=0; ix<p.get_width(); ix++ ){
				if( aligner.hasCachedOffset( ix, iy ) ){
					auto offset = aligner.getCachedOffset( ix, iy );
					row[ix] = color::from8bit( color( palette[ index(offset.error) ], c ) );
				}
				else
					row[ix] = color::from8bit( 127 );
				progress.add();
			}
		}
		
		out.addPlane( std::move( p ) );
	}
	
	return out;
}
