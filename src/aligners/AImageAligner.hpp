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

#include "../planes/Plane.hpp"
#include "../ImageEx.hpp"

#include <vector>
#include <QPointF>

class AProcessWatcher;

class AImageAligner{
	public:
		enum AlignMethod{
			ALIGN_BOTH,
			ALIGN_VER,
			ALIGN_HOR
		};
		
	protected:
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
		bool use_edges{ false };
		double movement{ 0.75 };
		bool raw;
		
	public:
		double x_scale() const;
		double y_scale() const;
		
	private:
		struct ImagePosition{
			const ImageEx& original;
			const Plane image;
			QPointF pos;
			
			ImagePosition( const ImageEx& org, Plane&& img )
				:	original(org), image(img) { }
		};
		std::vector<ImagePosition> images;
	public:
		//Accessors
		unsigned count() const{ return images.size(); }
		const ImageEx& image( unsigned index ) const{ return images[index].original; }
		const Plane& plane( unsigned img_index, unsigned p_index=0 ) const;
		QPointF pos( unsigned index ) const;
		void setPos( unsigned index, QPointF newVal ){ images[index].pos = newVal; }
		
		//Construction
		void add_image( const ImageEx& img );
		
	protected:
		//Triggers when inserting
		virtual Plane prepare_plane( const Plane& );
		virtual void on_add(){ }
	
	public:
		AImageAligner( AlignMethod method, double scale=1.0 ) : method(method), scale(scale), raw(false){ }
		//copy, move, assign?
		virtual ~AImageAligner();
		
		void set_raw( bool value ){ raw = value; }
		void set_movement( double movement ){ this->movement = movement; }
		void set_edges( bool enabled ){ use_edges = enabled; }
		
		AlignMethod get_method() const{ return method; }
		double get_scale() const{ return scale; }
		double get_movement() const{ return movement; }
		double get_edges() const{ return use_edges; }
		
		void offsetAll( double dx, double dy ){
			QPointF offset( dx, dy );
			for( auto& image : images )
				image.pos += offset;
		}
		
		
		QRect size() const;
		
		virtual void align( AProcessWatcher* watcher=nullptr ) = 0;
		
		void debug( QString csv_file ) const;
};

#endif