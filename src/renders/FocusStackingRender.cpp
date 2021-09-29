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


struct Layer{
	Point<int> size;
	int scale;
	Plane selected;
	Plane weight;
	Plane max_diff;
	Plane stdev;
	Plane selected_empty;
	Plane selected_filter;
	
	Layer( Point<int> size, int scale )
		:	size( size ), scale( scale )
		,	selected( size ), weight( size ), max_diff( size ), stdev( size ), selected_empty( size )
		{
			selected_empty.fill(color::WHITE);
		}
		
	std::string Extension() const {
		std::string extra = "-";
		if( scale < 10 )
			extra += "0";
		extra += std::to_string(scale) + ".png";
		return extra;
	}
		
	void FillSelected(const std::vector<Plane>& planes){
		selected.fill(0);
		int amount = planes.size();
		std::vector<color_type> median_arr( amount );
		for( int y=0; y<(int)size.height(); y++ )
			for( int x=0; x<(int)size.width(); x++ ){
				int best = 0;
				int id = 0;
				for( int i=0; i<(int)amount; i++ ){
					auto val = planes[i][y][x];
					if( val > best ){
						best = val;
						id = i;
					}
				}
				selected[y][x] = id;
				weight[y][x] = best;
			}
	}
	void FillMaxDiff(const std::vector<Plane>& planes){
		max_diff.fill(0);
		int amount = planes.size();
		std::vector<color_type> median_arr( amount );
		for( int y=0; y<(int)size.height(); y++ )
			for( int x=0; x<(int)size.width(); x++ ){
				int best = selected[y][x];
				int sum = 0;
				for( int i=0; i < amount; i++ ){
					auto val = planes[i][y][x];
					median_arr[i] = val;
					sum += val;
				}
				int avg = sum / amount;
				std::sort(median_arr.begin(), median_arr.end());
				auto median = median_arr[amount/2];
				
				
				max_diff[y][x] = best - avg;
				max_diff[y][x] = color::fromDouble((best / (double)median - 1.0) / 100.0);//color::fromDouble(std::min(1.0, best / (double)median / 50));
			}
	}
	
	void FillStDev( int r=2 ){		
		for(int y=r; y<(int)size.height()-r; y++)
			for (int x=r; x<(int)size.width()-r; x++) {
				int sum = 0;
				for( int i=-r; i<=r; i++ )
					for( int j=-r; j<=r; j++ ){
						sum += selected[y+i][x+j];
					}
					
				int count = (r*2+1) * (r*2+1);
				double avg = sum / (double)count;
				double diff = 0.0;
				for( int i=-r; i<=r; i++ )
					for( int j=-r; j<=r; j++ ){
						diff += std::abs(avg - selected[y+i][x+j]);
					}
				stdev[y][x] = diff / count;
			}
	}
	
	void FillSelectedFilter( int k ) {
		selected_filter = Plane( selected );
		
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
	}
};

