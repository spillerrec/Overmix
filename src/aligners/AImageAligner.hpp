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

#include "AAligner.hpp"
#include "../planes/Plane.hpp"
#include "../planes/ImageEx.hpp"
#include "../containers/AContainer.hpp"

#include <vector>

namespace Overmix{

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
			Point<double> distance;
			double error;
			double overlap;
			ImageOffset( Point<double> distance, double error, double overlap )
				: distance(distance), error(error), overlap(overlap) { }
		};
		static double calculate_overlap( Point<> offset, const Plane& img1, const Plane& img2 );
		
	public: //TODO:
		ImageOffset find_offset( const Plane& img1, const Plane& img2, const Plane& a1, const Plane& a2 ) const;
		ImageOffset findOffset( unsigned index, const Plane& img2, const Plane& a2 ) const
			{ return find_offset( image( index )[0], img2, alpha( index ), a2 ); }
		ImageOffset findOffset( unsigned index, unsigned index2 ) const
			{ return findOffset( index, image( index2 )[0], alpha( index2 ) ); }
		ImageOffset findOffset( const ImageEx& img1, const ImageEx& img2 ) const
			{ return find_offset( img1[0], img2[0], img1.alpha_plane(), img2.alpha_plane() ); }
	
	protected:
		const AlignMethod method;
		const double scale;
		bool use_edges{ false };
		double movement{ 0.75 };
		bool raw;
		bool fast_diffing{ true };
		AContainer& container;
		
	public:
		Point<double> scales() const
			{ return { (method != ALIGN_VER) ? scale : 1.0, (method != ALIGN_HOR) ? scale : 1.0 }; }
		AContainer& getContainer(){ return container; }
		
	private:
		std::vector<ImageEx> images;
	public:
		//Accessors
		unsigned count() const override{ return container.count(); }
		const ImageEx& image( unsigned index ) const override;
		ImageEx& imageRef( unsigned index ) override;
		const Plane& alpha( unsigned index ) const override{ return container.alpha( index ); }
		int imageMask( unsigned index ) const override{ return container.imageMask( index ); }
		void setMask( unsigned index, int id ) override { container.setMask( index, id ); }
		const Plane& mask( unsigned index ) const override{ return container.mask( index ); }
		unsigned maskCount() const override{ return container.maskCount(); }
		Point<double> pos( unsigned index ) const override;
		void setPos( unsigned index, Point<double> newVal ) override;
		int frame( unsigned index ) const override{ return container.frame( index ); }
		void setFrame( unsigned index, int newVal ) override{ container.setFrame( index, newVal ); }
		
		//Construction
		void addImages();
		
	protected:
		//Triggers when inserting
		virtual Plane prepare_plane( const Plane& ) const;
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
};

/* Until we have fixed all the methods */
class WrapperImageAligner : public AAligner{
	protected:
		AImageAligner::AlignMethod method;
		double scale;
		bool edges { false };
		double movement{ 0.75 };
		virtual std::unique_ptr<AImageAligner> makeAligner( AContainer& container ) = 0;
		
	public:
		void setOptions( AImageAligner::AlignMethod method, double scale, bool edges=false, double movement=0.75 ){
			this->method   = method;
			this->scale    = scale;
			this->edges    = edges;
			this->movement = movement;
		}
		
		virtual void align( AContainer& container, class AProcessWatcher* watcher=nullptr ) override{
			auto aligner = makeAligner( container );
			aligner->set_edges( edges );
			aligner->set_movement( movement );
			aligner->addImages();
			aligner->align( watcher );
		}
};

}

#endif