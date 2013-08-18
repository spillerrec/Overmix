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

#ifndef MULTI_IMAGE_H
#define MULTI_IMAGE_H

#include <QImage>
#include <QRect>

#include <utility>
#include <vector>

#include "ImageEx.hpp"


class MultiImage{
	//Cache
	private:
		QRect size_cache;
		void calculate_size();
	
	private:
		std::vector<ImageEx*> imgs;
		std::vector<QPoint> pos;
		unsigned threshould;
		double movement;
		int merge_method; //0 = both, 1 = hor, 2 = ver
		bool use_average;
		
		std::pair<QPoint,double> merge_image( ImageEx& img1, ImageEx& img2 ) const;
		
		QRect get_rect( unsigned index ) const{
			return QRect( pos[index], QSize( imgs[index]->get_width(), imgs[index]->get_height() ) );
		}
		
		typedef std::pair<unsigned,double> ImageMatch;
		typedef std::vector<ImageMatch> ImageMatches;
		ImageMatches overlaps_image( unsigned index ) const;
		
	private:
		bool interlace_on;
		ImageEx *prev_frame;
		bool frame_interlaced;
	public:
		bool interlacing_enabled() const{ return interlace_on; }
		bool set_interlaceing( bool setting );
	
	public:
		MultiImage();
		~MultiImage();
		
		//Setters
		void set_use_average( bool value ){ use_average = value; }
		void set_threshould( unsigned value ){ threshould = value * 256; }
		void set_movement( double value ){ movement = value; }
		void set_merge_method( int value ){ merge_method = value; }
		
		void clear();
		void add_image( ImageEx *img );
		void subalign_images();
		
		enum filters{
			FILTER_FOR_MERGING,
			FILTER_AVERAGE,
			FILTER_DIFFERENCE,
			FILTER_SIMPLE,
			FILTER_SIMPLE_SLIDE
		};
		ImageEx* render_image( filters filter ) const;
		QImage render( filters filter, bool dither=true, bool gamma=true, bool rec709=true ) const;
		
		unsigned get_count() const{ return imgs.size(); }
		QRect get_size() const{ return size_cache; }
	
};

#endif