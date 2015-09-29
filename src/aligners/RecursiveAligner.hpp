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

class RecursiveAligner : public AImageAligner{
	protected:
		std::pair<ImageGetter,Point<double>> combine( const ImageGetter& first, const ImageGetter& second ) const;
		ImageGetter align( AProcessWatcher* watcher, unsigned begin, unsigned end );
		
		virtual Plane prepare_plane( const Plane& ) const override{ return Plane(); };
		ImageGetter getGetter( unsigned index ) const;
		
	public:
		RecursiveAligner( AContainer& container, AlignMethod method, double scale=1.0 )
			:	AImageAligner( container, method, scale ){ }
		virtual void align( AProcessWatcher* watcher=nullptr ) override;
};

}

#endif