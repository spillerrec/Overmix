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
}
MultiImage::~MultiImage(){
	clear();
}

void MultiImage::clear(){
	for( unsigned i=0; i<imgs.size(); i++ )
		delete imgs[i];
	imgs.clear();
	pos.clear();
	calculate_size();
}

pair<QPoint,double> MultiImage::merge_image( ImageEx& img1, ImageEx& img2 ) const{
	//Keep repeating with higher levels until it drops
	//below threshould
	int level = 1;
	pair<QPoint,double> result;
	do{
		switch( merge_method ){
			case 1: 
					result = img1.best_horizontal( img2, level, movement );
				break;
			case 2: 
					result = img1.best_vertical( img2, level, movement );
				break;
			
			case 0:
			default:
					result = img1.best_round( img2, level, movement, movement );
				break;
		}
	}while( result.second > 24*256 && level++ < 6 );
	
	return result;
}

void MultiImage::add_image( QString path ){
	QTime t;
	t.start();
	ImageEx *img = new ImageEx();
	img->read_file( path.toLocal8Bit().constData() );
	QPoint p( 0,0 );
	qDebug( "image loaded: %d", t.elapsed() );
	
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
		
		qDebug( "image prepared: %d", t.elapsed() );
		
		
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
	
	qDebug( "render_image: image count: %d", (int)imgs.size() );
	
	//TODO: check for first image!
	
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
	int width = first[0].p.get_width();
	int height = first[0].p.get_height();
	
	for( unsigned i=0; i<planes_amount; i++ ){
		vector<PlaneItInfo> info;
		info.push_back( PlaneItInfo( &(*img)[i].p, box.x(),box.y() ) );
		
		int local_width = first[i].p.get_width();
		int local_height = first[i].p.get_height();
		vector<Plane*> temp;
		
		
		if( local_width == width && local_height == height ){
			for( unsigned j=0; j<imgs.size(); j++ )
				info.push_back( PlaneItInfo( &(*imgs[j])[i].p, pos[j].x(),pos[j].y() ) );
		}
		else{
			temp.reserve( imgs.size() );
			for( unsigned j=0; j<imgs.size(); j++ ){
				Plane *p = (*imgs[j])[i].p.scale_nearest( width, height, 0, 0 );
				if( !p )
					qDebug( "No plane :\\" );
				temp.push_back( p );
				info.push_back( PlaneItInfo( p, pos[j].x(),pos[j].y() ) );
			}
		}
		
		MultiPlaneIterator it( info );
		it.iterate_all();
		
		//Do average and store in [0]
		it.for_all_lines( [](MultiPlaneLineIterator &it){
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
		
		//Remove scaled planes
		for( unsigned j=0; j<temp.size(); j++ )
			delete temp[j];
	}
	
	
	/* 
	MultiImageIterator it( imgs, pos, box.x(),box.y() );
	it.set_threshould( threshould );
	//Pointer to the function we want to use
	color (MultiImageIterator::*function)();
	switch( filter ){
		case FILTER_DIFFERENCE:   function = &MultiImageIterator::difference; break;
		case FILTER_SIMPLE:       function = &MultiImageIterator::simple_filter; break;
		case FILTER_SIMPLE_SLIDE: function = &MultiImageIterator::simple_slide; break;
		default:
		case FILTER_AVERAGE:      function = &MultiImageIterator::average; break;
	}
	
	image *img = new image( box.width(), box.height() );
	qDebug( "render init took: %d", t.elapsed() );
	for( int iy = 0; iy < box.height(); iy++, it.next_line() ){
		color* row = img->scan_line( iy );
		for( int ix = 0; ix < box.width(); ++ix, it.next_x(), ++row )
			*row = (it.*function)();
	} */
	qDebug( "render rest took: %d", t.elapsed() );
	
	return img;
}

QImage MultiImage::render( filters filter, bool dither ) const{
	ImageEx *img_org = render_image( filter );
	QTime t;
	t.start();
	QImage img = img_org->to_qimage( dither );
	delete img_org;
	
	qDebug( "image load took: %d", t.elapsed() );
	
	return img;
}


void MultiImage::calculate_size(){
	size_cache = QRect();
	
	for( unsigned i=0; i<pos.size(); i++ )
		size_cache = size_cache.united( QRect( pos[i], QSize( imgs[i]->get_width(), imgs[i]->get_height() ) ) );
}




