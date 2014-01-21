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

#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <QImage>

namespace debug{
	
	void make_slide( QImage image, QString dir, double scale );
	
	void make_low_res( QImage image, QString dir, unsigned scale, unsigned amount=0 );
	
	void output_transfers_functions( QString path );
}

#endif