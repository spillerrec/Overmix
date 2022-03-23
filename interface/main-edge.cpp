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
#include "containers/ImageContainerSaver.hpp"
#include "planes/ImageEx.hpp"
#include "planes/FourierPlane.hpp"
#include "renders/FocusStackingRender.hpp"
#include "color.hpp"
#include "debug.hpp"

#include <QCoreApplication>
#include <QStringList>
#include <QLoggingCategory>
#include <QImage>

#include <stdexcept>
#include <iostream>
#include <cmath>
#include <random>

#include <kompute/Core.hpp>
#include <kompute/Manager.hpp>
#include <kompute/Tensor.hpp>
#include <kompute/operations/OpAlgoDispatch.hpp>
#include <kompute/operations/OpTensorSyncDevice.hpp>
#include <kompute/operations/OpTensorSyncLocal.hpp>

using namespace Overmix;
kp::Manager *mgr;

	
constexpr double stddev = 0.01;


static std::vector<uint32_t>
compiledSource(
  const std::string& path)
{
    std::ifstream fileStream(path, std::ios::binary);
    std::vector<char> buffer;
    buffer.insert(buffer.begin(), std::istreambuf_iterator<char>(fileStream), {});
    return {(uint32_t*)buffer.data(), (uint32_t*)(buffer.data() + buffer.size())};
}
	
Plane NLM_GPU(const Plane& edge, const Plane& p1, const Plane& p2, const Plane& p3){
	Timer t("NLM_GPU");

	auto toTensor = [&](const Plane& p){
		std::vector<float> v;
		v.reserve(p.get_height() * (p.get_width()));
		for( int y=0; y<(int)p.get_height(); y++ )
			for( int x=0; x<(int)p.get_width(); x++ )
				v.push_back((float)color::asDouble(p[y][x]));
		return mgr->tensor(v);
	};
	
	auto t_edge = toTensor(edge);
	auto t_p1 = toTensor(p1);

	
	Plane denoised(edge.getSize());
	auto t_out = toTensor(denoised);

	kp::Workgroup workgroup({edge.get_width(), edge.get_height(), 1});
    std::vector<float> specConsts({ (float)(edge.get_width()), (float)(edge.get_height()), 0.f, 0.f });

    auto algorithm = mgr->algorithm({t_edge, t_p1, t_out},
                                   // See documentation shader section for compileSource
                                   compiledSource("comp.spv"),
                                   workgroup,
                                   specConsts);

    mgr->sequence()
        ->record<kp::OpTensorSyncDevice>({t_edge, t_p1})
        ->record<kp::OpAlgoDispatch>(algorithm) // Binds default push consts
        ->eval(); // Evaluates only last recorded operation
		
    auto sq = mgr->sequence();
    sq->evalAsync<kp::OpTensorSyncLocal>({t_out});


    sq->evalAwait();

	auto v_out = t_out->vector();
	
	for (int y=0; y<(int)denoised.get_height(); y++)
		for (int x=0; x<(int)denoised.get_width(); x++)
			denoised[y][x] = color::fromDouble( v_out[x + y*(denoised.get_width())] );

	return denoised;
}


Plane NLM(const Plane& edge, const Plane& p1, const Plane& p2, const Plane& p3){
	Timer t("NLM");
	Plane denoised(edge);
	
	int k = 1;
	int r = 10;
	#pragma omp parallel for
	for (int y=k; y<(int)edge.get_height()-k; y++){
		//std::cout << y << std::endl;
		for (int x=k; x<(int)edge.get_width()-k; x++){
			
			double sum = 0.0;
			//double sum_p = 0.0;
			double weight = 0.0;
			
			//		std::cout << x << "x" << y << "\n";
			for    (int ry=std::max(k,y-r)-y; ry<=std::min(r+y,(int)edge.get_height()-k-1)-y; ry++)
				for (int rx=std::max(k,x-r)-x; rx<=std::min(r+x,(int)edge.get_width() -k-1)-x; rx++){
					//std::cout << "d_ " << rx << "x" << ry << "\n";
					
					auto v = edge[y+ry][x+rx];
					
					auto w = 0.0;
					
					for (int dy=-k; dy<=k; dy++)
						for (int dx=-k; dx<=k; dx++){
							auto diff = ((int)p1[y+dy][x+dx] - p1[y+ry+dy][x+rx+dx]) / 65535.0;
							w += diff * diff;
							//diff = ((int)p2[y+dy][x+dx] - p2[y+ry+dy][x+rx+dx]) / 65535.0;
							//w += diff * diff;
							//diff = ((int)p3[y+dy][x+dx] - p3[y+ry+dy][x+rx+dx]) / 65535.0;
							//w += diff * diff;
						}
					
					//w = (w > 0.0) ? 1.0 / w : 1.0;
					w = std::exp(-w / (stddev / 16));
					sum += v * w;
					//sum_p += v_p * w;
					weight += w;
				}
			
			
			denoised[y][x] = std::max(0.0, (weight > 0.0) ? sum / weight : edge[y][x]);
		}
	}
	
	return denoised;
}

