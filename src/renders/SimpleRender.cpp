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


#include "SimpleRender.hpp"
#include "../containers/AContainer.hpp"
#include "../ImageEx.hpp"
#include "../color.hpp"

#include "../MultiPlaneIterator.hpp"

#include <QTime>
#include <QRect>
#include <vector>
using namespace std;

/* MultiPlane layout
 * 
 * extended:
 * 0:  output
 * 1:  output alpha - initialized to color::WHITE
 * 2:  plane 0
 * 3:  plane 1
 * 4:  plane 2
 *   ...
 * 
 * alpha:
 * 0:  output
 * 1:  output alpha - initialized to color::WHITE
 * 3:  plane 0
 * 4:  plane 0 alpha
 * 5:  plane 1
 * 6:  plane 1 alpha
 * 7:  plane 2
 * 8:  plane 2 alpha
 *   ...
 * 
 */

static void render_average_extended( MultiPlaneIterator &it, unsigned offset, AProcessWatcher* watcher ){
	if( it.iterate_all() ){
		it.for_all_pixels( [](MultiPlaneLineIterator &it){
			precision_color_type avg = 0;
			for( unsigned i=2; i<it.size(); ++i )
				avg += it[i];
			
			if( it.size() == 2 )
				it[1] = color::BLACK;
			else
				it[0] = avg / (it.size() - 2);
		}, watcher, offset );
	}
	else{
		it.for_all_pixels( [](MultiPlaneLineIterator &it){
			precision_color_type avg = 0;
			double amount = 0;
			for( unsigned i=2; i<it.size(); ++i )
				if( it.valid( i ) ){
					avg += it[i];
					amount++;
				}
			
			if( amount )
				it[0] = avg / amount;
			else
				it[1] = color::BLACK;
		}, watcher, offset );
	}
}

static void render_average_alpha( MultiPlaneIterator &it, unsigned offset, AProcessWatcher* watcher ){
	it.iterate_all();
	it.for_all_pixels( [](MultiPlaneLineIterator &it){
		precision_color_type avg = 0;
		double amount = 0;

		for( unsigned i=2; i<it.size(); i+=2 ){
			if( it.valid( i ) ){
				if( it.valid( i+1 ) ){
					double w = color::asDouble( it[i+1] );
					avg += it[i] * w;
					amount += w;
				}
				else{
					avg += it[i];
					amount += 1.0;
				}
			}
		}
		
		if( amount ){
			it[0] = avg / amount;
			it[1] = color::WHITE;
		}
		else{
			it[0] = color::BLACK;
			it[1] = color::BLACK;
		}
	}, watcher, offset );
}

static void render_dark_select( MultiPlaneIterator &it, bool alpha_used, unsigned offset, AProcessWatcher* watcher ){
	unsigned plane_stride = alpha_used ? 2 : 1;
	it.data = (void*)&plane_stride;
	
	it.iterate_all(); //No need to optimize this filter
	
	//Do average and store in [0]
	it.for_all_pixels( [](MultiPlaneLineIterator &it){
			unsigned plane_stride = *(unsigned*)it.data;
			//Calculate sum
			color_type min = color::MAX_VAL;
			for( unsigned i=2; i<it.size(); i+=plane_stride ){
				if( it.valid( i ) ){
					if( min > it[i] )
						min = it[i];
				}
			}
			
			it[0] = min;
		}, watcher, offset );
}

