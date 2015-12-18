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

#ifndef CONFUSION_MATRIX_HPP
#define CONFUSION_MATRIX_HPP

namespace Overmix{

struct ConfusionMatrix {
	unsigned tp{ 0 };
	unsigned fp{ 0 };
	unsigned tn{ 0 };
	unsigned fn{ 0 };
	
	ConfusionMatrix() {}
	
	ConfusionMatrix( unsigned tp, unsigned fp, unsigned tn, unsigned fn )
		: tp(tp), fp(fp), tn(tn), fn(fn) { }
	
	ConfusionMatrix( bool expected, bool actual ) {
		     if( expected==true  && actual == true  ) tp = 1;
		else if( expected==true  && actual == false ) fn = 1;
		else if( expected==false && actual == true  ) fp = 1;
		else if( expected==false && actual == false ) tn = 1;
	}
	
	//TODO: statistics
	
	ConfusionMatrix& operator+=( const ConfusionMatrix& other ) {
		tp += other.tp;
		fp += other.fp;
		tn += other.tn;
		fn += other.fn;
		return *this;
	}
};

}

#endif