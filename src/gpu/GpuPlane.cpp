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


#include "GpuPlane.hpp"

#include "../color.hpp"

#include <cstring>


GpuPlane::GpuPlane(const Overmix::Plane& in, GpuDevice& device, GpuQueue& queue) {
	//TODO: Move the int to float convertion to the GPU. This might require us to slightly overallocate in PlaneBase
	auto p = in.mapParallel([](auto val){ return (float)Overmix::color::asDouble(val); } ); // u16/i16 requires an extension
	size = p.getSize();
	image_width = p.get_line_width();
	auto amount = p.get_height() * p.get_line_width();
	buffer = device.CreateBuffer<float>(amount, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
	buffer.Write(queue, std::span<const float>(p.allPixelsBegin(), amount));
}

Overmix::Plane GpuPlane::ToCpu(GpuDevice& device) {
	int intWidth = (size.width() + 1) / 2;
	auto readBuf = device.CreateBuffer<uint32_t>(intWidth * size.height(), WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst);
	auto convertBuf = device.CreateBuffer<uint32_t>(intWidth * size.height(), WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);

	auto shader = R"SHADER(
		struct PlaneInfo {
			size: vec2u,
			stride: u32,
			offset: u32
		}
		@group(0) @binding(0) var<storage,read> inputBuffer: array<f32>;
		@group(0) @binding(1) var<uniform> bufferInfo: PlaneInfo;
		@group(0) @binding(2) var<storage,read_write> outputBuffer: array<u32>;
		@compute @workgroup_size(16, 16)
		fn convertToInt(@builtin(global_invocation_id) id: vec3<u32>) {
			if (id.x >= bufferInfo.size.x || id.y >= bufferInfo.size.y)
			{
				return;
			}
			var v0 = inputBuffer[id.x * 2 + 0 + id.y * bufferInfo.stride];
			var v1 = inputBuffer[id.x * 2 + 1 + id.y * bufferInfo.stride];

			var maxVal = 65536/4 - 1.0;
			var res0 = u32(clamp(v0, 0.0, 1.0) * maxVal);
			var res1 = u32(clamp(v1, 0.0, 1.0) * maxVal);
			var res = res1 * 65536 + res0;
			
			var outputIdx = id.x + id.y * u32((bufferInfo.size.x + 1) / 2);
			outputBuffer[outputIdx] = res;
		}
	)SHADER";

	auto bindLayout = device.CreateBindLayout(
		WGPUBufferBindingType_ReadOnlyStorage, 
		WGPUBufferBindingType_Uniform, 
		WGPUBufferBindingType_Storage);
	auto computePipeline = device.MakePipeline("convertToInt", shader, bindLayout);


	auto queue = device.MakeQueue();
	auto infoBuf = device.CreateStruct(GetInfoStruct(), queue);

	auto encoder = device.MakeEncoder();
	auto computePass = encoder.MakeComputePass();

	auto bindGroup = device.CreateBindGroup(bindLayout, buffer, infoBuf, convertBuf);
	computePass.SetPipeline(computePipeline);
	computePass.SetBindGroup(bindGroup);
	computePass.SetDispatchWorkgroups((intWidth + 15)/16, (size.height() + 15)/16);

	computePass.End();
	
	encoder.CopyBufferToBuffer(convertBuf, readBuf);
	
	queue.Submit(encoder.Finish());

	auto waiter = readBuf.MapAsync();
	
	auto status = waiter->Wait(device);
	if (status != WGPUBufferMapAsyncStatus_Success)
		throw std::runtime_error("GpuPlane::ToCpu wait failed with code: " + std::to_string(status));
	
	Overmix::Plane out(intWidth*2, size.height());
	auto map = readBuf.GetMapped();
	std::memcpy(out[0].begin(), map.data(), size.y * intWidth * sizeof(uint32_t));
	out.crop({0,0}, size);
	
	readBuf.Unmap();

	return out;
}