ImageEx SimpleRender::render( const AContainer& aligner, unsigned max_count, AProcessWatcher* watcher ) const{
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
	
	//TODO: determine amount of planes!
	unsigned planes_amount = 3;
	if( filter == FOR_MERGING && ( aligner.image(0).get_system() == ImageEx::YUV || aligner.image(0).get_system() == ImageEx::GRAY ) )
		planes_amount = 1;
	
	//Determine if we need to care about alpha per plane
	bool use_plane_alpha = false;
	for( unsigned i=0; i<max_count; ++i )
		if( aligner.image( i ).alpha_plane() ){
			use_plane_alpha = true;
			break;
		}
	
	//Do iterator
	QRect full = aligner.size();
	ImageEx img( (planes_amount==1) ? ImageEx::GRAY : aligner.image(0).get_system() );
	img.create( 1, 1 ); //TODO: set as initialized
	
	//Fill alpha
	Plane alpha( full.width(), full.height() );
	alpha.fill( color::WHITE );
	img.alpha_plane() = alpha;
	
	//Fake alpha
	Plane fake_alpha( aligner.image(0)[0].get_width(), aligner.image(0)[0].get_height() );
	fake_alpha.fill( color::WHITE );
	
	if( watcher )
		watcher->setTotal( planes_amount*2000 );
	for( unsigned i=0; i<planes_amount; i++ ){
		//Determine local size
		double scale_x = (double)aligner.image(0)[i].get_width() / aligner.image(0)[0].get_width();
		double scale_y = (double)aligner.image(0)[i].get_height() / aligner.image(0)[0].get_height();
		
		//TODO: something is wrong with the rounding, chroma-channels are slightly off
		QRect local( 
				(int)round( full.x()*scale_x )
			,	(int)round( full.y()*scale_y )
			,	(int)round( full.width()*scale_x )
			,	(int)round( full.height()*scale_y )
			);
		QRect out_size( upscale_chroma ? full : local );
		
		//Create output plane
		Plane out( out_size.width(), out_size.height() );
		out.fill( 0 );
		
		vector<PlaneItInfo> info;
		info.push_back( PlaneItInfo( out, out_size.x(),out_size.y() ) );
		
		info.push_back( PlaneItInfo( alpha, out_size.x(),out_size.y() ) );
			//TODO: we still have issues with the chroma planes as the
			//up-scaled layers doesn't always cover all pixels in the Y plane.
		
		vector<Plane> temp;
		
		if( out_size == local ){
			for( unsigned j=0; j<max_count; j++ ){
				info.push_back( PlaneItInfo(
						const_cast<Plane&>( aligner.image( j )[i] ) //TODO: FIX!!!
					,	round( aligner.pos(j).x()*scale_x )
					,	round( aligner.pos(j).y()*scale_y )
					) );
				
				if( use_plane_alpha ){
					const Plane& current_alpha = aligner.alpha( j );
					
					info.push_back( PlaneItInfo(
							const_cast<Plane&>(current_alpha ? current_alpha : fake_alpha) //TODO: FIX!!!
						,	round( aligner.pos(j).x()*scale_x )
						,	round( aligner.pos(j).y()*scale_y )
						) );
				}
			}
		}
		else{
			temp.reserve( max_count );
			for( unsigned j=0; j<max_count; j++ ){
				if( watcher )
					watcher->setCurrent( i*2000 + (j * 1000 / max_count) );
				
				Plane p = aligner.image( j )[i].scale_cubic( aligner.image( j )[i].get_width(), aligner.image( j )[i].get_height() );
				QPoint pos = aligner.pos(j).toPoint();
				temp.push_back( p );
				info.push_back( PlaneItInfo( temp[temp.size()-1], pos.x(),pos.y() ) );
				
				if( use_plane_alpha ){
					//Alpha
					const Plane& current_alpha = aligner.alpha( j );
					info.push_back( PlaneItInfo( const_cast<Plane&>(current_alpha ? current_alpha : fake_alpha), pos.x(),pos.y() ) );  //TODO: FIX!!!
				}
			}
		}
		
		MultiPlaneIterator it( info );
		
		unsigned offset = i*2000 + 1000;
		if( filter == DARK_SELECT && i == 0 )
			render_dark_select( it, use_plane_alpha, offset, watcher );
		else if( use_plane_alpha )
			render_average_alpha( it, offset, watcher );
		else
			render_average_extended( it, offset, watcher );
		
		img[i] = out;
	}
	
	qDebug( "render rest took: %d", t.elapsed() );
	
	for( unsigned i=0; i<img.size(); i++ )
		qDebug( "Image %d was %s", i, img[i] ? "valid" : "invalid" );
	
	return img;
}



