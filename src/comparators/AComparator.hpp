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

#ifndef A_COMPARATOR_HPP
#define A_COMPARATOR_HPP

#include "../Geometry.hpp"
#include "../utils/PlaneUtils.hpp"

namespace Overmix{

class Plane;

struct ImageOffset{
	Point<double> distance { 0, 0 };
	Point<double> scale    { 1, 1 };
	double rotation {  0 };
	double error    { -1 };
	double overlap  { -1 };

	ImageOffset() = default;
	ImageOffset( Point<double> distance, double error, double overlap )
		: distance(distance), error(error), overlap(overlap) { }
	ImageOffset( Point<double> distance, double error, const Plane& img1, const Plane& img2 );
	bool isValid() const{ return overlap >= 0.0; }
	
	ImageOffset reverse() const{ return { {-distance.x, -distance.y}, error, overlap }; }
	
	static double calculate_overlap( Point<> offset, const Plane& img1, const Plane& img2 );
};

/// Interface for comparing two images and finding alignment between them.
/// AAligner will use the AComparator stored in AContainer to align more than two images.
/// To support comparing in different domains (such as doing edge detection first), process/processAlpha is used before the
/// result is passed to findOffset.
/// This might need some more consideration on how it should work
class AComparator{
	public:
		/// @brief Scaling for sub-pixel alignment. `process()` will return an image scaled by this amount
		/// @return The scaling factors for sub-pixel alignment
		virtual Size<double> scale() const { return { 1.0, 1.0 }; }

		/// @brief Optionally pre-processes the input plane before aligning.
		/// @param plane The input plane. Must stay alive as long as you hold a reference to the returned ModifiedPlane
		/// @return The input plane with any pre-processing if needed.
		virtual ModifiedPlane process( const Plane& plane ) const { return { plane }; }

		/// @brief Corresponding `process()` for the alpha channel. This should be overloaded if you transform the coordinate space,
		/// for example by scaling the images. 
		/// @param plane The input alpha plane. Must stay alive as long as you hold a reference to the returned ModifiedPlane
		/// @return An alpha plane compatible with the result from `process()`
		virtual ModifiedPlane processAlpha( const Plane& plane ) const { return { plane }; }

		/// @brief Finds the alignment between two images
		/// @param img1 Image which will be the reference for the alignment
		/// @param img2 The image which we want to find how much has been transformed
		/// @param a1 Optional cooresponding alpha plane to `img1`
		/// @param a2 Optional cooresponding alpha plane to `img2`
		/// @param hint Initial guess of how the images are aligned, which can speed up some algorithms
		/// @return An `ImageOffset` containing the result of the alignment detection
		virtual ImageOffset findOffset( const Plane& img1, const Plane& img2, const Plane& a1, const Plane& a2, Point<double> hint = {0.0,0.0} ) const = 0;

		virtual ~AComparator() = default;
		
		/// @brief Gives an error value for two images at a specific translation in an unspecifed metric specific to this Comparator.
		/// @todo Suppport rotation/scaling
		/// @param img1 Base image (offset 0,0)
		/// @param img2 Compare image (offset `x`,`y`)
		/// @param a1 Optional alpha for `img1`
		/// @param a2 Optional alpha for `img2`
		/// @param x The amount of translation in the horizontal direction
		/// @param y The amount of translation in the vertical direction
		/// @return The error for this alignment. Larger values mean worse alignment, but the scale is unspecified
		virtual double findError( const Plane& img1, const Plane& img2, const Plane& a1, const Plane& a2, double x, double y ) const = 0;

		/// @return True if `findOffset` does allow rotation and/or scaling to be detected, or false if only translation is considered
		virtual bool includesRotationOrScale() const { return false; }
};

}

#endif
