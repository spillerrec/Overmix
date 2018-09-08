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

#ifndef ANIMATION_SEPARATOR_HPP
#define ANIMATION_SEPARATOR_HPP

#include "AAligner.hpp"

namespace Overmix{

class AnimationSeparator : public AAligner{
	private:
		bool skip_align { false };
		double threshold_factor{ 1.0 };
		
		double findThreshold( AContainer& container, AProcessWatcher* watcher ) const;
		double findError( AContainer& container, int index1, int index2 ) const;
		
	public:
		AnimationSeparator( bool skip_align = false, double threshold_factor = 1.0 )
			: skip_align(skip_align), threshold_factor(threshold_factor)
			{ }
		virtual void align( AContainer& container, AProcessWatcher* watcher=nullptr ) const override;
};

}

#endif
