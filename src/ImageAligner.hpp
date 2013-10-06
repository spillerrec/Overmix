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

#ifndef IMAGE_ALIGNER_HPP
#define IMAGE_ALIGNER_HPP

#include "ImageEx.hpp"

#include <vector>
#include <QPoint>

class ImageAligner{
	public:
		enum AlignMethod{
			ALIGN_BOTH,
			ALIGN_VER,
			ALIGN_HOR
		};
		
	private:
		struct ImagePosition{
			const ImageEx* const original;
			ImageEx* image;
			double pos_x;
			double pos_y;
			
			ImagePosition( const ImageEx* const img, double scale_x, double scale_y );
		};
		
		struct ImageOffset{
			double distance_x;
			double distance_y;
			double error;
			double overlap;
		};
	
	private:
		AlignMethod method;
		const double scale;
		std::vector<ImagePosition> images;
		std::vector<ImageOffset> offsets;
		
		ImageOffset get_offset( unsigned img1, unsigned img2 ) const;
		ImageOffset find_offset( const ImageEx& img1, const ImageEx& img2 ) const;
		
		double x_scale() const;
		double y_scale() const;
		
		void rough_align();
		
		double total_error() const;
	
	public:
		ImageAligner( AlignMethod method, double scale=1.0 );
		~ImageAligner();
		
		void add_image( const ImageEx* const img );
		
		void align();
		
		QPoint pos( unsigned index ) const;
	
};

#endif