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
#include <algorithm> //For min
#include <cstdint> //For abs(int) and uint*_t
#include <limits>
#include <vector>
#include <cstring> //For memcpy

#include <QtConcurrent>
#include <QDebug>

using namespace std;

const unsigned long Plane::MAX_VAL = 0xFFFFFFFF;
static const double DOUBLE_MAX = numeric_limits<double>::max();

Plane::Plane( unsigned w, unsigned h ){
	height = h;
	width = w;
	line_width = w;
	data = new color_type[ h * line_width ];
}

Plane::~Plane(){
	if( data )
		delete[] data;
}

Plane::Plane( const Plane& p ){
	height = p.height;
	width = p.width;
	line_width = p.line_width;
	
	unsigned size = height * line_width;
	data = new color_type[ size ];
	memcpy( data, p.data, sizeof(color_type)*size );
}

void Plane::fill( color_type value ){
	for( unsigned iy=0; iy<get_height(); ++iy ){
		color_type* row = scan_line( iy );
		for( unsigned ix=0; ix<get_width(); ++ix )
			row[ix] = value;
	}
}

bool Plane::is_interlaced() const{
	double avg2_uneven = 0, avg2_even = 0;
	for( unsigned iy=0; iy<get_height()/4*4; ){
		color_type *row1 = scan_line( iy++ );
		color_type *row2 = scan_line( iy++ );
		color_type *row3 = scan_line( iy++ );
		color_type *row4 = scan_line( iy++ );
		
		unsigned long long line_avg_uneven = 0, line_avg_even = 0;
		for( unsigned ix=0; ix<get_width(); ++ix ){
			color_type diff_uneven = abs( row2[ix]-row1[ix] ) + abs( row4[ix]-row3[ix] );
			color_type diff_even = abs( row3[ix]-row1[ix] ) + abs( row4[ix]-row2[ix] );
			line_avg_uneven += (unsigned long long)diff_uneven*diff_uneven;
			line_avg_even += (unsigned long long)diff_even*diff_even;
		}
		avg2_uneven += (double)line_avg_uneven / get_width();
		avg2_even   += (double)line_avg_even   / get_width();
	}
	avg2_uneven /= get_height()/2;
	avg2_uneven /= 0xFFFF;
	avg2_uneven /= 0xFFFF;
	avg2_even /= get_height()/2;
	avg2_even /= 0xFFFF;
	avg2_even /= 0xFFFF;
	
	qDebug( "interlace factor: %f > %f", avg2_uneven, avg2_even );
	return avg2_uneven > avg2_even;
}

void Plane::replace_line( Plane &p, bool top ){
	if( get_height() != p.get_height() || get_width() != p.get_width() ){
		qWarning( "replace_line: Planes not equaly sized!" );
		return;
	}
	
	for( unsigned iy=(top ? 0 : 1); iy<get_height(); iy+=2 ){
		color_type *row1 = scan_line( iy );
		color_type *row2 = p.scan_line( iy );
		
		for( unsigned ix=0; ix<get_width(); ++ix )
			row1[ix] = row2[ix];
	}
}

void Plane::combine_line( Plane &p, bool top ){
	if( get_height() != p.get_height() || get_width() != p.get_width() ){
		qWarning( "combine_line: Planes not equaly sized!" );
		return;
	}
	
	for( unsigned iy=(top ? 0 : 1); iy<get_height(); iy+=2 ){
		color_type *row1 = scan_line( iy );
		color_type *row2 = p.scan_line( iy );
		
		for( unsigned ix=0; ix<get_width(); ++ix )
			row1[ix] = ( (unsigned)row1[ix] + row2[ix] ) / 2;
	}
}


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


#include <QTime>
Plane* Plane::scale_nearest( unsigned wanted_width, unsigned wanted_height ) const{
	Plane *scaled = new Plane( wanted_width, wanted_height );
	if( !scaled || scaled->is_invalid() )
		return 0;
	
	for( unsigned iy=0; iy<wanted_height; iy++ ){
		color_type* row = scaled->scan_line( iy );
		for( unsigned ix=0; ix<wanted_width; ix++ ){
			double pos_x = ((double)ix / (wanted_width-1)) * (width-1);
			double pos_y = ((double)iy / (wanted_height-1)) * (height-1);
			
			row[ix] = pixel( round( pos_x ), round( pos_y ) );
		}
	}
	
	return scaled;
}

double Plane::linear( double x ){
	x = abs( x );
	if( x <= 1.0 )
		return x;
	return 0;
}

