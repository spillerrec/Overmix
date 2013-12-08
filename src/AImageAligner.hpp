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
			/*const*/ Plane* const image;
			QPointF pos;
			
			ImagePosition( const ImageEx* const org, /*const*/ Plane* const img )
				:	original(org), image(img) { }
		};
		
		struct ImageOffset{
			double distance_x;
			double distance_y;
			double error;
			double overlap;
		};
		static double calculate_overlap( QPoint offset, const Plane& img1, const Plane& img2 );
		
	public: //TODO:
		ImageOffset find_offset( const Plane& img1, const Plane& img2 ) const;
	
	protected:
		const AlignMethod method;
		const double scale;
		double movement{ 0.75 };
		std::vector<ImagePosition> images;
		bool raw;
		
		double x_scale() const;
		double y_scale() const;
		
		virtual /*const*/ Plane* prepare_plane( /*const*/ Plane* p ){ return p; }
		virtual void on_add( ImagePosition& ){ }
	
	public:
		AImageAligner( AlignMethod method, double scale=1.0 ) : method(method), scale(scale), raw(false){ }
		virtual ~AImageAligner();
		
		void set_movement( double movement ){ this->movement = movement; }
		
		AlignMethod get_method() const{ return method; }
		double get_scale() const{ return scale; }
		double get_movement() const{ return movement; }
		
		void add_image( /*const*/ ImageEx* const img );
		
		QRect size() const;
		unsigned count() const{ return images.size(); }
		const ImageEx* image( unsigned index ) const{ return images[index].original; }
		/*const*/ Plane* plane( unsigned img_index, unsigned p_index ) const;
		QPointF pos( unsigned index ) const;
		
		virtual void align() = 0;
};

#endif