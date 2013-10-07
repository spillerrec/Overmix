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

#ifndef A_IMAGE_ALIGNER_HPP
#define A_IMAGE_ALIGNER_HPP

#include "Plane.hpp"
#include "ImageEx.hpp"

#include <vector>
#include <QPointF>

class AImageAligner{
	public:
		enum AlignMethod{
			ALIGN_BOTH,
			ALIGN_VER,
			ALIGN_HOR
		};
		
	protected:
		struct ImagePosition{
			const ImageEx* const original;
			const Plane* const image;
			QPointF pos;
			
			ImagePosition( const ImageEx* const org, const Plane* const img )
				:	original(org), image(img) { }
		};
		
		struct ImageOffset{
			double distance_x;
			double distance_y;
			double error;
			double overlap;
		};
		static double calculate_overlap( QPoint offset, const Plane& img1, const Plane& img2 );
		ImageOffset find_offset( const Plane& img1, const Plane& img2 ) const;
	
	protected:
		const AlignMethod method;
		const double scale;
		std::vector<ImagePosition> images;
		
		double x_scale() const;
		double y_scale() const;
		
		virtual const Plane* prepare_plane( const Plane* p ){ return p; }
		virtual void on_add( ImagePosition& ){ }
	
	public:
		AImageAligner( AlignMethod method, double scale=1.0 ) : method(method), scale(scale){ }
		virtual ~AImageAligner();
		
		void add_image( const ImageEx* const img );
		
		QRect size() const;
		unsigned count() const{ return images.size(); }
		const ImageEx* image( unsigned index ) const{ return images[index].original; }
		QPointF pos( unsigned index ) const{
			return QPointF( images[index].pos.x() / x_scale(), images[index].pos.y() / y_scale() );
		}
		
		virtual void align() = 0;
};

#endif