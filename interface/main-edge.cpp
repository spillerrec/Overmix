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


#define KOMPUTE_LOG_OVERRIDE
#define KP_LOG_DEBUG(...)
#define KP_LOG_INFO(...)
#define KP_LOG_ERROR(...)
#define KP_LOG_WARN(...)

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


extern kp::Manager* mgr;

class GpuPlane{
	Plane p;
	std::shared_ptr<kp::TensorT<float>> tensor;
	std::vector<float> tensor_data;
	bool onCpu = false;
	bool onGpu = false;

	void CopyToTensorData(const Plane& plane){
		//TODO: Check p and plane are compatible
		#pragma omp parallel for
		for( int y=0; y<(int)plane.get_height(); y++ )
			for( int x=0; x<(int)plane.get_width(); x++ )
				tensor_data[x + y*plane.get_width()] = (float)color::asDouble(plane[y][x]);
	}

public:
	GpuPlane(int width, int height) : p(width, height) { }
	GpuPlane(Size<unsigned> s) : p(s) { }
	GpuPlane(const Plane& input) : p(input) { onCpu = true; }

	unsigned GetWidth() const { return p.get_width(); }
	unsigned GetHeight() const { return p.get_height(); }
	Size<unsigned> GetSize() const { return {GetWidth(), GetHeight()}; }

	const Plane& GetCpuConst(){
		std::cout << (onCpu ? "On CPU, " : "no, ") << (onGpu ? "OnGpu\n" : "no\n");
		if(!onCpu && !onGpu){
			// start on CPU
			//TODO: Throw exception? You can't use a const plane that is uninitialized after all
			std::cout << "Alreadyu on CPU\n";
			onCpu = true;
			return p;
		}

		if(onGpu){
			//Move to CPU
			Timer t("Export from GPU");
			mgr->sequence()->record<kp::OpTensorSyncLocal>({tensor})->eval();
			
			float* v_out = tensor->data();
			#pragma omp parallel for
			for (int y=0; y<(int)p.get_height(); y++)
				for (int x=0; x<(int)p.get_width(); x++)
					p[y][x] = color::fromDouble( v_out[x + y*p.get_width()] );
			onCpu = true;
		}
		else
			std::cout << "Not on GPU\n";

		return p;
	}

	Plane& GetCpu(){
		GetCpuConst();
		onGpu = false;
		return p;
	}

	auto GetGpu(){
		if (!tensor){
			//Initialize Gpu data
			tensor_data.resize(p.get_width() * p.get_height());
			if (onCpu)
				CopyToTensorData(p);  //TODO: Get updating tensor afterwards to work
			tensor = mgr->tensor(tensor_data);
		}

		if( onCpu && !onGpu ){
			Timer t("Copy to GPU");
			CopyToTensorData(p);
			mgr->sequence()->record<kp::OpTensorSyncDevice>({tensor})->eval();
		}
		onGpu = true;
		onCpu = false;

		return tensor;
	}
};

class ShaderCache{
	private:
		struct CacheItem{
			std::string id;
			std::vector<uint32_t> data;
		};

		std::vector<CacheItem> cache;

		int indexOf(const std::string& name){
			for (int i=0; i<cache.size(); i++)
				if (cache[i].id == name)
					return i;
			return -1;
		}

	public:
		const std::vector<uint32_t>& Get(const std::string& name){
			auto path = "shaders/" + name + ".spv";

			int index = indexOf(path);
			if (index < 0){
				cache.push_back({path, compiledSource(path)});
				index = cache.size() - 1;
			}

			return cache[index].data;
		}
};

static ShaderCache shaderCache;

class GpuOperation{
	private:
		std::vector<std::shared_ptr<kp::Tensor>> textures;
		int tWidth = 16;
		int tHeight = 16;
		std::string name;
		std::vector<int32_t> specConsts;

	public:
		GpuOperation(std::string name) : name(name) {}

		template<class... GpuPlanes>
		GpuOperation& Textures(GpuPlanes&... planes){
			this->textures = {planes.GetGpu()...};
			return *this;
		}

