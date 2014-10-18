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


#include "AverageRender.hpp"
#include "../color.hpp"
#include "../planes/PlaneBase.hpp"


#include "../aligners/AImageAligner.hpp"
#include "../ImageEx.hpp"

#include <QTime>
#include <QRect>
#include <vector>
using namespace std;


class ScaledPlane{
	private:
		Plane scaled;
		const Plane& original;
		
	public:
		ScaledPlane( const Plane& p, unsigned width, unsigned height ) : original( p ){
			if( p.get_width() != width || p.get_height() != height )
				scaled = p.scale_cubic( width, height );
		}
		
		ScaledPlane( const Plane& p, const Plane& wanted_size )
			: ScaledPlane( p, wanted_size.get_width(), wanted_size.get_height() ) { }
		
		const Plane& operator()() const{ return scaled.valid() ? scaled : original; }
};

class SumPlane {
	//NOTE: we could split this into two classes, specialized in handling precision alpha or not
	private:
		PlaneBase<precision_color_type> sum;
		PlaneBase<precision_color_type> amount;
	
	public:
		SumPlane( unsigned width, unsigned height )
			: sum( width, height ), amount( width, height ){
			sum.fill( 0 );
			amount.fill( 0 );
		}
		
		void addPlane( const Plane& p, unsigned x, unsigned y ){
			//TODO: make multi-threaded //NOTE: haven't been working out too well...
			for( unsigned iy=0; iy<p.get_height(); iy++ ){
				//Add to sum
				auto in = p.const_scan_line( iy );
				auto out = sum.scan_line( iy + y );
				for( unsigned ix=0; ix<p.get_width(); ix++ )
					out[ix+x] += in[ix];
					
				//Add a full amount to amount
				auto a = amount.scan_line( iy+y );
				for( unsigned ix=0; ix<p.get_width(); ix++ )
					a[ix+x] += color::WHITE;
			}
		}
		
		void addAlphaPlane( const Plane& p, const Plane& alpha, unsigned x, unsigned y ){
			//Scale alpha if needed
			ScaledPlane alpha_scaled( alpha, p );
			
			for( unsigned iy=0; iy<p.get_height(); iy++ ){
				//Add to sum
				auto in = p.const_scan_line( iy );
				auto out = sum.scan_line( iy + y ) + x;
				auto a_in = alpha_scaled().const_scan_line( iy );
				auto a_out = amount.scan_line( iy+y ) + x;
				
				for( unsigned ix=0; ix<p.get_width(); ix++ ){
					auto a_val = a_in[ix];
					out[ix] += in[ix] * color::asDouble( a_val );
					a_out[ix] += a_val;
				}
			}
		}
		
		Plane average() const{
			Plane avg( sum.get_width(), sum.get_height() );
			
			for( unsigned iy=0; iy<avg.get_height(); iy++ ){
				auto sums = sum.const_scan_line( iy );
				auto amounts = amount.const_scan_line( iy );
				auto out = avg.scan_line( iy );
				
				//Calculate the average for each point using sum and amount
				for( unsigned ix=0; ix<avg.get_width(); ix++ )
					if( amounts[ix] != 0 )
						out[ix] = sums[ix] / ((double)amounts[ix] / color::WHITE);
					else
						out[ix] = color::BLACK;
			}
			
			return avg;
		}
		
		Plane alpha() const{
			Plane alpha( amount.get_width(), amount.get_height() );
			
			for( unsigned iy=0; iy<alpha.get_height(); iy++ ){
				auto amounts = amount.const_scan_line( iy );
				auto out = alpha.scan_line( iy );
				
				//Set to transparent if nothing have been added here
				for( unsigned ix=0; ix<alpha.get_width(); ix++ )
					out[ix] = amounts[ix] != 0 ? color::WHITE : color::BLACK;
			}
			
			return alpha;
		}
};

ImageEx AverageRender::render( const AContainer& aligner, unsigned max_count, AProcessWatcher* watcher ) const{
	QTime t;
	t.start();
	if( max_count > aligner.count() )
		max_count = aligner.count();
	
	//Abort if no images
	if( max_count == 0 ){
		qWarning( "No images to render!" );
		return ImageEx();
	}
	unsigned planes_amount = (for_merging || aligner.image(0).get_system() == ImageEx::GRAY) ? 1 : 3;
	qDebug( "render_image: image count: %d", (int)max_count );
	
	//Determine if we need to care about alpha per plane
	bool use_plane_alpha = false;
	for( unsigned i=0; i<max_count; ++i )
		if( aligner.alpha( i ) ){
			use_plane_alpha = true;
			break;
		}
	
	ImageEx img( (planes_amount==1) ? ImageEx::GRAY : aligner.image(0).get_system() );
	img.create( 1, 1 ); //TODO: set as initialized
	
	QRect full = aligner.size();
	auto min_point = aligner.minPoint();
	for( unsigned c=0; c<planes_amount; c++ ){
		//Determine local size
		double scale_x = upscale_chroma ? 1 : (double)aligner.image( 0 )[c].get_width() / aligner.image( 0 )[0].get_width();
		double scale_y = upscale_chroma ? 1 : (double)aligner.image( 0 )[c].get_height() / aligner.image( 0 )[0].get_height();
		
		
		//TODO: something is wrong with the rounding, chroma-channels are slightly off
		QRect local( 
				(int)round( full.x()*scale_x )
			,	(int)round( full.y()*scale_y )
			,	(int)round( full.width()*scale_x )
			,	(int)round( full.height()*scale_y )
			);
		
		SumPlane sum( local.width(), local.height() );
		
		for( unsigned j=0; j<max_count; j++ ){
			QPoint pos(
					round( (aligner.pos(j).x() - min_point.x())*scale_x )
				,	round( (aligner.pos(j).y() - min_point.y())*scale_y ) );
			ScaledPlane plane( aligner.image( j )[c]
				,	aligner.image( j )[0].get_width()*scale_x
				,	aligner.image( j )[0].get_height()*scale_y
				);
			
			const Plane& alpha_plane = aligner.alpha( j );
			if( use_plane_alpha && alpha_plane.valid() )
				sum.addAlphaPlane( plane(), alpha_plane, pos.x(), pos.y() );
			else
				sum.addPlane( plane(), pos.x(), pos.y() );
		}
		
		img[c] = sum.average();
		
		if( c == 0 )
			img.alpha_plane() = sum.alpha();
			//TODO: what to do about the rest? We should try to fill in the gaps?
	}
	
	return img;
}



