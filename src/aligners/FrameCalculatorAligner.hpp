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

#ifndef FRAME_CALCULATOR_ALIGNER_HPP
#define FRAME_CALCULATOR_ALIGNER_HPP

#include "AAligner.hpp"

#include <stdexcept>

namespace Overmix{

class FrameCalculatorAligner : public AAligner{
	private:
		int offset;
		int amount;
		int repeats;
	
	public:
		FrameCalculatorAligner( int offset, int amount, int repeats )
			:	offset(offset), amount(amount), repeats(repeats) { }
		
		virtual void align( AContainer& container, AProcessWatcher* ) const override{
			//Validate input
			auto max_frames = amount * repeats;
			if( amount < 1 || repeats < 1 )
				throw std::runtime_error( "Invalid arguments, no frames would be generated" );
			if( max_frames < offset )
				throw std::runtime_error( "Offset is bigger than frames per cycle" );
			
			//Calculate offsets
			for( unsigned i=0; i<container.count(); i++ )
				container.setFrame( i, ((i + max_frames - offset)/repeats) % amount );
				//Adding max_frames to be sure it is positive before MOD
		}
};

}

#endif