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
#include "../color.hpp"

#include <QDebug>
#include <QRectF>
#include <float.h>
#include <algorithm>
#include <vector>
#include <utility>
using namespace std;
using PointF = Point<double>;

static double cubic( double b, double c, double x ){
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
static double spline( double x ){ return cubic( 1.0, 0.0, x ); }
//TODO: reuse implementation in Plane

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
		vector<ValuePos>& points;
		
	public:
		PointRender( Point<int> pos, vector<ValuePos>& points ) : PointRenderBase( pos ), points(points) {
			points.clear();
		}
		
		color_type value(){
			sort( points.begin(), points.end() );
			double sum = 0.0;
			double weight = 0.0;
			for( unsigned i=0; i<min(points.size(),(vector<ValuePos>::size_type)16); ++i ){
				double w = spline( points[i].distance );
				sum += points[i].value * w;
				weight += w;
			}
			return (weight!=0.0) ? round(sum / weight) : 0;
		}
		
		virtual void add_point( ValuePos p ) override{
			points.push_back( p );
		}
};

class PointRender2 : public PointRenderBase{
	protected:
		double sum = 0.0;
		double weight = 0.0;
		
	public:
		PointRender2( Point<int> pos ) : PointRenderBase( pos ), sum(0), weight(0) { }
		
		color_type value(){
			return (weight!=0.0) ? color::truncate(sum / weight) : color::BLACK;
		}
		
		virtual void add_point( ValuePos p ) override{
			double w = spline( p.distance );
			sum += p.value * w;
			weight += w;
		}
};

class PointRender3 : public PointRenderBase{
	protected:
		ValuePos p{ 99999, 0 };
		
	public:
		PointRender3( Point<int> pos ) : PointRenderBase( pos ) { }
		
		color_type value(){
			return p.value;
		}
		
		virtual void add_point( ValuePos p ) override{
			this->p = min( this->p, p );
		}
		
};

bool isSubpixel( const AContainer& aligner ){
	for( unsigned j=0; j<aligner.count(); ++j ){
		auto pos = aligner.pos( j );
		if( pos != pos.round() )
			return true;
	}
	return false;
}


ImageEx FloatRender::render( const AContainer& aligner, AProcessWatcher* watcher ) const{
	ProgressWrapper progress( watcher );
	
	//Fall back to AverageRender if no sub-pixel alignment
	if( !isSubpixel( aligner ) ){
		qDebug( "No subpixel, using AverageRender instead" );
		ImageEx render = AverageRender().render( aligner );
		if( scale != PointF( 1.0, 1.0 ) )
			render.scale( (render.getSize() * scale).round() );
		return render;
	}
	
	//TODO: determine amount of planes!
	unsigned planes_amount = 3; //TODO: alpha?
	
	//Do iterator
	auto full = aligner.size();
	ImageEx img( (planes_amount==1) ? Transform::GRAY : aligner.image(0).getTransform() );
	
	//Fill alpha
	Plane alpha( full.size*scale );
	alpha.fill( color::WHITE );
	img.alpha_plane() = alpha;
	
	progress.setTotal( planes_amount*1000 );
	
	vector<PointRenderBase::ValuePos> points;
	for( unsigned i=0; i<planes_amount; i++ ){
		Plane out( full.size*scale );
		
		//Pre-calculate scales
		vector<PointF> scales;
		for( unsigned j=0; j<aligner.count(); ++j )
			scales.emplace_back( aligner.image( j )[0].getSize().to<double>() / aligner.image( j )[i].getSize().to<double>() * scale );
		
		for( unsigned iy=0; iy<out.get_height(); ++iy ){
			if( progress.shouldCancel() )
				return ImageEx();
			progress.setCurrent( i*1000 + (iy * 1000 / out.get_height() ) );
			
			color_type* row = out.scan_line( iy );
			for( unsigned ix=0; ix<out.get_width(); ++ix ){
				PointRender2 p( PointF( ix, iy ) + full.pos*scale/*, points*/ );
				
				for( unsigned j=0; j<aligner.count(); ++j )
					p.add_points( aligner.image( j )[i], aligner.pos( j ) * scale, scales[j] );
				
				row[ix] = p.value();
			}
		}
		
		img.addPlane( std::move( out ) );
	}
	
	return img;
}