int main( int argc, char *argv[] ){
	QCoreApplication app( argc, argv );
	kp::Manager mgr_global;
	mgr = &mgr_global;
	
	auto args = app.arguments();
	args.removeFirst();
	//*
	
	Overmix::ImageContainer images;
	for(auto path : args){
		if( QFileInfo( path ).completeSuffix() == "xml.overmix" )
			ImageContainerSaver::load( images, path );
		else{
			ImageEx a;
			a.read_file( path );
			images.addImage(std::move(a));
		}
	}

	FocusStackingRender fs_render(0.01, 15);
	auto fs_result = fs_render.render(images);
	fs_result.to_qimage().save("fs_render.png");

	return 0;
//*/
	ImageEx a;
	a.read_file( args[0] );

	NLM_GPU(a[1], a[0], a[1], a[2]).save_png("gpu_test.png");
	return 0;
	
	a[1].edge_laplacian().save_png("edge_lap_before.png");
	
	std::random_device rd{};
	std::mt19937 gen{rd()};
	std::normal_distribution<> d{0.0,stddev};
	auto apply_normal_noise = [&](auto& value){
		auto noise = d(gen);
		value = color::truncate(value + noise * color::WHITE);
	};
	#pragma omp parallel for
	for (int y=0; y<(int)a.get_height(); y++)
		for (int x=0; x<(int)a.get_width(); x++){
			apply_normal_noise( a[0][y][x] );
			apply_normal_noise( a[1][y][x] );
			apply_normal_noise( a[2][y][x] );
		}
	a.to_qimage().save("edge_noise.png");
		
		
	a[1].edge_laplacian().save_png("edge_lap_after.png");
	
	auto denoised_a = NLM(a[1], a[0], a[1], a[2]);
	auto denoised_b = NLM(a[1].edge_laplacian(), a[0], a[1], a[2]);
	
	a[1].save_png("edge_nlm_a_ref.png");
	denoised_a.save_png("edge_nlm_a.png");
	denoised_b.save_png("edge_nlm_b.png");
	denoised_a.edge_laplacian().save_png("edge_nlm_a_lap.png");
	return 0;
	
	auto subtract = [](double v, double ref){
		auto xx = std::sqrt(std::max((double)ref - 80, 0.0)) * 3.9; //NOTE: Based on the results from the CSV dump and will change for different images. Also misses a linear offset?
		return v - xx;
		return std::max(0.0, v - xx);
	};
	
	auto subtract_noise = [&](const auto& ref, auto& p)
	{
		for( int y=0; y<(int)p.getSize().height(); y++ )
			for( int x=0; x<(int)p.getSize().width(); x++ )
			{
				auto& v = p[y][x];
				
				v = subtract(v, ref[y][x]);
				
			}
	};
	
	auto& p = a[1];
	auto& p1 = a[0];
	auto& p2 = a[2];
	auto& p3 = a[3];
	p.save_png("edge-input.png");
	
	auto edge = p.edge_laplacian();
	edge.save_png("edge-lap.png");
	
	subtract_noise(p, edge);
	edge.save_png("edge-subtracted.png");
	
	Plane denoised(p);
	
	int k = 2;
	int r = 10;
	#pragma omp parallel for
	for (int y=r+k; y<(int)p.get_height()-r-k; y++){
		//std::cout << y << std::endl;
		for (int x=r+k; x<(int)p.get_width()-r-k; x++){
			
			double sum = 0.0;
			//double sum_p = 0.0;
			double weight = 0.0;
			
			for (int ry=-r; ry<=r; ry++)
				for (int rx=-r; rx<=r; rx++){
					
					//auto v_p = p[y+ry][x+rx];
					//auto v = p[y+ry][x+rx];
					auto v = edge[y+ry][x+rx];
					
					auto w = 0.0;
					
					for (int dy=-k; dy<=k; dy++)
						for (int dx=-k; dx<=k; dx++){
							auto diff = ((int)p[y+dy][x+dx] - p[y+ry+dy][x+rx+dx]) / 65535.0;
							w += diff * diff;
							//*
							diff = ((int)p1[y+dy][x+dx] - p1[y+ry+dy][x+rx+dx]) / 65535.0;
							w += diff * diff;
							diff = ((int)p2[y+dy][x+dx] - p2[y+ry+dy][x+rx+dx]) / 65535.0;
							w += diff * diff;
							diff = ((int)p3[y+dy][x+dx] - p3[y+ry+dy][x+rx+dx]) / 65535.0;
							w += diff * diff;
							//*/
						}
					
					//w = (w > 0.0) ? 1.0 / w : 1.0;
					w = std::exp(-w / 0.0001);
					sum += v * w;
					//sum_p += v_p * w;
					weight += w;
				}
			
			
			//denoised[y][x] = (weight > 0.0) ? sum / weight : p[y][x];
			denoised[y][x] = std::max(0.0, (weight > 0.0) ? sum / weight : edge[y][x]);
			//auto v_p = (weight > 0.0) ? sum_p / weight : p[y][x];
			//denoised[y][x] = subtract(denoised[y][x], v_p);
			//denoised[y][x] = std::pow(denoised[y][x]/65535.0, 1.0/2.0) * 65535.0;
		}
	}
	//denoised.save_png("edge-input-denoised.png");
	denoised.save_png("edge-denoised2.png");
	
	//subtract_noise(p, denoised);
	//denoised.save_png("edge-denoised2-subtracted.png");
	
	return 0;
	
	edge = denoised.edge_laplacian();
	edge.save_png("edge-lap2.png");
	
	subtract_noise(denoised, edge);
	edge.save_png("edge-subtracted2.png");
		
	return 0;
}
