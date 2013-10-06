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


#include "ImageAligner.hpp"

#include <algorithm>
#include <fstream>
#include <utility>
#include <float.h>
#include <QDebug>

ImageAligner::ImagePosition::ImagePosition( const ImageEx* const img, double scale_x, double scale_y )
	:	original( img ){
	image = new ImageEx( *img );
	image->scale( image->get_width() * scale_x + 0.5, image->get_height() * scale_y + 0.5 );
	image->apply_operation( &Plane::edge_sobel );
	//TODO: support scaling better
}


ImageAligner::ImageAligner( AlignMethod method, double scale ) : method(method), scale(scale){
	
}

ImageAligner::~ImageAligner(){
	//Remove scaled images
	for( unsigned i=0; i<images.size(); ++i )
		if( images[i].original != images[i].image )
			delete images[i].image;
}

ImageAligner::ImageOffset ImageAligner::get_offset( unsigned img1, unsigned img2 ) const{
	if( img1 == img2 ){
		qWarning( "Attempting to get offset of the same image!" );
		ImageOffset pos = { 0, 0, 0, 1 };
		return pos;
	}
	
	unsigned base = std::max( img1, img2 );
	unsigned pos = std::min( img1, img2 );
	unsigned index = (base-1) * base / 2 + pos; // n(n+1)/2, with n=base-1
	
	if( base >= images.size() )
		qFatal( "Out of bounds access in get_offset( %d, %d )", img1, img2 );
	
	ImageOffset offset = offsets[index];
	
	//Reverse distances if indexes where switched
	if( img2 < img1 ){
		offset.distance_x = -offset.distance_x;
		offset.distance_y = -offset.distance_y;
	}
	
	return offset;
}

double calculate_overlap( QPoint offset, const ImageEx& img1, const ImageEx& img2 ){
	QRect first( 0,0, img1.get_width(), img1.get_height() );
	QRect second( offset, QSize(img2.get_width(), img2.get_height()) );
	QRect common = first.intersected( second );
	
	double area = first.width() * first.height();
	return (double)common.width() * common.height() / area;
}

ImageAligner::ImageOffset ImageAligner::find_offset( const ImageEx& img1, const ImageEx& img2 ) const{
	//Keep repeating with higher levels until it drops
	//below threshould
	double movement = 0.75; //TODO:
	int level = 6;
	std::pair<QPoint,double> result;
	DiffCache cache;
	do{
		switch( method ){
			case ALIGN_HOR: 
					result = img1.best_horizontal( img2, level, movement, &cache );
				break;
			case ALIGN_VER: 
					result = img1.best_vertical( img2, level, movement, &cache );
				break;
			
			case ALIGN_BOTH:
			default:
					result = img1.best_round( img2, level, movement, movement, &cache );
				break;
		}
	}while( result.second > 24*256 && level++ < 6 );
	
	ImageOffset offset;
	offset.distance_x = result.first.x();
	offset.distance_y = result.first.y();
	offset.error = result.second;
	offset.overlap = calculate_overlap( result.first, img1, img2 );
	
	return offset;
}

double ImageAligner::x_scale() const{
	if( method == ALIGN_BOTH || method == ALIGN_HOR )
		return scale;
	else
		return 1.0;
}
double ImageAligner::y_scale() const{
	if( method == ALIGN_BOTH || method == ALIGN_VER )
		return scale;
	else
		return 1.0;
}

void ImageAligner::add_image( const ImageEx* const img ){
	images.push_back( ImagePosition( img, x_scale(), y_scale() ) );
	
	unsigned last = images.size()-1;
	for( unsigned i=0; i<last; ++i )
		offsets.push_back( find_offset( *(images[i].image), *(images[last].image) ) );
}

void ImageAligner::rough_align(){
	//Use two lists, one containing all uninitialized images (indexes) and another
	//containing all initialized ones. Set all images as unset as starting point.
	std::vector<unsigned> unset;
	std::vector<unsigned> set;
	unset.reserve( images.size() );
	set.reserve( images.size() );
	for( unsigned i=0; i<images.size(); ++i )
		unset.push_back( i );
	
	//Initialize a single image as (0,0)
	//The last image is used, but anyone could do
	unsigned first = unset.back();
	unset.pop_back();
	images[first].pos_x = 0;
	images[first].pos_y = 0;
	set.push_back( first );
	
	//Keep adding one image at a time until all have been initialized
	while( unset.size() > 0 ){
		//Find the image which offset has the smallest error
		unsigned best_set=0, best_unset=0;
		double min_error = DBL_MAX;
		for( unsigned i=0; i<unset.size(); ++i ){
			for( unsigned j=0; j<set.size(); ++j ){
				ImageOffset p = get_offset( unset[i], set[j] );
				if( p.error <= min_error ){
					best_unset = i;
					best_set = j;
					min_error = p.error;
				}
			}
		}
		
		//Use that offset as the positioning for this image
		unsigned next = unset[best_unset];
		unsigned old = set[best_set];
		ImageOffset offset = get_offset( old, next );
		images[next].pos_x = images[old].pos_x + offset.distance_x;
		images[next].pos_y = images[old].pos_y + offset.distance_y;
		
		//Update lists;
		std::swap( unset[best_unset], unset[unset.size()-1] );
		unset.pop_back(); //Removes in constant time
		set.push_back( next );
	}
}

