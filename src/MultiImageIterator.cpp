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


#include "MultiImageIterator.h"


MultiImageIterator::MultiImageIterator( const std::vector<QImage> &images, const std::vector<QPoint> &points, unsigned x, unsigned y )
	:	imgs( images), pos( points ){
	values.reserve( imgs.size() );
	line_width.reserve( imgs.size() );
	lines.reserve( imgs.size() );
	new_y( y );
	new_x( x );
}


void MultiImageIterator::new_x( unsigned x ){
	int offset = x - current_x;
	current_x = x;
	
	for( unsigned i=0; i<lines.size(); i++ )
		lines[i] += offset;
	
	values.clear();
}


void MultiImageIterator::new_y( unsigned y ){
	//Chech cache
	if( y == current_y && lines.size() != 0 )
		return;
	
	line_width.clear();
	lines.clear();
	
	for( unsigned iy=0; iy<imgs.size(); iy++ ){
		QPoint p = pos[iy];
		int new_y = y - p.y();
		if( new_y >= 0 && new_y < imgs[iy].height() ){
			lines.push_back( (const QRgb*)imgs[iy].constScanLine( new_y ) - p.x() + current_x );
			line_width.push_back( Line( p.x(), p.x() + imgs[iy].width() ) );
		}
	}
	
	current_y = y;
	values.clear();
}

void MultiImageIterator::fill_values(){
	if( values.size() == 0 )
		for( unsigned i=0; i<lines.size(); i++ )
			if( current_x >= line_width[i].first && current_x < line_width[i].second )
				values.push_back( color( *(lines[i]) ) );
}

color MultiImageIterator::average(){
	color avg;
	fill_values();
	
	if( values.size() ){
		for( unsigned i=0; i<values.size(); i++ )
			avg += values[i];
		avg /= values.size();
	}
	
	return avg;
}

color MultiImageIterator::simple_filter( unsigned threshould ){
	fill_values();
	
	//Calculate average:
	color avg;
	for( unsigned i=0; i<values.size(); i++ )
		avg += values[i];
	avg /= values.size();
	
	//Calculate value
	color r;
	unsigned amount = 0;
	for( unsigned i=0; i<values.size(); i++ ){
		//Find difference from average
		color d( values[i] );
		d.diff( avg );
		
		//Find the largest difference
		unsigned max = d.r > d.g ? d.r : d.g;
		max = d.b > max ? d.b : max;
		
		//Only apply if below threshould
		if( max <= threshould ){
			r += values[i];
			amount++;
		}
	}
	
	//Let it be transparent if no amount
	if( amount ){
		r.a *= amount;
		return r / amount;
	}
	else
		return color( 0,0,255*256 );
}

color MultiImageIterator::simple_slide( unsigned threshould ){
	fill_values();
	
	unsigned best = 0;
	color best_color;
	for( unsigned i=0; i<values.size(); i++ ){
		unsigned amount = 0;
		color avg;
		
		for( unsigned j=0; j<values.size(); j++ ){
			color d( values[i] );
			d.diff( values[j] );
			
			unsigned max = d.r > d.g ? d.r : d.g;
			max = d.b > max ? d.b : max;
			if( max <= threshould ){
				amount++;
				avg += values[j];
			}
		}
		
		if( amount > best ){
			best = amount;
			best_color = avg / amount;
		}
	}
	
	
	if( best ){
	//	if( do_diff )
	//		best_color.r = best_color.g = best_color.b = 255*256 * best / pixels.size();
		return best_color;
	}
	else
		return color( 0,0,255*256 );
}
