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


#include "FloatRender.hpp"
#include "AverageRender.hpp"
#include "../containers/AContainer.hpp"
#include "../planes/ImageEx.hpp"
#include "../planes/basic/distributions.hpp"
#include "../color.hpp"
#include "../utils/AProcessWatcher.hpp"

#include <QDebug>
#include <QRectF>
#include <float.h>
#include <algorithm>
#include <vector>
#include <utility>
using namespace std;
using namespace Overmix;
using PointF = Point<double>;

static double spline( double x ){ return Distributions::cubic( 1.0, 0.0, x ); }

QPointF toQPoint( Point<double> point ){ return QPointF( point.x, point.y ); }
QRectF toQRectF( Point<double> pos, Size<double> size ){ return QRectF( pos.x, pos.y, size.width(), size.height() ); }


class PointRenderBase{
	public:
		struct ValuePos{
			double distance;
			color_type value;
			bool operator<(const ValuePos& other) const{
				return distance < other.distance;
			}
		};
		
	private:
		Point<> pos;

		PointF toAbsolute( PointF pos, PointF offset, PointF scale ) const{ return pos * scale + offset; }
		PointF toRelative( PointF pos, PointF offset, PointF scale ) const{ return (pos - offset) / scale; }
		
	public:
		PointRenderBase( Point<> pos ) : pos(pos) { }
		virtual ~PointRenderBase() = default;
		
		void add_points( const Plane& img, Point<double> offset, Point<double> scale ){
			auto relative = toQRectF( offset, scale*img.getSize() );
			auto window   = toQRectF( pos - scale*2, scale*4 );
			auto usable   = window.intersected( relative );
			
			auto fstart = toRelative( {usable.topLeft()    }, offset, scale );
			auto fend   = toRelative( {usable.bottomRight()}, offset, scale );
			for(    unsigned iy=ceil(fstart.y); iy<floor(fend.y); ++iy )
				for( unsigned ix=ceil(fstart.x); ix<floor(fend.x); ++ix ){
					auto distance = (toAbsolute( PointF( ix, iy ), offset, scale ) - pos) / scale;
					ValuePos p{ sqrt(distance.x*distance.x + distance.y*distance.y), img.pixel( {ix,iy} ) };
					add_point( p );
				}
		}
		
		virtual void add_point( ValuePos p ) = 0;
};

class PointRender : public PointRenderBase{
	protected:
		double sum = 0.0;
		double weight = 0.0;
		
	public:
		explicit PointRender( Point<int> pos ) : PointRenderBase( pos ), sum(0), weight(0) { }
		
		color_type value(){
			return (weight!=0.0) ? color::truncate(sum / weight) : color::BLACK;
		}
		
		virtual void add_point( ValuePos p ) override{
			double w = spline( p.distance );
			sum += p.value * w;
			weight += w;
		}
};

bool isSubpixel( const AContainer& aligner ){
	for( unsigned j=0; j<aligner.count(); ++j ){
		auto pos = aligner.rawPos( j );
		if( pos != pos.round() )
			return true;
	}
	return false;
}


ImageEx FloatRender::render( const AContainer& aligner, AProcessWatcher* watcher ) const{
	//Fall back to AverageRender if no sub-pixel alignment
	if( !isSubpixel( aligner ) ){
		qDebug( "No subpixel, using AverageRender instead" );
		ImageEx render = AverageRender().render( aligner, watcher );
		if( scale != PointF( 1.0, 1.0 ) )
			render.scale( (render.getSize() * scale).round() );
		return render;
	}
	
	//TODO: determine amount of planes!
	unsigned planes_amount = 3; //TODO: alpha?
	
	Progress progress( "FloatRender", planes_amount, watcher );
	
	//Do iterator
	auto full = aligner.size();
	auto color_space = aligner.image(0).getColorSpace();
	ImageEx img( (planes_amount==1) ? color_space.changed( Transform::GRAY ) : color_space );
	
	//Fill alpha
	Plane alpha( full.size*scale );
	alpha.fill( color::WHITE );
	img.alpha_plane() = alpha;
	
	vector<PointRenderBase::ValuePos> points;
	for( unsigned i=0; i<planes_amount; i++ ){
		Plane out( full.size*scale );
		auto plane_progress = progress.makeProgress( "Plane", out.get_height() );
		
		//Pre-calculate scales
		vector<PointF> scales;
		for( unsigned j=0; j<aligner.count(); ++j )
			scales.emplace_back( aligner.image( j )[0].getSize().to<double>() / aligner.image( j )[i].getSize().to<double>() * scale );
		
		for( unsigned iy=0; iy<out.get_height(); ++iy ){
			auto row = out.scan_line( iy );
			for( unsigned ix=0; ix<out.get_width(); ++ix ){
				PointRender p( PointF( ix, iy ) + full.pos*scale/*, points*/ );
				
				for( unsigned j=0; j<aligner.count(); ++j )
					p.add_points( aligner.image( j )[i], aligner.rawPos( j ) * scale, scales[j] );
				
				row[ix] = p.value();
			}
			
			plane_progress.add();
			if( plane_progress.shouldCancel() )
				return {};
		}
		
		img.addPlane( std::move( out ) );
	}
	
	return img;
}



