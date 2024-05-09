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

#include "rotation.hpp"

#include "../Plane.hpp"
#include "../PlaneExcept.hpp"
#include "interpolation.hpp"

#include "../../color.hpp"

#include "../../gpu/GpuInstance.hpp"
#include "../../gpu/GpuPlane.hpp"

using namespace std;
using namespace Overmix;

class TransformMatrix {
	public:
		double x0{1}, x1{0};
		double y0{0}, y1{1};
		
	public:
		TransformMatrix() = default;
		
		static TransformMatrix forward( double radians, Point<double> scale ) {
			TransformMatrix out;
			
			out.x0 =  std::cos(radians) * scale.x;
			out.x1 = -std::sin(radians) * scale.y;
			out.y0 =  std::sin(radians) * scale.x;
			out.y1 =  std::cos(radians) * scale.y;
			
			return out;
		}
		static TransformMatrix backwards( double radians, Point<double> scale ) {
			TransformMatrix out;
			radians = 3.14*2-radians;
			scale = Point<double>(1.0, 1.0)/scale;
			
			out.x0 =  std::cos(radians) * scale.x;
			out.x1 = -std::sin(radians) * scale.x;
			out.y0 =  std::sin(radians) * scale.y;
			out.y1 =  std::cos(radians) * scale.y;
			
			return out;
		}
		
		Point<double> operator()(Point<double> pos){
			return {
				pos.x * x0 + pos.y * x1,
				pos.x * y0 + pos.y * y1
			};
		}
};


Rectangle<int> Transformations::rotationEndSize( Size<unsigned> size, double radians, Point<double> scale ){
	auto forward = TransformMatrix::forward(radians, scale);
	auto p0 = Point<double>(0, 0);
	auto p1 = forward(Point<double>(0, size.y));
	auto p2 = forward(Point<double>(size.x, size.y));
	auto p3 = forward(Point<double>(size.x, 0));
	
	auto start = p0.min(p1).min(p2).min(p3).floor();
	auto end   = p0.max(p1).max(p2).max(p3).ceil();
	return Rectangle<int>( start, end - start );
}

Plane Transformations::rotation( const Plane& p, double radians, Point<double> scale ){
//	auto& device = GpuInstance::GetDevice();
//	GpuPlane in(p, device);
//	return rotation(in, radians, scale).ToCpu(device);

	auto area = rotationEndSize( p.getSize(), radians, scale );
	auto transform = TransformMatrix::backwards( radians, scale );
	
	Plane out(area.size);
	
	#pragma omp parallel for
	for( int iy=0; iy<out.get_height(); iy++ )
		for( int ix=0; ix<out.get_width(); ix++ ){
			auto pos = transform( Point<double>(ix, iy) + area.pos );
			out[iy][ix] = bilinear(p, pos);
		}
	
	return out;
}

Plane Transformations::rotationAlpha( const Plane& p, double radians, Point<double> scale ){
	auto area = rotationEndSize( p.getSize(), radians, scale );
	auto transform = TransformMatrix::backwards( radians, scale );
	
	Plane out(area.size);
	
	#pragma omp parallel for
	for( int iy=0; iy<out.get_height(); iy++ )
		for( int ix=0; ix<out.get_width(); ix++ ){
			auto pos = transform( Point<double>(ix, iy) + area.pos ).round();
			auto posClamped = pos.max({0,0}).min(p.getSize()-1);
			
			out[iy][ix] = (pos == posClamped) ? color::WHITE : color::BLACK;
		}
	
	return out;
}


GpuPlane Transformations::rotation( GpuPlane& in, double radians, Point<double> scale ) {
	auto area = rotationEndSize( in.GetSize(), radians, scale );
	auto transform = TransformMatrix::backwards( radians, scale );

	auto& device = GpuInstance::GetDevice();
	
	auto bindLayout = device.CreateBindLayout(
		WGPUBufferBindingType_ReadOnlyStorage, 
		WGPUBufferBindingType_Uniform, 
		WGPUBufferBindingType_Storage,
		WGPUBufferBindingType_Uniform,
		WGPUBufferBindingType_Uniform);
	
	struct RotationInfo {
		float x0, x1, y0, y1, xOffset, yOffset;
	};
	auto shader = R"SHADER(
		struct PlaneInfo {
			width: u32,
			height: u32,
			stride: u32,
		}
		struct RotationInfo {
			x0: f32,
			x1: f32,
			y0: f32,
			y1: f32,
			xOffset: f32,
			yOffset: f32
		}

		@group(0) @binding(0) var<storage,read> inputBuffer: array<f32>;
		@group(0) @binding(1) var<uniform> inputBufferInfo: PlaneInfo;
		@group(0) @binding(2) var<storage,read_write> outputBuffer: array<f32>;
		@group(0) @binding(3) var<uniform> outputBufferInfo: PlaneInfo;
		@group(0) @binding(4) var<uniform> rotInfo: RotationInfo;
		@compute @workgroup_size(16, 16)
		fn computeStuff(@builtin(global_invocation_id) id: vec3<u32>) {
			if (id.x >= outputBufferInfo.width || id.y >= outputBufferInfo.height)
			{
				return;
			}
			
			var outputIdx = id.x + id.y * outputBufferInfo.width;

			var p = vec2f(f32(id.x), f32(id.y)) + vec2f(rotInfo.xOffset, rotInfo.yOffset);
			var outPos = vec2f(
							p.x * rotInfo.x0 + p.y * rotInfo.x1,
							p.x * rotInfo.y0 + p.y * rotInfo.y1);
			outPos = clamp(outPos, vec2f(0.0,0.0), vec2f(f32(inputBufferInfo.width-1), f32(inputBufferInfo.height-1)));

			//TODO: Implement bilinear
			var inputIdx  = u32(outPos.x) + u32(outPos.y) * inputBufferInfo.width;
			outputBuffer[outputIdx] = inputBuffer[inputIdx];
		}
	)SHADER";
	auto computePipeline = device.MakePipeline("computeStuff", shader, bindLayout);

	
	auto queue = device.MakeQueue();
	RotationInfo rotInfo { (float)transform.x0, (float)transform.x1, (float)transform.y0, (float)transform.y1, (float)area.pos.x, (float)area.pos.y };

	GpuPlane out(area.size, device);
	auto rotInfoBuf = device.CreateStruct(rotInfo, queue);
	auto inInfoBuf = device.CreateStruct(in.GetInfoStruct(), queue);
	auto outInfoBuf = device.CreateStruct(out.GetInfoStruct(), queue);
	
	auto encoder = device.MakeEncoder();
	
	auto computePass = encoder.MakeComputePass();
	
	auto bindGroup = device.CreateBindGroup(bindLayout, in, inInfoBuf, out, outInfoBuf, rotInfoBuf);
	computePass.SetPipeline(computePipeline);
	computePass.SetBindGroup(bindGroup);
	computePass.SetDispatchWorkgroups((area.size.width() + 15)/16, (area.size.height() + 15)/16);

	computePass.End();
	
	queue.Submit(encoder.Finish());
	
	//queue.Wait();

	return out;
}

