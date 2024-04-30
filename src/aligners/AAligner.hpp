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

#ifndef A_ALIGNER_HPP
#define A_ALIGNER_HPP

namespace Overmix{

enum class AlignMethod{ //TODO: We have this twice
		BOTH
	,	VER
	,	HOR
};


/// @brief Base class for all higher-level alignment methods which aligns several images into a single panorama image.
/// The lower level alignment between two images are handled by AComparator, which AAligner sub-classes should use (if possible)
/// to align an arbitary amount of images together.
class AAligner{
	public:
		virtual ~AAligner() { }
		
		/// @brief Aligns all the images in the specified AContainer. The container must return a valid AComparator
		/// and some implementations might have further requirements. The initial frame offsets might be preserved,
		/// but usually they are ignored and completely replaced with the results.
		/// In order to only align a sub-set of images, wrap the container using a meta AContainer such as FrameContainer
		/// On failure an exception will be thrown.
		/// @param container The container with the images to align. Must have a AComparator set
		/// @param watcher If progress monitoring is wanted, set this to non-null to receive progress updates during this call
		virtual void align( class AContainer& container, class AProcessWatcher* watcher=nullptr ) const = 0;
};

}

#endif