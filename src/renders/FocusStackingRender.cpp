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


#include "FocusStackingRender.hpp"
#include "../debug.hpp"

#include "../containers/AContainer.hpp"
#include "../planes/ImageEx.hpp"
#include "../utils/AProcessWatcher.hpp"
#include "../utils/PlaneUtils.hpp"
#include "../planes/manipulators/Inpaint.hpp"

#include <QTime>
#include <QImage>
#include <algorithm>
#include <vector>
using namespace std;
using namespace Overmix;

ImageEx diff( const ImageEx& img1, const ImageEx& img2 )
{
	ImageEx out(img1);
	for (int i=0; i<(int)out.size(); i++)
		out[i].substract( img2[i] );
	return out;
}

Plane Maximum(const Plane& p, int size){
	return Plane(p);
	auto s = p.getSize();
	Plane out(s);
	
	
	for( int y=0; y<(int)s.height(); y++ )
		for( int x=0; x<(int)s.width(); x++ ){
			
			int xs = std::max(0, x - size);
			int ys = std::max(0, y - size);
			int xe = std::min(x + size, (int)s.width()-1);
			int ye = std::min(y + size, (int)s.height()-1);
			
			color_type val = color::MIN_VAL;
			for( int i=ys; i<=ye; i++ )
				for( int j=xs; j<=xe; j++ ){
					val = std::max(val, p[i][j]);
				}
			
			out[y][x] = val;
		}
	
	return out;
}


