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

#include "Plane.hpp"
#include <limits>

#include <QtConcurrent>
#include <QDebug>

using namespace std;

static const double DOUBLE_MAX = numeric_limits<double>::max();

struct Sum{
	uint64_t total;
	Sum() : total( 0 ) { }
	void reduce( const uint64_t add ){ total += add; }
};
struct Para{
	color_type *c1;
	color_type *c2;
	unsigned width;
	unsigned stride;
	Para( color_type* c1, color_type *c2, unsigned width, unsigned stride )
		:	c1( c1 ), c2( c2 ), width( width ), stride( stride )
		{ }
};
static uint64_t diff_2_line( Para p ){
	uint64_t sum = 0;
	for( color_type* end=p.c1+p.width; p.c1<end; p.c1+=p.stride, p.c2+=p.stride )
		sum += abs( *p.c1 - *p.c2 );
	
	return sum;
}

double Plane::diff( const Plane& p, int x, int y, unsigned stride ) const{
	//Find edges
	int p1_top = y < 0 ? 0 : y;
	int p2_top = y > 0 ? 0 : -y;
	int p1_left = x < 0 ? 0 : x;
	int p2_left = x > 0 ? 0 : -x;
	unsigned width = min( get_width() - p1_left, p.get_width() - p2_left );
	unsigned height = min( get_height() - p1_top, p.get_height() - p2_top );
	
	//Initial offsets on the two planes
	color_type* c1 = scan_line( p1_top ) + p1_left;
	color_type* c2 = p.scan_line( p2_top ) + p2_left;
	
	//Calculate all the offsets for QtConcurrent::mappedReduced
	vector<Para> lines;
	lines.reserve( height );
	for( unsigned i=0; i<height; i+=stride ){
		lines.push_back( Para( c1+i%stride, c2+i%stride, width, stride ) );
		c1 += line_width;
		c2 += p.line_width;
	}
	
	Sum sum = QtConcurrent::blockingMappedReduced( lines, &diff_2_line, &Sum::reduce );
	return sum.total / (double)( height * width / (stride*stride) );
}


//TODO: these two are brutally simple, improve?
double DiffCache::get_diff( int x, int y ) const{
	for( auto c : cache )
		if( c.x == x && c.y == y ){
		//	qDebug( "Reusing diff: %dx%d with %.2f", x, y, c.diff );
			return c.diff;
		}
	return -1;
}
void DiffCache::add_diff( int x, int y, double diff ){
	Cached c = { x, y, diff };
	cache << c;
}



struct img_comp{
	const Plane& img1;
	const Plane& img2;
	int h_middle;
	int v_middle;
	double diff;
	int level;
	int left;
	int right;
	int top;
	int bottom;
	bool diff_set;
	
	img_comp( const Plane& image1, const Plane& image2, int hm, int vm, int lvl=0, int l=0, int r=0, int t=0, int b=0 )
		:	img1( image1 ), img2( image2 )
		,	h_middle( hm ), v_middle( vm )
		,	diff( -1 )
		,	level( lvl )
		,	left( l ), right( r )
		,	top( t ), bottom( b )
		,	diff_set( false )
		{ }
	void do_diff( int x, int y ){
		if( !diff_set )
			diff = img1.diff( img2, x, y );
	}
	void set_diff( double new_diff ){
		diff = new_diff;
		if( diff >= 0 )
			diff_set = true;
	}
	
	MergeResult result( DiffCache *cache ) const{
		if( level > 0 )
			return img1.best_round_sub( img2, level, left, right, top, bottom, cache );
		else
			return MergeResult(QPoint( h_middle, v_middle ),diff);
	}
};

static void do_diff_center( img_comp& comp ){
	comp.do_diff( comp.h_middle, comp.v_middle );
}
MergeResult Plane::best_round_sub( const Plane& p, int level, int left, int right, int top, int bottom, DiffCache *cache ) const{
//	qDebug( "Round %d: %d,%d x %d,%d", level, left, right, top, bottom );
	std::vector<img_comp> comps;
	int amount = level*2 + 2;
	double h_offset = (double)(right - left) / amount;
	double v_offset = (double)(bottom - top) / amount;
	level = level > 1 ? level-1 : 1;
	
	if( h_offset < 1 && v_offset < 1 ){
		//Handle trivial step
		//Check every diff in the remaining area
		for( int ix=left; ix<=right; ix++ )
			for( int iy=top; iy<=bottom; iy++ ){
				img_comp t( *this, p, ix, iy );
				t.set_diff( cache->get_diff( ix, iy ) );
				comps.push_back( t );
			}
	}
	else{
		//Make sure we will not do the same position multiple times
		double h_add = ( h_offset < 1 ) ? 1 : h_offset;
		double v_add = ( v_offset < 1 ) ? 1 : v_offset;
		
		for( double iy=top+v_offset; iy<=bottom; iy+=v_add )
			for( double ix=left+h_offset; ix<=right; ix+=h_add ){
				int x = ( ix < 0.0 ) ? ceil( ix-0.5 ) : floor( ix+0.5 );
				int y = ( iy < 0.0 ) ? ceil( iy-0.5 ) : floor( iy+0.5 );
				
				//Avoid right-most case. Can't be done in the loop
				//as we always want it to run at least once.
				if( ( x == right && x != left ) || ( y == bottom && y != top ) )
					continue;
				
				//Create and add
				img_comp t(
						*this, p, x, y, level
					,	floor( ix - h_offset ), ceil( ix + h_offset )
					,	floor( iy - v_offset ), ceil( iy + v_offset )
					);
				
				t.set_diff( cache->get_diff( x, y ) );
				
				comps.push_back( t );
			}
	}
	
	//Calculate diffs
	QtConcurrent::map( comps, do_diff_center ).waitForFinished();
	
	//Find best comp
	const img_comp* best = NULL;
	double best_diff = DOUBLE_MAX;
	
	for( auto &c : comps ){
		if( c.diff < best_diff ){
			best = &c;
			best_diff = best->diff;
		}
		
		//Add to cache
		if( !c.diff_set )
			cache->add_diff( c.h_middle, c.v_middle, c.diff );
	}
	
	if( !best ){
		qDebug( "ERROR! no result to continue on!!" );
		return MergeResult(QPoint(),DOUBLE_MAX);
	}
	
	return best->result( cache );
}

