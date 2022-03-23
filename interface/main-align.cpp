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

#include "cli/CommandParser.hpp"
#include "containers/ImageContainer.hpp"
#include "planes/ImageEx.hpp"
#include "planes/FourierPlane.hpp"

#include <QCoreApplication>
#include <QStringList>
#include <QLoggingCategory>

#include <stdexcept>
#include <iostream>
#include <cmath>

using namespace Overmix;

int main( int argc, char *argv[] ){
	QCoreApplication app( argc, argv );
	
	auto args = app.arguments();
	args.removeFirst();
	
	ImageEx a, b;
	a.read_file( args[0] );
	b.read_file( args[1] );
	
	auto& p = a[0];
	Plane lp(p.getSize()*4);
	p.save_png("transform_before.png");
	
	for (int iy=0; iy<lp.get_height(); iy++)
		for (int ix=0; ix<lp.get_width(); ix++){
			auto ep = std::max(0.0, std::exp(((double)ix) / (lp.get_width()/10)));
			auto angle = (iy) / (double)lp.get_height() * 2 * 3.14 + 3.14;
			auto half_size = p.getSize() / 2.0;
			Point<double> wanted = { ep * std::cos(angle)+half_size.x, ep * std::sin(angle)+half_size.y };
			Point<int> pos = { std::clamp((int)wanted.x, 0, (int)p.get_width()-1), std::clamp((int)wanted.y, 0, (int)p.get_height()-1) };
			lp[iy][ix] = p[pos.y][pos.x];
		}
	
	lp.save_png("transform_after.png");
	
	auto p0 = FourierPlane(a[0]);
	auto p1 = FourierPlane(b[0]);
	
	p0.asPlane().save_png("alignment_a.png");
	p1.asPlane().save_png("alignment_b.png");
	
	FourierPlane res(p0);
	
	for( unsigned iy=0; iy<res.get_height(); iy++ )
		for( unsigned ix=0; ix<res.get_width(); ix++ ){
			auto a = p0[iy][ix];
			auto b = p1[iy][ix];
			b = { b.real(), b.imag() * -1};
			res[iy][ix] = (a * b) / std::abs(a * b);
		}
		
	std::cout << "Done" << std::endl;
	
	auto alignment = res.toPlane();
	
	Point<unsigned> pos;
	color_type maxVal = 0;
	for( unsigned iy=0; iy<alignment.get_height(); iy++ )
		for( unsigned ix=0; ix<alignment.get_width(); ix++ ){
			auto p = alignment[iy][ix];
			if( p > maxVal ){
				maxVal = p;
				pos = {ix, iy};
			}
		}
		
	std::cout << "Position: " << pos.x << "x" << pos.y << std::endl;
	pos = alignment.getSize() - pos;
	std::cout << "Position: " << pos.x << "x" << pos.y << std::endl;
	
	alignment.save_png("alignment_result.png");
	std::cout << "Done" << std::endl;
	
	return 0;
}
