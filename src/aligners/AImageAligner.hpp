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
#include "../containers/AContainer.hpp"

#include <vector>
#include <QPointF>

class AProcessWatcher;
class ImageContainer;

class AImageAligner : public AContainer{
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
		AContainer& container;
		
	public:
		double x_scale() const;
		double y_scale() const;
		AContainer& getContainer(){ return container; }
		
	private:
		std::vector<ImageEx> images;
	public:
		//Accessors
		unsigned count() const{ return container.count(); }
		const ImageEx& image( unsigned index ) const;
		const Plane& alpha( unsigned index ) const{ return container.alpha( index ); }
		const Plane& mask( unsigned index ) const override{ return container.mask( index ); }
		unsigned maskCount() const override{ return container.maskCount(); }
		QPointF pos( unsigned index ) const;
		void setPos( unsigned index, QPointF newVal );
		int frame( unsigned index ) const{ return container.frame( index ); }
		void setFrame( unsigned index, int newVal ){ container.setFrame( index, newVal ); }
		
		//Construction
		void addImages();
		
	protected:
		//Triggers when inserting
		virtual Plane prepare_plane( const Plane& );
		virtual void on_add(){ }
	
	public:
		AImageAligner( AContainer& container, AlignMethod method, double scale=1.0 );
		//copy, move, assign?
		virtual ~AImageAligner() { }
		
		void set_raw( bool value ){ raw = value; }
		void set_movement( double movement ){ this->movement = movement; }
		void set_edges( bool enabled ){ use_edges = enabled; }
		
		AlignMethod get_method() const{ return method; }
		double get_scale() const{ return scale; }
		double get_movement() const{ return movement; }
		double get_edges() const{ return use_edges; }
		
		virtual void align( AProcessWatcher* watcher=nullptr ) = 0;
		
		void debug( QString csv_file ) const;
};

#endif