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


#include "MultiImage.hpp"
#include "ImageEx.hpp"

#include <cmath>
#include <queue>
#include <fstream>
#include <QTime>
#include "MultiPlaneIterator.hpp"

using namespace std;

MultiImage::MultiImage(){
	threshould = 16*256;
	movement = 0.5;
	merge_method = 0;
	use_average = true;
	prev_frame = nullptr;
	interlace_on = false;
}
MultiImage::~MultiImage(){
	clear();
}

void MultiImage::clear(){
	for( unsigned i=0; i<imgs.size(); i++ )
		delete imgs[i];
	if( prev_frame && frame_interlaced )
		delete prev_frame;
	prev_frame = nullptr;
	
	imgs.clear();
	pos.clear();
	calculate_size();
}

bool MultiImage::set_interlaceing( bool setting ){
	if( imgs.size() == 0 )
		interlace_on = setting;
	return interlace_on;
}

pair<QPoint,double> MultiImage::merge_image( ImageEx& img1, ImageEx& img2 ) const{
	//Keep repeating with higher levels until it drops
	//below threshould
	int level = 1;
	pair<QPoint,double> result;
	DiffCache cache;
	do{
		switch( merge_method ){
			case 1: 
					result = img1.best_horizontal( img2, level, movement, &cache );
				break;
			case 2: 
					result = img1.best_vertical( img2, level, movement, &cache );
				break;
			
			case 0:
			default:
					result = img1.best_round( img2, level, movement, movement, &cache );
				break;
		}
	}while( result.second > 24*256 && level++ < 6 );
	
	return result;
}

void MultiImage::add_image( ImageEx *img ){
	QTime t;
	t.start();
	QPoint p( 0,0 );
	
	//Detelecine
	if( interlace_on ){
		bool current_interlaced = img->is_interlaced();
		
		//Handle first image
		if( !prev_frame ){
			frame_interlaced = current_interlaced;
			prev_frame = img;
			
			//Do we need another frame to continue?
			if( current_interlaced )
				return;
		}
		else{
			if( !frame_interlaced ){ //p
				if( !current_interlaced ){ //p + p
					//Nothing to do, neighter is interlaced
					prev_frame = img;
					frame_interlaced = current_interlaced; //false
				}
				else{ //p + i
					prev_frame->combine_line( *img, true );
					prev_frame = img;
					frame_interlaced = current_interlaced; //true
					return;
				}
			}
			else{
				if( !current_interlaced ){ //i + p
					img->combine_line( *prev_frame, false );
					delete prev_frame;
					prev_frame = img;
					frame_interlaced = current_interlaced; //false
				}
				else{ // i + i
					prev_frame->replace_line( *img, true );
					ImageEx* temp = prev_frame;
					prev_frame = img;
					img = temp; //Swap
					frame_interlaced = current_interlaced; //true
				}
			}
		}
	}
	
	if( imgs.size() > 0 ){
		ImageEx *img1 = NULL;
		bool destroy = true;
		if( use_average )
			img1 = render_image( FILTER_FOR_MERGING );
		else{
			img1 = imgs[imgs.size()-1];
			p = pos[imgs.size()-1];
			if( p.x() < 0 )
				p.setX( 0 );
			if( p.y() < 0 )
				p.setY( 0 );
			destroy = false;
		}
		
		qDebug( "image prepared: %d", t.restart() );
		
		
		pair<QPoint,double> result = merge_image( *img1, *img );
		
		p += get_size().topLeft() + result.first;
		if( destroy )
			delete img1;
		qDebug( "image compared: %d", t.elapsed() );
	}
	
	//Add image
	imgs.push_back( img );
	pos.push_back( p );
	calculate_size();
}


MultiImage::ImageMatches MultiImage::overlaps_image( unsigned index ) const{
	if( index >= get_count() )
		return ImageMatches();
	
	QRect current( get_rect( index ) );
	ImageMatches matches;
	double area = current.width() * current.height();
	
	for( unsigned i=0; i<imgs.size(); i++ ){
		if( i == index )
			continue;
		
		QRect common = current.intersected( get_rect( i ) );
		if( common.width() > 0 && common.height() > 0 ){
			double common_area = common.width() * common.height();
			if( common_area/area > 0.75 )
				matches.push_back( ImageMatch( i, common_area / area ) );
		}
	}
	
	return matches;
}


void MultiImage::subalign_images(){
	//Find all overlapping images
	vector<ImageMatches> align;
	align.reserve( get_count() );
	for( unsigned i=0; i<imgs.size(); i++ )
		align.push_back( overlaps_image( i ) );
	
	//We will store the enlarged planes and when we can release them again here:
	vector<std::pair<Plane*,unsigned> > cache;
	cache.reserve( get_count() );
	for( unsigned i=0; i<get_count(); i++ )
		cache.push_back( pair<Plane*,unsigned>( NULL, 0 ) );
	
	//Find the last occurence of each image
	//Do this by looping through all overlapping images, and set unsigned to i
	for( unsigned i=0; i<align.size(); i++ ){
		for( unsigned j=0; j<align[i].size(); j++ ){
			cache[ align[i][j].first ].second = i;
		}
	}
	
	//TODO: Find all relative positions
	fstream rel( "/home/spiller/Projects/overmix/merges.txt", fstream::out );
	for( unsigned i=0; i<imgs.size(); i++ ){
		for( unsigned j=0; j<align[i].size(); j++ ){
			unsigned index = align[i][j].first;
			if( index >= i )
				rel << "Find distance between " << i << " and " << index << "\n";
		//	else
		//		rel << "Reusing " << index << " and " << i << "\n";
		}
	}
	rel.close();
	
	//TODO: adjust all relative positions into aboslute ones
	
	//TODO: align so that most images are on the grid?
	
	//Debug positions
	fstream f( "/home/spiller/Projects/overmix/positions.txt", fstream::out );
	for( unsigned i=0; i<imgs.size(); i++ ){
		f << "Image " << i << ": " << pos[i].x() << "x" << pos[i].y() << " (ends at: " << cache[i].second << ") " << align[i].size() << "\n";
		for( unsigned j=0; j<align[i].size(); j++ ){
			f << "\t" << align[i][j].first << " " << align[i][j].second << "\n";
		}
	}
	f.close();
}


