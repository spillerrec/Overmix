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
#include "../containers/AContainer.hpp"
#include "../planes/Plane.hpp"
#include "../planes/ImageEx.hpp"
#include "../utils/PlaneUtils.hpp"
#include "../comparators/GradientComparator.hpp" //TODO: remove the need of this

#include <vector>

namespace Overmix{

class AProcessWatcher;
class ImageContainer;

struct AlignSettings{
	AlignMethod method;
	double movement;
	
	AlignSettings( AlignMethod method=AlignMethod::BOTH, double movement=1.0 )
		: method(method), movement(movement) { }
};

class AImageAligner{
	public:
		struct ImageOffset{
			Point<double> distance;
			double error;
			double overlap;
			ImageOffset( Point<double> distance, double error, double overlap )
				: distance(distance), error(error), overlap(overlap) { }
		};
		static double calculate_overlap( Point<> offset, const Plane& img1, const Plane& img2 );
		
	public: //TODO:
		static ImageOffset findOffset( Point<double> movement, const Plane& img1, const Plane& img2, const Plane& a1, const Plane& a2 );
		static ImageOffset findOffset( const AContainer& container, Point<double> movement, unsigned i1, unsigned i2 )
			{ return findOffset( movement, container.image(i1)[0], container.image(i2)[0], container.alpha(i1), container.alpha(i2) ); }
};

class AlignerProcessor{
	private:
		AlignSettings settings;
		double scale_amount{ 1.0 };
		bool edges{ false };
		
	public:
		AlignerProcessor( AlignSettings settings, double scale, bool edges=false )
			: settings(settings), scale_amount(scale), edges(edges) { }
		
		Point<double> filter( Point<double> value ) const;
		
		Point<double> scale() const;
		Point<double> movement() const
			{ return filter( {settings.movement, settings.movement} ); }
		
		ModifiedPlane operator()( const Plane& ) const;
		Modified<ImageEx> image( const ImageEx& ) const;
		ModifiedPlane scalePlane( const Plane& p ) const
			{ return getScaled( p, p.getSize()*scale() ); }
};

}

#endif
