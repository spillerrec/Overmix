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


void ImageAligner::on_add(){
	//Compare this one against all other images
	for( unsigned i=0; i<count()-1; ++i )
		offsets.push_back( find_offset( plane(i), plane( count()-1 ) ) );
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
	
	if( base >= count() )
		qFatal( "Out of bounds access in get_offset( %d, %d )", img1, img2 );
	
	ImageOffset offset = offsets[index];
	
	//Reverse distances if indexes where switched
	if( img2 < img1 ){
		offset.distance_x = -offset.distance_x;
		offset.distance_y = -offset.distance_y;
	}
	
	return offset;
}


void ImageAligner::rough_align(){
	//Use two lists, one containing all uninitialized images (indexes) and another
	//containing all initialized ones. Set all images as unset as starting point.
	std::vector<unsigned> unset;
	std::vector<unsigned> set;
	unset.reserve( count() );
	set.reserve( count() );
	for( unsigned i=0; i<count(); ++i )
		unset.push_back( i );
	
	//Initialize a single image as (0,0)
	//The last image is used, but anyone could do
	unsigned first = unset.back();
	unset.pop_back();
	setPos( first, { 0.0, 0.0 } );
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
		setPos( next, pos( old ) + QPointF( offset.distance_x, offset.distance_y ) );
		
		//Update lists;
		std::swap( unset[best_unset], unset[unset.size()-1] );
		unset.pop_back(); //Removes in constant time
		set.push_back( next );
	}
}

double ImageAligner::total_error() const{
	double error=0;
	
	for( unsigned i=0; i<count(); ++i ){
		double local_error=0, weight=0, weight_sum=0;
		for( unsigned j=0; j<count(); ++j ){
			if( j == i )
				continue;
			ImageOffset offset = get_offset( i, j );
			if( offset.overlap > 0.25 )
				weight_sum += offset.error;
		}
		
		for( unsigned j=0; j<count(); ++j ){
			if( j == i )
				continue;
			
			ImageOffset offset = get_offset( i, j );
			if( offset.overlap > 0.25 ){
				double w = weight_sum / offset.error; //TODO: offset.error == 0 !
				local_error += std::abs(pos( j ).x() + offset.distance_x - pos( i ).x()) * w;
				local_error += std::abs(pos( j ).y() + offset.distance_y - pos( i ).y()) * w;
				weight += w;
			}
		}
		error += local_error / weight;
	}
	
	return error;
}

#include <QTime>
void ImageAligner::align( AProcessWatcher* watcher ){
	if( count() == 0 ){
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
	for( unsigned i=1; i<count(); ++i ){
		for( unsigned j=0; j<i; ++j ){
			if( j == i )
				continue;
			
			ImageOffset& offset = offsets[offset_index++];
			
			double new_overlap = calculate_overlap(
					QPoint( pos( j ).x() - pos( i ).x(), pos( j ).y() - pos( i ).y() )
				,	plane( i )
				,	plane( j )
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
		
		for( unsigned i=0; i<count(); ++i ){
			double x=0, y=0, weight=0, weight_sum=0;
			for( unsigned j=0; j<count(); ++j ){
				if( j == i )
					continue;
				
				ImageOffset offset = get_offset( i, j );
				if( offset.overlap > 0.25 )
					weight_sum += offset.error;
			}
			
			for( unsigned j=0; j<count(); ++j ){
				if( j == i )
					continue;
				
				ImageOffset offset = get_offset( j, i );
				if( offset.overlap > 0.25 ){
					double w = weight_sum / offset.error * offset.overlap; //TODO: offset.error == 0 !
					x += (pos( j ).x() + offset.distance_x) * w;
					y += (pos( j ).y() + offset.distance_y) * w;
					weight += w;
				}
			}
			
			setPos( i, { x / weight, y / weight } );
		}
	}
	rel << "Fine alignment took: " << t.restart() << " msec\n";
	
	for( unsigned i=0; i<count(); ++i )
		rel << "Image " << i << ": " << pos( i ).x() << "x" << pos( i ).y() << "\n";
	
	rel.close();
}