double Plane::cubic( double b, double c, double x ){
	x = abs( x );
	
	if( x < 1 )
		return
				(12 - 9*b - 6*c)/6 * x*x*x
			+	(-18 + 12*b + 6*c)/6 * x*x
			+	(6 - 2*b)/6
			;
	else if( x < 2 )
		return
				(-b - 6*c)/6 * x*x*x
			+	(6*b + 30*c)/6 * x*x
			+	(-12*b - 48*c)/6 * x
			+	(8*b + 24*c)/6
			;
	else
		return 0;
}


struct ScalePoint{
	unsigned start;
	vector<double> weights; //Size of this is end
	
	ScalePoint( unsigned index, unsigned width, unsigned wanted_width, double window, Plane::Filter f ){
		
		double pos = ((double)index / (wanted_width-1)) * (width-1);
		start = (unsigned)max( (int)ceil( pos-window ), 0 );
		unsigned end = min( (unsigned)floor( pos+window ), width-1 );
		
		weights.reserve( end - start + 1 );
		for( unsigned j=start; j<=end; ++j )
			weights.push_back( f( pos - j ) );
	}
};

struct ScaleLine{
	std::vector<ScalePoint>& points;
	Plane& wanted;
	unsigned index;
	unsigned width;
	
	color_type* row; //at (0,index)
	unsigned line_width;
	double window;
	Plane::Filter f;
	
	ScaleLine( std::vector<ScalePoint>& points, Plane &wanted, unsigned index, unsigned width )
		:	points(points), wanted(wanted), index(index), width(width) { }
};

void do_line( const ScaleLine& line ){
	color_type *out = line.wanted.scan_line( line.index );
	ScalePoint ver( line.index, line.width, line.wanted.get_height(), line.window, line.f );
	
	for( auto x : line.points ){
		double avg = 0;
		double amount = 0;
		color_type* row = line.row - (line.index-ver.start)*line.line_width + x.start;
		
		for( auto wy : ver.weights ){
			color_type* row2 = row;
			for( auto wx : x.weights ){
				double weight = wy * wx;
				avg += *(row2++) * weight;
				amount += weight;
			}
			
			row += line.line_width;
		}
		
		//TODO: limit negative numbers
		*(out++) = amount ? std::min( unsigned( avg / amount + 0.5 ), 0xFFFFu ) : 0;
		
	}
}

