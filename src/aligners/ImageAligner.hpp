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

#ifndef IMAGE_ALIGNER_HPP
#define IMAGE_ALIGNER_HPP

#include "AImageAligner.hpp"

namespace Overmix{

class ImageAligner : public AImageAligner{
	protected:
		std::vector<ImageOffset> offsets;
		
		ImageOffset get_offset( unsigned img1, unsigned img2 ) const;
		
		void rough_align();
		
		double total_error() const;
		virtual void on_add();
	
	public:
		ImageAligner( AContainer& container, AlignMethod method, double scale=1.0 )
			:	AImageAligner( container, method, scale ){ }
		
		void align( AProcessWatcher* watcher=nullptr );
	
};

}

#endif