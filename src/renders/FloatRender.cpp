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

#include <QTime>
#include <float.h>
#include <algorithm>
#include <vector>
#include <utility>
using namespace std;

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

class PointRenderBase{
	public:
		struct Point{
			double distance;
			color_type value;
			bool operator<(const Point& other) const{
				return distance < other.distance;
			}
		};
		
	private:
		QPoint pos;

		QPointF toAbsolute( QPointF img_pos, QPointF offset, double scale_x, double scale_y ) const{
			return QPointF( img_pos.x()*scale_x, img_pos.y()*scale_y ) + offset;
		}
		QPointF toRelative( QPointF pos, QPointF offset, double scale_x, double scale_y ) const{
			QPointF img_pos( pos - offset );
			return QPointF( img_pos.x()/scale_x, img_pos.y()/scale_y );
		}
		
	public:
		PointRenderBase( int x, int y ) : pos( x, y ) { }
		
		void add_points( const Plane& img, QPointF offset, double scale_x, double scale_y ){
			QRectF relative( offset, QSizeF( img.get_width()*scale_x, img.get_height()*scale_y ) );
			QRectF window( pos - QPointF( 2*scale_x,2*scale_y ), QSizeF( 4*scale_x,4*scale_y ) );
			QRectF usable = window.intersected(relative);
			
			QPointF fstart = toRelative( usable.topLeft(), offset, scale_x, scale_y );
			QPointF fend = toRelative( usable.bottomRight(), offset, scale_x, scale_y );
			for( int iy=ceil(fstart.y()); iy<floor(fend.y()); ++iy )
				for( int ix=ceil(fstart.x()); ix<floor(fend.x()); ++ix ){
					QPointF distance = toAbsolute( QPointF( ix, iy ), offset, scale_x, scale_y ) - pos;
					distance.setX( distance.x() / scale_x );
					distance.setY( distance.y() / scale_y );
					Point p{ sqrt(distance.x()*distance.x() + distance.y()*distance.y()), img.pixel( {ix,iy} ) };
					add_point( p );
				}
		}
		
		virtual void add_point( Point p ) = 0;
};

class PointRender : public PointRenderBase{
	protected:
		vector<Point>& points;
		
	public:
		PointRender( int x, int y, vector<Point>& points ) : PointRenderBase( x, y ), points(points) {
			points.clear();
		}
		
		color_type value(){
			sort( points.begin(), points.end() );
			double sum = 0.0;
			double weight = 0.0;
			for( unsigned i=0; i<min(points.size(),(vector<Point>::size_type)16); ++i ){
				double w = spline( points[i].distance );
				sum += points[i].value * w;
				weight += w;
			}
			return (weight!=0.0) ? round(sum / weight) : 0;
		}
		
		virtual void add_point( Point p ) override{
			points.push_back( p );
		}
};

class PointRender2 : public PointRenderBase{
	protected:
		double sum = 0.0;
		double weight = 0.0;
		
	public:
		PointRender2( int x, int y ) : PointRenderBase( x, y ), sum(0), weight(0) { }
		
		color_type value(){
			return (weight!=0.0) ? color::truncate(sum / weight) : color::BLACK;
		}
		
		virtual void add_point( Point p ) override{
			double w = spline( p.distance );
			sum += p.value * w;
			weight += w;
		}
};

class PointRender3 : public PointRenderBase{
	protected:
		Point p{ 99999, 0 };
		
	public:
		PointRender3( int x, int y ) : PointRenderBase( x, y ) { }
		
		color_type value(){
			return p.value;
		}
		
		virtual void add_point( Point p ) override{
			this->p = min( this->p, p );
		}
		
};

bool isSubpixel( const AContainer& aligner, unsigned max_count ){
	for( unsigned j=0; j<max_count; ++j ){
		auto pos = aligner.pos( j );
		if( pos != pos.round() )
			return true;
	}
	return false;
}


#include <QMessageBox>
ImageEx FloatRender::render( const AContainer& aligner, unsigned max_count, AProcessWatcher* watcher ) const{
	QTime t;
	t.start();
	
	if( max_count > aligner.count() )
		max_count = aligner.count();
	
	//Abort if no images
	if( max_count == 0 ){
		qWarning( "No images to render!" );
		return ImageEx();
	}
	qDebug( "render_image: image count: %d", (int)max_count );
	
	//Fall back to AverageRender if no sub-pixel alignment
	if( !isSubpixel( aligner, max_count ) ){
		ImageEx render = AverageRender().render( aligner );
		if( scale_x != 1.0 && scale_y != 1.0 )
			render.scale( (render.getSize() * Size<double>( scale_x, scale_y )).round() );
		return render;
	}
	
	//TODO: determine amount of planes!
	unsigned planes_amount = 3; //TODO: alpha?
	
	//Do iterator
	QRect full = aligner.size();
	ImageEx img( (planes_amount==1) ? ImageEx::GRAY : aligner.image(0).get_system() );
	img.create( { full.width()*scale_x, full.height()*scale_y } );
	
	//Fill alpha
	Plane alpha( full.width()*scale_x, full.height()*scale_y );
	alpha.fill( color::WHITE );
	img.alpha_plane() = alpha;
	
	if( watcher )
		watcher->setTotal( planes_amount*1000 );
	
	vector<PointRenderBase::Point> points;
	for( unsigned i=0; i<planes_amount; i++ ){
		auto out = img[i];
		
		//Pre-calculate scales
		vector<pair<double,double>> scales;
		for( unsigned j=0; j<max_count; ++j )
			scales.emplace_back(
					(double)aligner.image( j )[0].get_width() / aligner.image( j )[i].get_width() * scale_x
				,	(double)aligner.image( j )[0].get_height() / aligner.image( j )[i].get_height() * scale_y
				);
		
		for( unsigned iy=0; iy<out.get_height(); ++iy ){
			if( watcher )
				watcher->setCurrent( i*1000 + (iy * 1000 / out.get_height() ) );
			
			color_type* row = out.scan_line( iy );
			for( unsigned ix=0; ix<out.get_width(); ++ix ){
				PointRender2 p( ix + full.x()*scale_x, iy + full.y()*scale_y/*, points*/ );
				
				for( unsigned j=0; j<max_count; ++j ){
					QPointF pos( aligner.pos( j ).x * scale_x, aligner.pos( j ).y * scale_y );
					p.add_points( aligner.image( j )[i], pos, scales[j].first, scales[j].second );
				}
				
				row[ix] = p.value();
			}
		}
	}
	
	//QMessageBox::information( nullptr, QString("Float Render time"), QString(to_string( t.elapsed() ).c_str()) );
	qDebug( "float render rest took: %d", t.elapsed() );
	
	return img;
}