		GpuOperation& SpecConsts(std::vector<int32_t> specConsts){
			this->specConsts = specConsts;
			return *this;
		}

		template<typename Params>
		void Run(int w, int h, Params params){
			kp::Workgroup workgroup({(unsigned int)(w+tWidth-1)/tWidth, (unsigned int)(h+tHeight-1)/tHeight, 1});
			auto algorithm = mgr->algorithm(textures,
										shaderCache.Get(name),
										workgroup,
										specConsts, std::vector<Params>({params}));
										
			Timer t("Gpu Run");
			auto sq = mgr->sequence();
			sq->record<kp::OpAlgoDispatch>(algorithm);
			sq->eval();
		}

};

GpuPlane LuminanceAdjust(GpuPlane& p, GpuPlane& ref, float offset=color::asDouble(80), float scale=3.9){
	Timer t("LuminanceAdjust_GPU");

	struct Params{
		int32_t width;
		int32_t height;
		int32_t stride;
		int32_t offset_x;
		int32_t offset_y;
		float offset;
		float scale;
	} params;
	params.width = params.stride = p.GetWidth();
	params.height = p.GetHeight();
	params.offset_x = params.offset_y = 0;
	params.offset = offset;
	params.scale = scale;

	GpuPlane t_out(p.GetSize());

	GpuOperation("luminanceAdjust").Textures(p, ref, t_out).Run(p.GetWidth(), p.GetHeight(), params);
	
	return t_out;
}

GpuPlane HotPixel(GpuPlane& p, float threshold){
	Timer t("HotPixel");

	struct Params{
		int32_t width;
		int32_t height;
		int32_t stride;
		int32_t offset_x;
		int32_t offset_y;
		float threshold;
	} params;
	params.width = params.stride = p.GetWidth();
	params.height = p.GetHeight();
	params.offset_x = params.offset_y = 0;
	params.threshold = threshold;

	GpuPlane t_out(p.GetSize());

	GpuOperation("hotpixel").Textures(p, t_out).Run(p.GetWidth(), p.GetHeight(), params);
	
	return t_out;
}

struct Mat3{
	float m00, m10, m20;
	float m01, m11, m21;
	float m02, m12, m22;

	Mat3 Transpose() const { return {m00, m01, m02, m10, m11, m12, m20, m21, m22}; }
};

std::array<GpuPlane,3> ColorMatrix(GpuPlane& r, GpuPlane& g, GpuPlane& b, Mat3 m){
	Timer t("ColorMatrix");

	struct Params{
		int32_t width;
		int32_t height;
		int32_t stride;
		int32_t offset_x;
		int32_t offset_y;
		float m00, m10, m20;
		float m01, m11, m21;
		float m02, m12, m22;
	} params;
	params.width = params.stride = r.GetWidth();
	params.height = r.GetHeight();
	params.offset_x = params.offset_y = 0;
	
	params.m00 = m.m00;
	params.m01 = m.m01;
	params.m02 = m.m02;
	params.m10 = m.m10;
	params.m11 = m.m11;
	params.m12 = m.m12;
	params.m20 = m.m20;
	params.m21 = m.m21;
	params.m22 = m.m22;

	GpuPlane t_out_r(r.GetSize());
	GpuPlane t_out_g(r.GetSize());
	GpuPlane t_out_b(r.GetSize());

	GpuOperation("colorMatrix").Textures(r, g, b, t_out_r, t_out_g, t_out_b).Run(r.GetWidth(), r.GetHeight(), params);
	
	return {t_out_r, t_out_g, t_out_b};
}

GpuPlane Sobel(GpuPlane& p){
	Timer t("Sobel_GPU");

	struct Params{
		int32_t width;
		int32_t height;
		int32_t stride;
		int32_t offset_x;
		int32_t offset_y;
	} params;
	params.width = params.stride = p.GetWidth();
	params.height = p.GetHeight();
	params.offset_x = params.offset_y = 0;

	GpuPlane t_out(p.GetSize());

	std::vector<int32_t> specConsts({ 1,1,1, 1,-8,1, 1,1,1 });
	GpuOperation("kernel3x3int").Textures(p, t_out).SpecConsts(specConsts).Run(p.GetWidth(), p.GetHeight(), params);
	
	return t_out;
}

