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

#pragma once

#include "GpuHandle.hpp"
#include "GpuCommandBuffer.hpp"
#include "GpuBuffer.hpp"
#include "GpuComputePass.hpp"

class GpuCommandEncoder {
private:
	GpuHandle<WGPUCommandEncoder,wgpuCommandEncoderRelease> encoder;
	
public:
	GpuCommandEncoder(WGPUDevice device){
		WGPUCommandEncoderDescriptor encoderDesc = {};
		encoderDesc.nextInChain = nullptr;
		encoderDesc.label = "My command encoder";
		encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);
	}
	
	void InsertDebugMarker(const char* const msg) { wgpuCommandEncoderInsertDebugMarker(encoder, msg); }
	
	GpuCommandBuffer Finish() {
		WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
		cmdBufferDescriptor.nextInChain = nullptr;
		cmdBufferDescriptor.label = "Command buffer";
		return wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
	}
	
	GpuComputePass MakeComputePass() {
		WGPUComputePassDescriptor computePassDesc;
		//computePassDesc.timestampWriteCount = 0;
		computePassDesc.timestampWrites = nullptr;
		return wgpuCommandEncoderBeginComputePass(encoder, &computePassDesc);
	}
	
	template<typename T>
	void CopyBufferToBuffer(GpuBuffer<T>& src, GpuBuffer<T>& dst){
		wgpuCommandEncoderCopyBufferToBuffer(encoder, src.Get(), 0, dst.Get(), 0, src.Size());
	}
};