Plane* Plane::scale_generic( unsigned wanted_width, unsigned wanted_height, double window, Plane::Filter f ) const{
	Plane *scaled = new Plane( wanted_width, wanted_height );
	if( !scaled || scaled->is_invalid() )
		return scaled;
	
	QTime t;
	t.start();
	
	//Calculate all x-weights
	std::vector<ScalePoint> points;
	points.reserve( wanted_width );
	for( unsigned ix=0; ix<wanted_width; ++ix ){
		ScalePoint p( ix, width, wanted_width, window, f );
		points.push_back( p );
	}
	
	//Calculate all y-lines
	std::vector<ScaleLine> lines;
	for( unsigned iy=0; iy<wanted_height; ++iy ){
		ScaleLine line( points, *scaled, iy, height );
		line.row = scan_line( iy );
		line.line_width = line_width;
		line.window = window;
		line.f = f;
		lines.push_back( line );
	}
	
	QtConcurrent::blockingMap( lines, &do_line );
	
	qDebug( "Resize took: %d msec", t.restart() );
	return scaled;
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


template<typename T>
struct EdgeLine{
	//Output
	color_type* out;
	unsigned width;
	
	//Kernels
	unsigned size;
	T *weights_x;
	T *weights_y;
	unsigned div;
	
	//Operator
	typedef color_type (*func_t)( const EdgeLine<T>&, color_type* );
	func_t func;
	
	//Input, with line_width as we need several lines
	color_type* in;
	unsigned line_width;
};

template<typename T>
static T calculate_kernel( T *kernel, unsigned size, color_type* in, unsigned line_width ){
	T sum = 0;
	for( unsigned iy=0; iy<size; ++iy ){
		color_type *row = in + iy*line_width;
		for( unsigned ix=0; ix<size; ++ix, ++kernel, ++row )
			sum += *kernel * *row;
	}
	return sum;
}

template<typename T>
static color_type calculate_edge( const EdgeLine<T>& line, color_type* in ){
	using namespace std;
	
	int sum_x = calculate_kernel( line.weights_x, line.size, in, line.line_width );
	int sum_y = calculate_kernel( line.weights_y, line.size, in, line.line_width );
	int sum = abs( sum_x ) + abs( sum_y );
	sum /= line.div;
	return min( max( sum, 0 ), 256*256-1 );
}

template<typename T>
static color_type calculate_zero_edge( const EdgeLine<T>& line, color_type* in ){
	using namespace std;
	//TODO: improve
	int sum = max( calculate_kernel( line.weights_x, line.size, in, line.line_width ), 0 );
	sum /= line.div;
	return min( max( sum, 0 ), 256*256-1 );
}

template<typename T>
static void edge_line( const EdgeLine<T>& line ){
	color_type *in = line.in;
	color_type *out = line.out;
	unsigned size_half = line.size/2;
	
	//Fill the start of the row with the same value
	color_type first = line.func( line, in );
	for( unsigned ix=0; ix<size_half; ++ix )
		*(out++) = first;
	
	unsigned end = line.width - (line.size-size_half);
	for( unsigned ix=size_half; ix<end; ++ix, ++in )
		*(out++) = line.func( line, in );
	
	//Repeat the end with the same value
	color_type last = *(out-1);
	for( unsigned ix=end; ix<line.width; ++ix )
		*(out++) = last;
}


template<typename T, typename T2>
Plane* parallel_edge_line( const Plane& p, T* weights_x, T* weights_y, unsigned size, unsigned div, T2 func ){
	Plane *out = new Plane( p.get_width(), p.get_height() );
	if( !out || out->is_invalid() )
		return out;
	
	//Calculate all y-lines
	std::vector<EdgeLine<T> > lines;
	for( unsigned iy=0; iy<p.get_height(); ++iy ){
		EdgeLine<T> line;
		line.out = out->scan_line( iy );
		line.width = p.get_width();
		
		line.size = size;
		line.weights_x = weights_x;
		line.weights_y = weights_y;
		line.div = div;
		
		line.func = func;
		
		line.in = p.scan_line( std::min( std::max( int(iy-size/2), 0 ), int(p.get_height()-size-1) ) ); //Always stay inside
		line.line_width = p.get_line_width();
		
		lines.push_back( line );
	}
	
	QtConcurrent::blockingMap( lines, &edge_line<T> );
	return out;
}

Plane* Plane::edge_zero_generic( int *weights, unsigned size, unsigned div ) const{
	return parallel_edge_line( *this, weights, (int*)NULL, size, div, calculate_zero_edge<int> );
}

Plane* Plane::edge_dm_generic( int *weights_x, int *weights_y, unsigned size, unsigned div ) const{
	return parallel_edge_line( *this, weights_x, weights_y, size, div, calculate_edge<int> );
}


struct SimplePixel{
	color_type *row1;
	color_type *row2;
	unsigned width;
	void (*f)( const SimplePixel& );
	void *data;
};

static void do_pixel_line( SimplePixel pix ){
	for( unsigned i=0; i<pix.width; ++i ){
		pix.f( pix );
		++pix.row1;
		++pix.row2;
	}
}


static void substract_pixel( const SimplePixel& pix ){
	*pix.row1 = std::max( (int)*pix.row2 - (int)*pix.row1, 0 );
}


void Plane::substract( Plane &p ){
	if( get_height() != p.get_height() || get_width() != p.get_width() ){
		qWarning( "replace_line: Planes not equaly sized!" );
		return;
	}
	
	std::vector<SimplePixel> lines;
	for( unsigned iy=0; iy<get_height(); ++iy ){
		SimplePixel pix = { scan_line( iy )
			,	p.scan_line( iy )
			,	get_width()
			,	&substract_pixel
			,	0
			};
		lines.push_back( pix );
	}
	
	QtConcurrent::blockingMap( lines, &do_pixel_line );
}


struct LevelOptions{
	color_type limit_min;
	color_type limit_max;
	color_type output_min;
	color_type output_max;
	double gamma;
};

static void level_pixel( const SimplePixel& pix ){
	LevelOptions *opt = (LevelOptions*)pix.data;
	
	//Limit
	double scale = 1.0 / ( opt->limit_max - opt->limit_min );
	double val = ( (int)*pix.row1 - opt->limit_min ) * scale;
	val = std::min( std::max( val, 0.0 ), 1.0 );
	
	//Gamma
	if( opt->gamma != 1.0 )
		val = std::pow( val, opt->gamma );
	
	//Output
	scale = opt->output_max - opt->output_min;
	*pix.row1 = opt->output_min + std::round( val * scale );
}

Plane* Plane::level(
		color_type limit_min
	,	color_type limit_max
	,	color_type output_min
	,	color_type output_max
	,	double gamma
	) const{
	Plane *out = new Plane( *this );
	if( !out )
		return out;
	
	//Don't do anything if nothing will change
	if( limit_min == output_min
		&&	limit_max == output_max
		&&	limit_min == 0
		&&	limit_max == (256*256-1)
		&&	gamma == 1.0
		)
		return out;
	
	QTime t;
	t.start();
	
	
	LevelOptions options = {
				limit_min
			,	limit_max
			,	output_min
			,	output_max
			,	gamma
		};
	
	std::vector<SimplePixel> lines;
	for( unsigned iy=0; iy<out->get_height(); ++iy ){
		SimplePixel pix = { out->scan_line( iy )
			,	NULL
			,	out->get_width()
			,	&level_pixel
			,	&options
			};
		lines.push_back( pix );
	}
	
	QtConcurrent::blockingMap( lines, &do_pixel_line );
	
	
	qDebug( "Level took: %d msec", t.restart() );
	return out;
}


struct WeightedSumLine{
	color_type *out;	//Output row
	color_type *in; //Input row, must be the top row weights affects
	unsigned width;
	unsigned line_width; //For input row
	
	//Only bounds-checked in the horizontal direction!
	double *weights;
	unsigned w_width;
	unsigned w_height; //How many input lines should be weighted, must not overflow!
	double full_sum;
	
	double calculate_sum( unsigned start=0 ) const{
		double sum = 0;
		for( unsigned iy=0; iy<w_height; ++iy )
			for( unsigned ix=start; ix<w_width; ++ix )
				sum += weights[ ix + iy*w_width ];
		return sum;
	}
	
	color_type weighted_sum( color_type* in ) const{
		double sum = 0;
		for( unsigned iy=0; iy<w_height; ++iy ){
			color_type *line = in + iy*line_width;
			for( unsigned ix=0; ix<w_width; ++ix, ++line )
				sum += weights[ ix + iy*w_width ] * (*line);
		}
		
		sum /= full_sum;
		return std::round( std::max( sum, 0.0 ) );
		
	}
	color_type weighted_sum( color_type* in, int cutting ) const{
		unsigned start = cutting > 0 ? 0 : -cutting;
		unsigned end = cutting > 0 ? w_width-cutting : w_width;
		
		double sum = 0;
		double w_sum = 0;
		for( unsigned iy=0; iy<w_height; ++iy ){
			color_type *line = in + iy*line_width;
			for( unsigned ix=start; ix<end; ++ix, ++line ){
				double w = weights[ ix + iy*w_width ];
				sum += w * (*line);
				w_sum += w;
			}
		}
		
		sum /= w_sum;
		return std::round( std::max( sum, 0.0 ) );
	}
};

void sum_line( const WeightedSumLine& line ){
	color_type *in = line.in;
	color_type *out = line.out;
	unsigned size_half = line.w_width/2;
	
	//Fill the start of the row with the same value
	for( unsigned ix=0; ix<size_half; ++ix )
		*(out++) = line.weighted_sum( in, size_half-ix );
	
	unsigned end = line.width - (line.w_width-size_half);
	for( unsigned ix=size_half; ix<=end; ++ix, ++in )
		*(out++) = line.weighted_sum( in );
	
	//Repeat the end with the same value
	for( unsigned ix=end+1; ix<line.width; ++ix, ++in )
		*(out++) = line.weighted_sum( in, (int)end-ix );
}

Plane* Plane::weighted_sum( double *kernel, unsigned w_width, unsigned w_height ) const{
	if( !kernel || w_width == 0 || w_height == 0 )
		return NULL;
	
	Plane *out = create_compatiable();
	if( out ){
		//Set default settings
		WeightedSumLine default_line;
		default_line.width = width;
		default_line.line_width = out->get_line_width();
		default_line.weights = kernel;
		default_line.w_width = w_width;
		default_line.w_height = w_height;
		default_line.full_sum = default_line.calculate_sum();
		
		int half_size = w_height / 2;
		std::vector<WeightedSumLine> lines;
		for( unsigned iy=0; iy<height; ++iy ){
			WeightedSumLine line = default_line;
			line.out = out->scan_line( iy );
			
			int top = iy - half_size;
			if( top < 0 ){
				//Cut stuff from top
				line.w_height += top; //Subtracts!
				line.weights += -top * line.w_width;
				line.in = scan_line( 0 );
				line.full_sum = line.calculate_sum();
			}
			else if( top+w_height >= height ){
				//Cut stuff from bottom
				line.in = scan_line( top );
				line.w_height = height - top;
				line.full_sum = line.calculate_sum();
			}
			else //Use defaults
				line.in = scan_line( top );
			
			lines.push_back( line );
		}
		
		QtConcurrent::blockingMap( lines, &sum_line );
	}
	
	return out;
}

Plane* Plane::blur_box( unsigned amount_x, unsigned amount_y ) const{
	unsigned size = ++amount_x * ++amount_y;
	double *kernel = new double[ size ];
	if( !kernel )
		return NULL;
	
	for( unsigned iy=0; iy<amount_y; ++iy )
		for( unsigned ix=0; ix<amount_x; ++ix )
			kernel[ ix + iy*amount_x ] = 1.0;
	
	Plane *p = weighted_sum( kernel, amount_x, amount_y );
	delete[] kernel;
	return p;
}

const double PI = std::atan(1)*4;
static double gaussian( double dx, double dy, double devi ){
	double base = 1.0 / ( 2*PI*devi*devi );
	double power = -( dx*dx + dy*dy ) / ( 2*devi*devi );
	return base * exp( power );
}

double* Plane::gaussian_kernel( unsigned amount_x, unsigned amount_y ) const{
	unsigned size = ++amount_x * ++amount_y;
	double *kernel = new double[ size ];
	if( !kernel )
		return NULL;
	
	//Estimate deviation
	double devi = std::max( amount_x, amount_y ) / 6;
	
	double half_x = amount_x/2.0;
	double half_y = amount_y/2.0;
	for( unsigned iy=0; iy<amount_y; ++iy )
		for( unsigned ix=0; ix<amount_x; ++ix )
			kernel[ ix + iy*amount_x ] = gaussian( ix-half_x, iy-half_y, devi );
	
	return kernel;
}

Plane* Plane::blur_gaussian( unsigned amount_x, unsigned amount_y ) const{
	double *kernel = gaussian_kernel( amount_x, amount_y );
	if( !kernel )
		return nullptr;
	
	Plane *p = weighted_sum( kernel, amount_x+1, amount_y+1 );
	delete[] kernel;
	return p;
}


static void divide_pixel( const SimplePixel& pix ){
	double val1 = (double)*pix.row1 / (double)(256*256-1);
	double val2 = (double)*pix.row2 / (double)(256*256-1);
	*pix.row1 = std::round( val2 / val1 * (256*256-1) );
}
static void multiply_pixel( const SimplePixel& pix ){
	double val1 = (double)*pix.row1 / (double)(256*256-1);
	double val2 = (double)*pix.row2 / (double)(256*256-1);
	*pix.row1 = std::round( val1 * val2 * (256*256-1) );
}
void Plane::divide( Plane &p ){
	if( get_height() != p.get_height() || get_width() != p.get_width() ){
		qWarning( "replace_line: Planes not equaly sized!" );
		return;
	}
	
	std::vector<SimplePixel> lines;
	for( unsigned iy=0; iy<get_height(); ++iy ){
		SimplePixel pix = { scan_line( iy )
			,	p.scan_line( iy )
			,	get_width()
			,	&divide_pixel
			,	0
			};
		lines.push_back( pix );
	}
	
	QtConcurrent::blockingMap( lines, &do_pixel_line );
}
void Plane::multiply( Plane &p ){
	if( get_height() != p.get_height() || get_width() != p.get_width() ){
		qWarning( "replace_line: Planes not equaly sized!" );
		return;
	}
	
	std::vector<SimplePixel> lines;
	for( unsigned iy=0; iy<get_height(); ++iy ){
		SimplePixel pix = { scan_line( iy )
			,	p.scan_line( iy )
			,	get_width()
			,	&multiply_pixel
			,	0
			};
		lines.push_back( pix );
	}
	
	QtConcurrent::blockingMap( lines, &do_pixel_line );
}

Plane* Plane::deconvolve_rl( double amount, unsigned iterations ) const{
	Plane* estimate = new Plane( *this ); //NOTE: some use just plain 0.5 as initial estimate
	if( !estimate )
		return nullptr;
	Plane* copy = new Plane( *this ); //TODO: make subtract/divide/etc take const input
	
	//Create point spread function
	//NOTE: It is symmetric, so we don't need a flipped one
	double *psf = gaussian_kernel( amount, amount );
	if( !psf ){
		delete estimate;
		return nullptr;
	}
	
	for( unsigned i=0; i<iterations; ++i ){
		Plane* est_psf = estimate->weighted_sum( psf, amount+1, amount+1 );
		est_psf->divide( *copy ); //This is observed / est_psf
		Plane* error_est = est_psf->weighted_sum( psf, amount+1, amount+1 );
		estimate->multiply( *est_psf );
		delete est_psf;
		delete error_est;
		//TODO: make better checking
	}
	
	delete copy;
	delete psf;
	return estimate;
}