GpuPlane Level(GpuPlane& p, float gamma=1.0, float in_low=0.0, float in_high=1.0, float out_low=0.0, float out_high=1.0){
	Timer t("Level_GPU");

	struct Params{
		int32_t width;
		int32_t height;
		int32_t stride;
		int32_t offset_x;
		int32_t offset_y;
		float in_low;
		float in_high;
		float out_low;
		float out_high;
		float gamma;
	} params;
	params.width = params.stride = p.GetWidth();
	params.height = p.GetHeight();
	params.offset_x = params.offset_y = 0;
	params.in_low = in_low;
	params.in_high = in_high;
	params.out_low = out_low;
	params.out_high = out_high;
	params.gamma = gamma;

	GpuPlane t_out(p.GetSize());

	GpuOperation("level").Textures(p, t_out).Run(p.GetWidth(), p.GetHeight(), params);
	
	return t_out;
}

std::array<GpuPlane,3> Saturation(GpuPlane& r, GpuPlane& g, GpuPlane& b, double amount){
	Timer t("ColorMatrix");

	struct Params{
		int32_t width;
		int32_t height;
		int32_t stride;
		int32_t offset_x;
		int32_t offset_y;
		float amount;
	} params;
	params.width = params.stride = r.GetWidth();
	params.height = r.GetHeight();
	params.offset_x = params.offset_y = 0;
	params.amount = amount;

	GpuPlane t_out_r(r.GetSize());
	GpuPlane t_out_g(r.GetSize());
	GpuPlane t_out_b(r.GetSize());

	GpuOperation("saturation").Textures(r, g, b, t_out_r, t_out_g, t_out_b).Run(r.GetWidth(), r.GetHeight(), params);
	
	return {t_out_r, t_out_g, t_out_b};
}

GpuPlane NLM3(GpuPlane& p, GpuPlane& ref, GpuPlane& ref1, GpuPlane& ref2, double stddev){
	Timer t("NLM3_GPU");

	struct Params{
		int32_t width;
		int32_t height;
		int32_t stride;
		int32_t offset_x;
		int32_t offset_y;
		float std_dev;
	} params;
	params.width = params.stride = p.GetWidth();
	params.height = p.GetHeight();
	params.offset_x = params.offset_y = 0;
	params.std_dev = stddev;

	GpuPlane t_out(p.GetSize());

	std::vector<int32_t> specConsts({ 1, 10 });
	GpuOperation("nlm").Textures(p, ref, ref1, ref2, t_out).SpecConsts(specConsts).Run(p.GetWidth(), p.GetHeight(), params);
	
	return t_out;
}
Plane NLM3(const Plane& p, const Plane& ref, const Plane& ref1, const Plane& ref2, double stddev){
	//return Plane(p);
	Timer t("NLM_GPU");
	
	GpuPlane t_edge(p), t_p1(ref), t_p2(ref1), t_p3(ref2);
	return Plane(NLM3(t_edge, t_p1, t_p2, t_p3, stddev).GetCpuConst());
}