ImageEx FocusStackingRender::render( const AContainer& aligner, AProcessWatcher* watcher ) const{
	Timer t( "FocusStackingRender::render()" );
	
	auto makeError = [](const char* msg){
		qWarning( "%s", msg );
		return ImageEx();
	};
	
	//Abort if no images
	if( aligner.count() == 0 )
		return makeError( "No images to render!" );
	
	Progress progress( "FocusStackingRender", aligner.count() * 1, watcher );
	
	//Determine if we need to care about alpha per plane
// 	for( unsigned i=0; i<aligner.count(); ++i )
// 		if( aligner.alpha( i ) || aligner.imageMask( i ) >= 0 )
// 			return makeError( "Alpha not supported in FocusStacking" );
		
	//Check for movement in both direction
	auto movement = aligner.hasMovement();
	if( movement.first && movement.second )
		return makeError( "No movement support in FocusStacking yet" );
	
	auto out = ImageEx(aligner.image(0));
	
	std::vector<Plane> planes;
	for( unsigned i=0; i<aligner.count(); i++){
		if( progress.shouldCancel() )
			return {};
		
		auto inputImg = aligner.image(i);
		auto img = inputImg.copyApply( &Plane::edge_laplacian );
		img.to_grayscale();
		
		planes.push_back(img[0]);
		//planes.push_back( Maximum(inputImg[c]/*.blur_gaussian(blur_amount, blur_amount)*/.edge_laplacian(), 10) );
		//planes.push_back( inputImg[0].edge_guassian(1.5, 1.0, 1.0) );
		
		progress.add();
	}
	
	//for( unsigned i=0; i<planes.size(); i++ )
	//	planes[i].save_png("plane-" + std::to_string(i) + ".png");
	
	
	auto size = planes[0].getSize();
	Plane selected( size );
	Plane weight( size );
	Plane max_diff( size );
	Plane stdev( size );
	Plane selected_empty( size );
	selected_empty.fill(color::WHITE);
	
	std::vector<color_type> median_arr( aligner.count() );
	for( int y=0; y<(int)size.height(); y++ )
		for( int x=0; x<(int)size.width(); x++ ){
			int best = 0;
			int id = 0;
			int sum = 0;
			for( int i=0; i<(int)aligner.count(); i++ ){
				auto val = planes[i][y][x];
				median_arr[i] = val;
				sum += val;
				if( val > best ){
					best = val;
					id = i;
				}
			}
			int avg = sum / aligner.count();
			std::sort(median_arr.begin(), median_arr.end());
			auto median = median_arr[aligner.count()/2];
			
			
			max_diff[y][x] = best - avg;
			max_diff[y][x] = color::fromDouble((best / (double)median - 1.0) / 100.0);//color::fromDouble(std::min(1.0, best / (double)median / 50));
		//	if ((best - avg) <= color::from8bit(1))
		//	{
		//		id = 0;
		//		best = 0;
		//	}
			
			selected[y][x] = id;
			weight[y][x] = best;
		}
		
		
	int k2 = 2;
	for(int y=k2; y<(int)size.height()-k2; y++)
		for (int x=k2; x<(int)size.width()-k2; x++) {
			int sum = 0;
			for( int i=-k2; i<=k2; i++ )
				for( int j=-k2; j<=k2; j++ ){
					sum += selected[y+i][x+j];
				}
				
			int count = (k2*2+1) * (k2*2+1);
			double avg = sum / (double)count;
			double diff = 0.0;
			for( int i=-k2; i<=k2; i++ )
				for( int j=-k2; j<=k2; j++ ){
					diff += std::abs(avg - selected[y+i][x+j]);
				}
			stdev[y][x] = diff / count;
		}
	
	double stdev_max = stdev.max_value();
	for( int y=0; y<(int)size.height(); y++ )
		for( int x=0; x<(int)size.width(); x++ ){
			//weight[y][x] *= (stdev[y][x] < stdev_max/4) ? 1 : 0;
			
			//weight[y][x] *= (1.0 - stdev[y][x] / stdev_max);
		}
	
		
	Plane selected_filter( selected );
	int k = kernel_size;
	for(int y=k; y<(int)size.height()-k; y++)
		for (int x=k; x<(int)size.width()-k; x++)
		{
			int best = 0;
			int id = 0;
			int px=0, py=0;
			for( int i=-k; i<=k; i++ )
				for( int j=-k; j<=k; j++ ){
					auto val = weight[y+i][x+j];// * (int)planes[selected[y+i][x+j]][y][x];
					if( val > best ){
						best = val;
						id = selected[y+i][x+j];
						px = x+j;
						py = y+i;
					}
				}
			selected_filter[y][x] = id;
			selected_empty[py][px] = id;
		}
		
	/*
	k *= 1;
	std::vector<color_type> local_ids;
	local_ids.reserve(aligner.count());
	for(int y=k; y<(int)size.height()-k; y++)
		for (int x=k; x<(int)size.width()-k; x++)
		{
			local_ids.clear();
			for( int i=-k; i<=k; i++ )
				for( int j=-k; j<=k; j++ ){
					auto val = selected_empty[y+i][x+j];
					if( val != color::WHITE )
						local_ids.push_back(val);
				}
			std::sort(local_ids.begin(), local_ids.end());
			auto new_end = std::unique(local_ids.begin(), local_ids.end());
			local_ids.resize( std::distance(local_ids.begin(), new_end) );
			
			int best = 0;
			int best_id = 0;
			for( auto id : local_ids )
			{
				auto val = planes[id][y][x];
				if( val >= best )
				{
					best = val;
					best_id = id;
				}
			}
			selected_filter[y][x] = best_id;
		}
	//*/
	
	auto autoLevel = [](const Plane& p){
		return p.level(0, p.max_value(), color::WHITE, color::BLACK, 1.0);
	};
	
	auto blurred = selected_filter.blur_box(k*5, k*5);
	Plane suspicious( size );
	for( int y=0; y<(int)size.height(); y++ )
		for ( int x=0; x<(int)size.width(); x++ )
			suspicious[y][x] = std::abs(selected_filter[y][x] - (int)blurred[y][x]);
	autoLevel(suspicious)     .save_png("suspicious.png");
	
	suspicious.binarize_threshold( suspicious.max_value()*0.1 );
	autoLevel(suspicious)     .save_png("suspicious-binarized.png");
	suspicious = suspicious.level(color::BLACK, color::WHITE, color::WHITE, color::BLACK, 1.0);
	
	auto filled = Inpaint::simple(selected_filter, suspicious);
	
	
	auto blurred2 = filled.blur_box(k*5, k*5);
	Plane suspicious2( size );
	for( int y=0; y<(int)size.height(); y++ )
		for ( int x=0; x<(int)size.width(); x++ )
			suspicious2[y][x] = std::abs(filled[y][x] - (int)blurred2[y][x]);
	autoLevel(suspicious2)     .save_png("suspicious2.png");
	
	suspicious2.binarize_threshold( suspicious2.max_value()*0.1 );
	autoLevel(suspicious2)     .save_png("suspicious-binarized2.png");
	suspicious2 = suspicious2.level(color::BLACK, color::WHITE, color::WHITE, color::BLACK, 1.0);
	
	auto filled2 = Inpaint::simple(filled, suspicious2);
	
	
	for( int y=0; y<(int)size.height(); y++ )
		for ( int x=0; x<(int)size.width(); x++ ){
			auto id = filled2[y][x];
			auto& img = aligner.image(id);
			for( int c=0; c<(int)img.size(); c++ )
				out[c][y][x] = img[c][y][x];
		}
		
	std::string extra = ".png";//" - " + std::to_string(c) + ".png";
 	autoLevel(selected)       .save_png("selected" + extra);
 	selected_empty .save_png("selected_empty" + extra);
 	autoLevel(selected_filter).save_png("selected_filter" + extra);
 	autoLevel(filled).save_png("filled" + extra);
 	autoLevel(filled2).save_png("filled2" + extra);
 	filled2.save_png("filled2-raw" + extra);
 	weight         .save_png("weight" + extra);
 	max_diff       .save_png("max_diff" + extra);
 	stdev          .save_png("stdev" + extra);
	//} // c loop end
	return out;
}



