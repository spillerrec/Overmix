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

#ifndef RECURSIVE_ALIGNER_HPP
#define RECURSIVE_ALIGNER_HPP

#include "AImageAligner.hpp"
#include <utility>

namespace Overmix{

class ImageGetter;

/** Aligns the container using a divide and conquer algorithm. Assumes that each image
  * overlaps the images right next to it. */
class RecursiveAligner : public AAligner{
	protected:
		AlignerProcessor process;
		
		std::pair<ImageGetter,Point<double>> combine( const ImageGetter& first, const ImageGetter& second ) const;
		ImageGetter align( AContainer& container, AProcessWatcher* watcher, unsigned begin, unsigned end ) const;
		
		ImageGetter getGetter( const AContainer& container, unsigned index ) const;
		
	public:
		RecursiveAligner( AlignMethod method, double scale=1.0 )
			: process( method, scale ) { }
		virtual void align( AContainer& container, AProcessWatcher* watcher=nullptr ) const override;
};

}

#endif