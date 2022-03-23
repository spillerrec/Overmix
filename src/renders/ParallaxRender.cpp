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


#include "ParallaxRender.hpp"
#include "AverageRender.hpp"
#include "StatisticsRender.hpp"
#include "../aligners/AverageAligner.hpp"
#include "../containers/AContainer.hpp"
#include "../containers/ImageContainer.hpp"
#include "../containers/DelegatedContainer.hpp"
#include "../color.hpp"
#include "../planes/ImageEx.hpp"

#include <set>
#include <vector>
#include <stdexcept>
#include <QImage>
using namespace std;
using namespace Overmix;


ImageEx ParallaxRender::render( const AContainer& aligner, AProcessWatcher* watcher ) const{
	//Find the smallest shared size
	auto size = aligner.size().size; //No image is larger than the final result
	for( unsigned i=0; i<aligner.count(); i++ )
		size = size.min( aligner.image(i).getSize() );
	
	
	auto render1 = StatisticsRender( Statistics::MEDIAN ).render( aligner );
	
	ImageContainer second_align;
	second_align.setComparator( aligner.getComparator() );
	
	int i=0;
	for( auto align : aligner ){
		auto& base = render1[0];
		auto& img = align.image()[0];
		auto d = align.pos();
		Plane res( img.getSize() );
		for( unsigned iy=0; iy<img.get_height(); iy++ )
			for( unsigned ix=0; ix<img.get_width(); ix++ ){
				auto dif = std::abs( base[iy+d.y][ix+d.x] - img[iy][ix] );
				res[iy][ix] = (dif > threshold*color::WHITE) ? color::WHITE : color::BLACK;
			}
		
		//res.save_png("diff_" + std::to_string(i) + ".png");
		
		ImageEx final(align.image());
		final.alpha_plane() = std::move(res);
		second_align.addImage(std::move(final));
		
		i++;
	}
	/*
	for( unsigned i=0; i<aligner.count(); i++ ){
		second_align.setPos( i, positions1.at(i) );
		
		//Invert mask
		auto& alpha = second_align.imageRef(i).alpha_plane();
		for( unsigned iy=0; iy<alpha.get_height(); iy++ )
			for( unsigned ix=0; ix<alpha.get_width(); ix++ )
				alpha[iy][ix] = color::WHITE - alpha[iy][ix];
	}
	*/
	AverageAligner().align(second_align);
	
	std::vector<Point<double>> positions1, positions2;
	for( unsigned i=0; i<aligner.count(); i++ ){
		positions1.push_back( aligner.pos(i) );
		positions2.push_back( second_align.pos(i) );
	}
	
	auto alignTo1 = [&](){ for( unsigned i=0; i<aligner.count(); i++ ) second_align.setPos( i, positions1.at(i) ); };
	auto alignTo2 = [&](){ for( unsigned i=0; i<aligner.count(); i++ ) second_align.setPos( i, positions2.at(i) ); };
	
	auto invertMasks = [&](){		
		for( unsigned i=0; i<aligner.count(); i++ ){
			//Invert mask
			auto& alpha = second_align.imageRef(i).alpha_plane();
			for( unsigned iy=0; iy<alpha.get_height(); iy++ )
				for( unsigned ix=0; ix<alpha.get_width(); ix++ )
					alpha[iy][ix] = color::WHITE - alpha[iy][ix];
		}
	};
	invertMasks();
	
	for (int it = 0; it<iteration_count; it++ ){
	//render1 = StatisticsRender( Statistics::MEDIAN ).render( aligner );
	//auto render2 = StatisticsRender( Statistics::MEDIAN ).render( second_align );
	alignTo1();
	render1 = AverageRender().render( second_align );
	render1.to_qimage().save(("render_" + std::to_string(it) + "_1.png").c_str());
	invertMasks();
	alignTo2();
	auto render2 = AverageRender().render( second_align );
	render2.to_qimage().save(("render_" + std::to_string(it) + "_2.png").c_str());
	
	
	Plane init( render2[0].getSize() );
	init.fill( color::WHITE/2 );
	
	for( unsigned i=0; i<aligner.count(); i++ ){
		auto& img = aligner.image(i)[0];
		auto d1 = aligner.pos(i);
		auto d2 = second_align.pos(i);
		for( unsigned iy=0; iy<img.get_height(); iy++ )
			for( unsigned ix=0; ix<img.get_width(); ix++ ){
				auto pix1 = render1[0][iy+d1.y][ix+d1.x];
				auto pix2 = render2[0][iy+d2.y][ix+d2.x];
				auto dif1 = std::abs( pix1 - img[iy][ix] );
				auto dif2 = std::abs( pix2 - img[iy][ix] );
				init[iy+d2.y][ix+d2.x] += (dif1 > dif2) ? 100 : -100;
			}
	}
	//init.binarize_threshold(color::WHITE/2);
	//init = init.dilate(iteration_count-it);
	init.save_png("init_" + std::to_string(it) + ".png");
	
	//*
	for( unsigned i=0; i<aligner.count(); i++ ){
		//second_align.setPos( i, positions1.at(i) );
		auto d2 = positions2.at(i);
		//Invert mask
		auto& alpha = second_align.imageRef(i).alpha_plane();
		for( unsigned iy=0; iy<alpha.get_height(); iy++ )
			for( unsigned ix=0; ix<alpha.get_width(); ix++ )
				//if (it == iteration_count-1 )
					alpha[iy][ix] = init[iy+d2.y][ix+d2.x] < color::WHITE/2 ? color::WHITE : color::BLACK;
				//else
				//	alpha[iy][ix] = init[iy+d2.y][ix+d2.x] > color::WHITE/2 ? color::WHITE : color::BLACK;
			
		//second_align.image(i).to_qimage().save(("masked_" + std::to_string(i) + ".png").c_str());
	}
	//*/
	
	}
	
	
	/*
	for( unsigned i=0; i<aligner.count(); i++ ){
		second_align.setPos( i, positions1.at(i) );
		auto d2 = positions2.at(i);
		//Invert mask
		auto& alpha = second_align.imageRef(i).alpha_plane();
		for( unsigned iy=0; iy<alpha.get_height(); iy++ )
			for( unsigned ix=0; ix<alpha.get_width(); ix++ )
				alpha[iy][ix] = init[iy+d2.y][ix+d2.x] < color::WHITE/2 ? color::WHITE : color::BLACK;
			
		second_align.image(i).to_qimage().save(("masked_" + std::to_string(i) + ".png").c_str());
	}
	//*/
	
	alignTo1();
	return AverageRender().render( second_align );
	
	//Create output image
	//return ImageEx{ std::move(init) };
}