#include <iostream>
#include <fstream>
ImageEx FocusStackingRender::render( const AContainer& aligner, AProcessWatcher* watcher ) const{
	Timer t( "FocusStackingRender::render()" );
	
	auto autoLevel = [](const Plane& p){
		return p.level(0, p.max_value(), color::WHITE, color::BLACK, 1.0);
	};
	
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
	std::vector<int64_t> sum0(65536, 0), count0(65536, 0);
	std::vector<int64_t> sum1(65536, 0), count1(65536, 0);
	std::vector<int64_t> sum2(65536, 0), count2(65536, 0);
	std::vector<int64_t> sum3(65536, 0), count3(65536, 0);
	for( unsigned i=0; i<aligner.count(); i++){
		if( progress.shouldCancel() )
			return {};
		
		auto subtract_noise = [](auto& h, auto& p, auto& sum, auto& count)
		{
			for( int y=0; y<p.getSize().height(); y++ )
				for( int x=0; x<p.getSize().width(); x++ )
				{
					auto j = h[y][x];
					auto& v = p[y][x];
					sum[j] += v;
					count[j] += 1;
					
					auto xx = (double)j * 8.9; //NOTE: Based on the results from the CSV dump and will change for different images. Also misses a linear offset?
					if( xx > 0.0 )
						v = std::max(0.0, v - std::sqrt(xx));
				}
		};
		
		
		auto inputImg = aligner.image(i);
		auto img = inputImg.copyApply( &Plane::edge_laplacian );
		//img[0].save_png("before0.png");
		//img[1].save_png("before1.png");
		//img[2].save_png("before2.png");
		//img[3].save_png("before3.png");
		subtract_noise( inputImg[0], img[0], sum0, count0 );
		subtract_noise( inputImg[1], img[1], sum1, count1 );
		subtract_noise( inputImg[2], img[2], sum2, count2 );
		subtract_noise( inputImg[3], img[3], sum3, count3 );
		//img[0].save_png("after0.png");
		//img[1].save_png("after1.png");
		//img[2].save_png("after2.png");
		//img[3].save_png("after3.png");
		//img.to_grayscale();
		//img = img.toRgb();
		//img.to_grayscale();
		img[0].mix(img[1]);
		img[2].mix(img[3]);
		img[0].mix(img[2]);
		
		planes.push_back(img[0]);
		//planes.push_back( Maximum(inputImg[c]/*.blur_gaussian(blur_amount, blur_amount)*/.edge_laplacian(), 10) );
		//planes.push_back( inputImg[0].edge_guassian(1.5, 1.0, 1.0) );
		
		progress.add();
	}
	
	std::ofstream myfile;
	myfile.open ("noise.csv");
	for(int i=0; i<65536; i++){
		myfile << sum0[i] << ", " << count0[i] << ", ";
		myfile << sum1[i] << ", " << count1[i] << ", ";
		myfile << sum2[i] << ", " << count2[i] << ", ";
		myfile << sum3[i] << ", " << count3[i] << ", ";
		myfile << "\n";
	}
	myfile.close();
	
	
	//for( unsigned i=0; i<planes.size(); i++ )
	//	planes[i].save_png("plane-" + std::to_string(i) + ".png");
	
	
	auto size = planes[0].getSize();
	
	std::vector<Layer> layers;
	
	for( int i=1; i<=64; i*=2 )
	{
		Layer l( planes[0].getSize(), i );
		l.FillSelected( planes );
		l.FillMaxDiff( planes );
		l.FillStDev( 2 );
		l.FillSelectedFilter( kernel_size );
		
		auto extra = l.Extension();
		autoLevel(l.selected       ).save_png("selected"        + extra);
		autoLevel(l.selected_filter).save_png("selected_filter" + extra);
		autoLevel(l.weight         ).save_png("weight"          + extra);
		autoLevel(l.max_diff       ).save_png("max_diff"        + extra);
		autoLevel(l.stdev          ).save_png("stdev"           + extra);
		
		layers.push_back( std::move(l) );
		
		// Downscale
		for( unsigned i=0; i<planes.size(); i++ )
			planes[i] = planes[i].scale_cubic( Plane(), planes[i].getSize() / 2 );
	}
	
	for( int i=layers.size()-2; i >= 0; i-- ) {
		auto& lower_scale = layers[i+1].selected;
		auto& current_scale = layers[i].selected;
		
		auto size = lower_scale.getSize() * 2;
		Plane ignore( size );
		ignore.fill( 0 );
		int k = 0;
		for(int y=k; y<(int)size.height()-k; y++)
			for (int x=k; x<(int)size.width()-k; x++)
			{
				int diff = 10000;
				for( int y2 = std::max(y/2-1, 0); y2 <= std::min(y/2+1, (int)lower_scale.get_height()-1); y2++ )
					for( int x2 = std::max(x/2-1, 0); x2 <= std::min(x/2+1, (int)lower_scale.get_width()-1); x2++ )
						diff = std::min(diff, std::abs(lower_scale[y2][x2] - (int)current_scale[y][x]));
				ignore[y][x] = (diff > 1) ? 1 : 0;
			}
			
		for(int y=k; y<(int)size.height()-k; y++)
			for (int x=k; x<(int)size.width()-k; x++)
			{
				if( ignore[y][x] > 0 )
					current_scale[y][x] = lower_scale[y/2][x/2];
			}
		
		auto extra = layers[i].Extension();
		autoLevel(current_scale).save_png("selected_updated" + extra);
		autoLevel(ignore       ).save_png("ignore"           + extra);
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
	
	auto& selected_filter = layers[0].selected;
	
	int k = kernel_size;
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
	
	double gMult = 2.4;
	double mults[4] = {1.5 * gMult, gMult, gMult, 2.75 * gMult};
	for( int y=0; y<(int)size.height(); y++ )
		for ( int x=0; x<(int)size.width(); x++ ){
			auto id = filled2[y][x];
			auto& img = aligner.image(id);
			for( int c=0; c<(int)img.size(); c++ )
				out[c][y][x] = color::truncateFullRange(img[c][y][x] * mults[c]);
		}
		
		//*
	std::string extra = ".png";//" - " + std::to_string(c) + ".png";
 	//autoLevel(selected)       .save_png("selected" + extra);
 	//selected_empty .save_png("selected_empty" + extra);
 	autoLevel(selected_filter).save_png("selected_filter" + extra);
 	autoLevel(filled).save_png("filled" + extra);
 	autoLevel(filled2).save_png("filled2" + extra);
 	filled2.save_png("filled2-raw" + extra);
 	//weight         .save_png("weight" + extra);
 	//max_diff       .save_png("max_diff" + extra);
 	//stdev          .save_png("stdev" + extra);
	//*/
	//} // c loop end
	return out;
}