double ImageAligner::total_error() const{
	double error=0;
	
	for( unsigned i=0; i<images.size(); ++i ){
		double local_error=0, weight=0, weight_sum=0;
		for( unsigned j=0; j<images.size(); ++j ){
			if( j == i )
				continue;
			ImageOffset offset = get_offset( i, j );
			if( offset.overlap > 0.25 )
				weight_sum += offset.error;
		}
		
		for( unsigned j=0; j<images.size(); ++j ){
			if( j == i )
				continue;
			
			ImageOffset offset = get_offset( i, j );
			if( offset.overlap > 0.25 ){
				double w = weight_sum / offset.error; //TODO: offset.error == 0 !
				local_error += std::abs(images[j].pos_x + offset.distance_x - images[i].pos_x) * w;
				local_error += std::abs(images[j].pos_y + offset.distance_y - images[i].pos_y) * w;
				weight += w;
			}
		}
		error += local_error / weight;
	}
	
	return error;
}

#include <QTime>
void ImageAligner::align(){
	if( images.size() == 0 ){
		qWarning( "No images to align" );
		return;
	}
	
	std::fstream rel( "align.txt", std::fstream::out );
	
	QTime t;
	t.start();
	
	rough_align();
	
	//Update overlap values
	//*
	unsigned offset_index = 0;
	for( unsigned i=1; i<images.size(); ++i ){
		ImagePosition img = images[i];
		for( unsigned j=0; j<i; ++j ){
			if( j == i )
				continue;
			
			ImagePosition img2 = images[j];
			ImageOffset& offset = offsets[offset_index++];
			
			double new_overlap = calculate_overlap(
					QPoint( img2.pos_x - img.pos_x, img2.pos_y - img.pos_y )
				,	*img.image
				,	*img2.image
				);
			
			rel << "overlap error is: " << std::abs( offset.overlap - new_overlap ) << " (" << offset.overlap << " - " << new_overlap << ")\n";
			if( std::abs( offset.overlap - new_overlap ) > 0.1 )
				new_overlap = 0.0;
			else
				offset.error *= 1.0 + std::abs( offset.overlap - new_overlap ) * 100;
			offset.error *= offset.error * offset.error;
			offset.overlap = new_overlap;
		}
	}
	//*/
	rel << "rough_align() took: " << t.restart() << " msec\n";
	
	for( unsigned iterations=0; iterations<2000; ++iterations ){
		rel << "Error at iteration " << iterations << ": " << total_error() << "\n";
		
		for( unsigned i=0; i<images.size(); ++i ){
			double x=0, y=0, weight=0, weight_sum=0;
			for( unsigned j=0; j<images.size(); ++j ){
				if( j == i )
					continue;
				
				ImageOffset offset = get_offset( i, j );
				if( offset.overlap > 0.25 )
					weight_sum += offset.error;
			}
			
			for( unsigned j=0; j<images.size(); ++j ){
				if( j == i )
					continue;
				
				ImageOffset offset = get_offset( j, i );
				if( offset.overlap > 0.25 ){
					double w = weight_sum / offset.error * offset.overlap; //TODO: offset.error == 0 !
					x += (images[j].pos_x + offset.distance_x) * w;
					y += (images[j].pos_y + offset.distance_y) * w;
					weight += w;
				}
			}
			
			images[i].pos_x = x / weight;
			images[i].pos_y = y / weight;
		}
	}
	rel << "Fine alignment took: " << t.restart() << " msec\n";
	
	for( unsigned i=0; i<images.size(); ++i )
		rel << "Image " << i << ": " << images[i].pos_x << "x" << images[i].pos_y << "\n";
	
	rel.close();
}

QPoint ImageAligner::pos( unsigned index ) const{
	return QPoint(
			round( images[index].pos_x / x_scale() )
		,	round( images[index].pos_y / y_scale() )
		);
}