ImageEx ConvertRaw(const ImageEx& input){
	GpuPlane p1(input[0]), p2(input[1]), p3(input[2]), p4(input[3]);
	auto hotpixel_thres = 0.15;
	auto p1hot = HotPixel(p1, hotpixel_thres);
	auto p2hot = HotPixel(p2, hotpixel_thres);
	auto p3hot = HotPixel(p3, hotpixel_thres);
	auto p4hot = HotPixel(p4, hotpixel_thres);
	
	//inputImg[1].mix(inputImg[2]);

	//p1s2.GetCpuConst().level(0, color::WHITE, color::BLACK, color::WHITE, 0.5).save_png("_edge_0.png");
	//p2s2.GetCpuConst().level(0, color::WHITE, color::BLACK, color::WHITE, 0.5).save_png("_edge_1.png");
	//p4s2.GetCpuConst().level(0, color::WHITE, color::BLACK, color::WHITE, 0.5).save_png("_edge_3.png");

	auto nr1 = NLM3(p1hot, p1hot, p2hot, p4hot, 0.00001);
	auto nr2 = NLM3(p2hot, p1hot, p2hot, p4hot, 0.00001);
	auto nr3 = NLM3(p3hot, p1hot, p2hot, p4hot, 0.00001);
	auto nr4 = NLM3(p4hot, p1hot, p2hot, p4hot, 0.00001);

	auto black_level = color::asDouble(color::from16bit(512));
	auto white_level = color::asDouble(color::from16bit(15360));

	auto res1 = Level(nr1, 1.0, black_level, white_level, 0., 2876 / 1024.0);//1.49502694606781);
	auto res2 = Level(nr2, 1.0, black_level, white_level, 0., 1.0);
	//auto res3 = Level(nr3, 1.0, black_level, 1., 0., 1.49502694606781);
	auto res4 = Level(nr4, 1.0, black_level, white_level, 0., 1724 / 1024.0);//2.8888301849365234);
//*


	auto [r1, g1, b1] = Saturation(res1, res2, res4, 1.5);

	ImageEx result({Transform::RGB, Transfer::LINEAR});
	result.addPlane(std::move(Plane(r1.GetCpuConst())));
	result.addPlane(std::move(Plane(g1.GetCpuConst())));
	result.addPlane(std::move(Plane(b1.GetCpuConst())));

	return result;
//*/
	Mat3 forward1_x1d = {0.549400, 0.296200, 0.118600,
	                0.306900, 0.656500, 0.036600,
					0.147400, 0.001300, 0.676400};
	Mat3 forward2_x1d = {0.542700, 0.264000, 0.157600,
	                0.331000, 0.620900, 0.048100,
					0.157100, 0.001700, 0.666300};
	Mat3 color_x1d   = {0.500200, -0.087800, 0.011100,
	                -0.485600, 1.192900, 0.333800,
					-0.118300, 0.204100, 0.702200};

	Mat3 forward1 = {0.540600, 0.300400, 0.123300,
	                0.280100, 0.683200, 0.036700,
					0.169200, 0.000400, 0.655500};
	Mat3 forward2 = {0.525100, 0.281600, 0.157600,
	                0.280700, 0.670900, 0.048400,
					0.138400, 0.004000, 0.682700};
	Mat3 color   = {0.597300, -0.169500, -0.041900,
	                -0.382600, 1.179700, 0.229300,
					-0.063900, 0.139800, 0.578900};
	Mat3 colorInv= {1.844927940911720516, 0.2615321331384660041, 0.029941548791761677124,
	0.58628320072769339621, 0.9725318956376324943, -0.34278165064643077841,
	0.062063402940969777487, -0.20599076818551225605, 1.8134982548422259353};

	
	Mat3 rgb2xyz = {0.5767309  , 0.1855540, 0.1881852, //From D65
					0.2973769,  0.6273491,  0.0752741,
 					0.0270343, 0.0706872 , 0.9911085};

//	Mat3 xyz2rgb = {2.4934969, -0.9313836, -0.4027108,
//-0.8294890,  1.7626641,  0.0236247,
// 0.0358458, -0.0761724 , 0.9568845};
//	Mat3 xyz2rgb = {3.1338561, -1.6168667, -0.4906146, //From D50
//					-0.9787684,  1.9161415,  0.0334540,
// 					0.0719453, -0.2289914 , 1.4052427};
	Mat3 xyz2rgb = {3.2404542, -1.5371385, -0.4985314, //From D65
					-0.9692660,  1.8760108,  0.0415560,
 					0.0556434, -0.2040259 , 1.0572252};
					 
//	Mat3 xyz2rgb = {2.0413690, -0.5649464, -0.3446944, //From aRGB
//					-0.9692660,  1.8760108,  0.0415560,
// 					0.0134474, -0.1183897 , 1.0154096};

	auto [x, y, z] = ColorMatrix(res1, res2, res4, forward2);
	//x.GetCpuConst().save_png("_x.png");
	auto [r, g, b] = ColorMatrix(x, y, z, xyz2rgb);


/*
	ImageEx result({Transform::RGB, Transfer::LINEAR});
	result.addPlane(std::move(Plane(r.GetCpuConst())));
	result.addPlane(std::move(Plane(g.GetCpuConst())));
	result.addPlane(std::move(Plane(b.GetCpuConst())));

	return result;
	//*/
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

			auto subtract_noise2 = [](auto& h, auto& p)
		{
			for( int y=0; y<(int)p.getSize().height(); y++ )
				for( int x=0; x<(int)p.getSize().width(); x++ )
				{
					auto j = h[y][x];
					auto& v = p[y][x];
					auto xx = std::sqrt(std::max((double)j - 80, 0.0)) * 3.9; //NOTE: Based on the results from the CSV dump and will change for different images. Also misses a linear offset?
					v = v - xx;
					//if( xx > 0.0 )
						//v = std::max(0.0, v - std::sqrt(xx)) * ((140 - std::sqrt(xx)) / 140);
				}
		};

	/*
	auto pdaf_full = images.image(0);
	Plane pdaf(pdaf_full.get_width(), pdaf_full.get_height()/210);

	for(int iy=0; iy<pdaf.get_height(); iy++)
		for(int ix=0; ix<pdaf.get_width(); ix++){
			pdaf[iy][ix] = pdaf_full[0][iy*210 + 147][ix];
		}

		
	Plane pdaf_g(pdaf.get_width()/2, pdaf.get_height());
	Plane pdaf_b(pdaf.get_width()/2, pdaf.get_height());
	
	for(int iy=0; iy<pdaf_g.get_height(); iy++)
		for(int ix=0; ix<pdaf_g.get_width(); ix++){
			pdaf_g[iy][ix] = pdaf[iy][ix*2+0];
			pdaf_b[iy][ix] = pdaf[iy][ix*2+1];
		}
		
	Plane pdaf_g2(pdaf_g.get_width()/3, pdaf_g.get_height());
	Plane pdaf_b2(pdaf_b.get_width()/3, pdaf_b.get_height());
	
	for(int iy=0; iy<pdaf_g2.get_height(); iy++)
		for(int ix=0; ix<pdaf_g2.get_width(); ix++){
			auto average = [&](auto& p){ return (p[iy][ix*3+0] + p[iy][ix*3+1] + p[iy][ix*3+2]) / 3;};
			pdaf_g2[iy][ix] = average(pdaf_g);
			pdaf_b2[iy][ix] = average(pdaf_b);
		}

		
	Plane pdaf_g_l(pdaf_g2.get_width()/2, pdaf_g2.get_height());
	Plane pdaf_b_l(pdaf_b2.get_width()/2, pdaf_b2.get_height());
	Plane pdaf_g_r(pdaf_g2.get_width()/2, pdaf_g2.get_height());
	Plane pdaf_b_r(pdaf_b2.get_width()/2, pdaf_b2.get_height());
	
	for(int iy=0; iy<pdaf_g_l.get_height(); iy++)
		for(int ix=0; ix<pdaf_g_l.get_width(); ix++){
			pdaf_g_l[iy][ix] = pdaf_g2[iy][ix*2+0];
			pdaf_b_l[iy][ix] = pdaf_b2[iy][ix*2+0];
			pdaf_g_r[iy][ix] = pdaf_g2[iy][ix*2+1];
			pdaf_b_r[iy][ix] = pdaf_b2[iy][ix*2+1];
		}

		
	Plane pdaf_l_diff(pdaf_g_l.get_width(), pdaf_g_l.get_height());
	Plane pdaf_r_diff(pdaf_g_l.get_width(), pdaf_g_l.get_height());
	for(int iy=0; iy<pdaf_r_diff.get_height(); iy++)
		for(int ix=0; ix<pdaf_r_diff.get_width(); ix++){
			pdaf_l_diff[iy][ix] = (pdaf_b_l[iy][ix] - pdaf_g_l[iy][ix]) + color::WHITE/2;
			pdaf_r_diff[iy][ix] = (pdaf_b_r[iy][ix] - pdaf_g_r[iy][ix]) + color::WHITE/2;
		}

	Plane pdaf_comb(pdaf_g_l.get_width(), pdaf_g_l.get_height());
	for(int iy=0; iy<pdaf_r_diff.get_height(); iy++)
		for(int ix=0; ix<pdaf_r_diff.get_width(); ix++){
			pdaf_comb[iy][ix] = (pdaf_b_l[iy][ix] + pdaf_b_r[iy][ix]) / 2;
		}

	pdaf.save_png("_pdaf.png");
	pdaf_g_l.save_png("_pdaf_g_l.png");
	pdaf_g_r.save_png("_pdaf_g_r.png");
	pdaf_b_l.save_png("_pdaf_b_l.png");
	pdaf_b_r.save_png("_pdaf_b_r.png");
	pdaf_l_diff.save_png("_pdaf_l_diff.png");
	pdaf_r_diff.save_png("_pdaf_r_diff.png");
	pdaf_comb.save_png("_pdaf_comb.png");

	return 0;
	//*/

	try{
		auto inputImg = images.image(0);

		ConvertRaw(inputImg).to_qimage(true).save("_converted.png");
		return 0;

		GpuPlane p1(inputImg[0]), p2(inputImg[1]), p3(inputImg[2]), p4(inputImg[3]);
		auto hotpixel_thres = 0.15;
		auto p1hot = HotPixel(p1, hotpixel_thres);
		auto p2hot = HotPixel(p2, hotpixel_thres);
		auto p3hot = HotPixel(p3, hotpixel_thres);
		auto p4hot = HotPixel(p4, hotpixel_thres);
		auto p1s = Sobel(p1hot);
		auto p2s = Sobel(p2hot);
		auto p3s = Sobel(p3hot);
		auto p4s = Sobel(p4hot);
		auto p1s2 = LuminanceAdjust(p1s, p1hot);
		auto p2s2 = LuminanceAdjust(p2s, p2hot);
		auto p3s2 = LuminanceAdjust(p3s, p3hot);
		auto p4s2 = LuminanceAdjust(p4s, p4hot);
		
		//inputImg[1].mix(inputImg[2]);

		//p1s2.GetCpuConst().level(0, color::WHITE, color::BLACK, color::WHITE, 0.5).save_png("_edge_0.png");
		//p2s2.GetCpuConst().level(0, color::WHITE, color::BLACK, color::WHITE, 0.5).save_png("_edge_1.png");
		//p4s2.GetCpuConst().level(0, color::WHITE, color::BLACK, color::WHITE, 0.5).save_png("_edge_3.png");

		auto result  = Plane(NLM3(p2s2, p1hot, p2hot, p4hot, 0.0001).GetCpuConst());
		auto result1 = Plane(NLM3(p1s2, p1hot, p2hot, p4hot, 0.0001).GetCpuConst());
		auto result2 = Plane(NLM3(p4s2, p1hot, p2hot, p4hot, 0.0001).GetCpuConst());
		result.save_png("_result1.png");
		result1.save_png("_result0.png");
		result2.save_png("_result3.png");
		result1.mix(result2);
		result.mix(result1);
		result.save_png("_result_mixed.png");
		return 0;

		FocusStackingRender fs_render(0.0001, 15);
		auto fs_result = fs_render.render(images);
		fs_result.to_qimage().save("fs_render.png");

		return 0;
	}
	catch(std::exception& e){
		std::cout << e.what();
		return -1;
	}
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
}
