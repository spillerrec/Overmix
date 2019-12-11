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

#ifndef SR_SAMLE_CREATOR_HPP
#define SR_SAMLE_CREATOR_HPP

namespace Overmix{

class ImageEx;
class ImageContainer;

class SRSampleCreator{
	public:
		int scale{ 4 };
		bool use_random_movement { false };
		int sample_count{ 16 };
		
	private:
		
		
	public:
		void render( ImageContainer& add_to, const ImageEx& img ) const;
};

}

#endif