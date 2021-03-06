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

#include "GradientComparator.hpp"
#include "GradientPlane.hpp"

#include <QRect>

using namespace Overmix;


ImageOffset GradientComparator::findOffset( const Plane& img1, const Plane& img2, const Plane& a1, const Plane& a2, Point<double> hint ) const{
	Point<double> moves{ method == AlignMethod::VER ? 0.0 : movement
	                   , method == AlignMethod::HOR ? 0.0 : movement
	                   };
	
	ImageOffset result;
	GradientPlane plane( img1, img2, a1, a2, settings );
	int level = start_level;
	
	//Keep repeating with higher levels until it drops below threshold
	do{
		result = plane.findMinimum( { img1.getSize(), img2.getSize(), moves.x, moves.y, hint, level } );
	}while( result.error > max_difference && level++ < max_level );
	
	return { result.distance, result.error, img1, img2 };
}

