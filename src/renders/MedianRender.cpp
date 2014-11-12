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


#include "MedianRender.hpp"

#include <algorithm>
#include <vector>

static void median_pixel( MultiPlaneLineIterator &it ){
	std::vector<color_type> all;
	for( unsigned i=1; i<it.size(); i++ )
		if( it.valid( i ) )
			all.push_back( it[i] );
	
	std::sort( all.begin(), all.end() );
	
	if( all.size() )
		it[0] = all[all.size()/2];
}
MedianRender::pixel_func* MedianRender::pixel() const{
	return &median_pixel;
}
