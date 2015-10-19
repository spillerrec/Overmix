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


#include "StatisticsRender.hpp"
#include "../color.hpp"

#include "../MultiPlaneIterator.hpp"

#include <algorithm>
#include <vector>

using namespace Overmix;

static void median_pixel( MultiPlaneLineIterator &it ){
	std::vector<color_type> all;
	for( unsigned i=1; i<it.size(); i++ )
		if( it.valid( i ) )
			all.push_back( it[i] );
	
	if( all.size() ){
		auto pos = all.size() / 2;
		std::nth_element( all.begin(), all.begin()+pos, all.end() );
		it[0] = all[pos];
	}
}
static void min_pixel( MultiPlaneLineIterator &it ){
	it[0] = color::MAX_VAL;
	for( unsigned i=1; i<it.size(); i++ )
		if( it.valid( i ) )
			it[0] = std::min( it[0], it[i] );
}
static void max_pixel( MultiPlaneLineIterator &it ){
	it[0] = color::MIN_VAL;
	for( unsigned i=1; i<it.size(); i++ )
		if( it.valid( i ) )
			it[0] = std::max( it[0], it[i] );
}

static void avg_pixel( MultiPlaneLineIterator &it ){
	unsigned amount = 0;
	precision_color_type sum = 0;
	for( unsigned i=1; i<it.size(); i++ ){
		if( it.valid( i ) ){
			sum += it[i];
			amount++;
		}
	}
	if( amount > 0 )
		it[0] = sum / amount;
}

static void difference_pixel( MultiPlaneLineIterator &it ){
	avg_pixel( it );
	
	//Calculate sum of the difference from average
	unsigned amount = 0;
	precision_color_type diff_sum = 0;
	for( unsigned i=1; i<it.size(); i++ )
		if( it.valid( i ) ){
			amount++;
			diff_sum += abs( it[0] - it[i] );
		}
	
	if( amount > 0 )
		it[0] = diff_sum / amount;
}


StatisticsRender::pixel_func* StatisticsRender::pixel() const{
	switch( function ){
		default:
		case Statistics::AVG: return &avg_pixel;
		case Statistics::DIFFERENCE: return &difference_pixel;
		case Statistics::MIN: return &min_pixel;
		case Statistics::MAX: return &max_pixel;
		case Statistics::MEDIAN: return &median_pixel;
	}
}