ImageEx* MultiImage::render_image( filters filter ) const{
	QTime t;
	t.start();
	
	//Abort if no images
	if( imgs.size() == 0 ){
		qWarning( "No images to render!" );
		return nullptr;
	}
	qDebug( "render_image: image count: %d", (int)imgs.size() );
	
	
	//Do iterator
	QRect box = get_size();
	ImageEx *img = new ImageEx( imgs[0]->get_system() );
	if( !img )
		return NULL;
	if( !img->create( box.width(), box.height() ) ){
		delete img;
		return NULL;
	}
	
	//TODO: determine amount of planes!
	unsigned planes_amount = 3; //TODO: alpha?
	if( filter == FILTER_FOR_MERGING && imgs[0]->get_system() == ImageEx::YUV )
		planes_amount = 1;
	
	ImageEx &first( *imgs[0] );
	int width = first[0]->get_width();
	int height = first[0]->get_height();
	
	for( unsigned i=0; i<planes_amount; i++ ){
		//Clear output plane
		Plane& p( *((*img)[i]) );
		for( unsigned iy=0; iy<p.get_height(); ++iy ){
			color_type* row = p.scan_line( iy );
			for( unsigned ix=0; ix<p.get_width(); ++ix )
				row[ix] = 0;
		}
		
		vector<PlaneItInfo> info;
		info.push_back( PlaneItInfo( &p, box.x(),box.y() ) );
		
		int local_width = first[i]->get_width();
		int local_height = first[i]->get_height();
		vector<Plane*> temp;
		
		
		if( local_width == width && local_height == height ){
			for( unsigned j=0; j<imgs.size(); j++ )
				info.push_back( PlaneItInfo( (*imgs[j])[i], pos[j].x(),pos[j].y() ) );
		}
		else{
			temp.reserve( imgs.size() );
			for( unsigned j=0; j<imgs.size(); j++ ){
				Plane *p = (*imgs[j])[i]->scale_cubic( width, height );
				if( !p )
					qDebug( "No plane :\\" );
				temp.push_back( p );
				info.push_back( PlaneItInfo( p, pos[j].x(),pos[j].y() ) );
			}
		}
		
		MultiPlaneIterator it( info );
		
		if( it.iterate_all() ){
		//Do average and store in [0]
		it.for_all_pixels( [](MultiPlaneLineIterator &it){
				unsigned avg = 0;
				for( unsigned i=1; i<it.size(); i++ )
					avg += it[i];
				
				if( it.size() > 1 )
					it[0] = avg / (it.size() - 1); //NOTE: Will crash if image contains empty parts
				else
					it[0] = 0;
			} );
		}
		else{
		//Do average and store in [0]
		it.for_all_pixels( [](MultiPlaneLineIterator &it){
				unsigned avg = 0, amount = 0;
	
				for( unsigned i=1; i<it.size(); i++ ){
					if( it.valid( i ) ){
						avg += it[i];
						amount++;
					}
				}
				
				if( amount )
					it[0] = avg / amount;
			} );
		}
		
		//Remove scaled planes
		for( unsigned j=0; j<temp.size(); j++ )
			delete temp[j];
	}
	
	// Testing code
//	if( !filter == FILTER_FOR_MERGING )
//		img->scale( 0.95 );
	
	/* 
	color (MultiImageIterator::*function)();
	function = &MultiImageIterator::average;
	*row = (it.*function)();
	*/
	qDebug( "render rest took: %d", t.elapsed() );
	
	return img;
}

QImage MultiImage::render( filters filter, ImageEx::YuvSystem system,  unsigned setting, double scale_width ) const{
	ImageEx *img_org = render_image( filter );
	if( !img_org )
		return QImage();
	
	QTime t;
	t.start();
	
	//Fix aspect ratio
	if( scale_width <= 0.9999 || scale_width >= 1.0001 )
		img_org->scale( img_org->get_width() * scale_width + 0.5, img_org->get_height() );
	
	QImage img = img_org->to_qimage( system, setting );
	delete img_org;
	
	qDebug( "to_qimage() took: %d", t.elapsed() );
	
	return img;
}


void MultiImage::calculate_size(){
	size_cache = QRect();
	
	for( unsigned i=0; i<pos.size(); i++ )
		size_cache = size_cache.united( QRect( pos[i], QSize( imgs[i]->get_width(), imgs[i]->get_height() ) ) );
}




