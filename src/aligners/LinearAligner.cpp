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


#include "LinearAligner.hpp"

using namespace Overmix;

struct LinearFunc{
	double x1 = 0.0, x2 = 0.0, xy = 0.0, y1 = 0.0;
	unsigned n = 0;
	
	void add( double x, double y ){
		x1 += x;
		x2 += x*x;
		xy += x*y;
		y1 += y;
		n++;
	}
	
	double dev() const{ return n * x2 - x1 * x1; }
	double a() const{ return 1.0 / dev() * (  n*xy - x1*y1 ); }
	double b() const{ return 1.0 / dev() * ( x2*y1 - x1*xy ); }
	
	double operator()( double x ) const{ return a() * x + b(); }
};

void LinearAligner::align( AContainer& container, AProcessWatcher* watcher ) const {
	LinearFunc hor, ver;//, both;
	for( unsigned i=0; i<container.count(); i++ ){
		hor.add( i, container.pos(i).x );
		ver.add( i, container.pos(i).y );
		//both.add( pos(i).x, pos(i).y );
	}
	
	for( unsigned i=0; i<container.count(); i++ ){
		switch( method ){
			case AlignMethod::ALIGN_BOTH: container.setPos( i, { hor(i), ver(i) } ); break;
			case AlignMethod::ALIGN_VER:  container.setPos( i, { 0, ver(i) } ); break;
			case AlignMethod::ALIGN_HOR:  container.setPos( i, { hor(i), 0 } ); break;
		};
	}
}

