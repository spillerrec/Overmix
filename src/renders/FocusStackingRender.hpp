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

#ifndef FOCUS_STACKING_RENDER_HPP
#define FOCUS_STACKING_RENDER_HPP

#include "ARender.hpp"

#include "../color.hpp"
#include "../planes/PlaneBase.hpp"

namespace Overmix{

class FocusStackingRender : public ARender{
	protected:
		double blur_amount;
		int kernel_size;
		
	public:
		FocusStackingRender( double blur_amount, int kernel_size )
			:	blur_amount(blur_amount), kernel_size(kernel_size) { }
		
		virtual ImageEx render( const AContainer& group, AProcessWatcher* watcher=nullptr ) const override;
		
		void setBlurAmount( double val ){ blur_amount = val; }
};

}

#endif
