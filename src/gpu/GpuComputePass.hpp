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

class GpuComputePass{
private:
	GpuHandle<WGPUComputePassEncoder, wgpuComputePassEncoderRelease> pass;
	
public:
	GpuComputePass(WGPUComputePassEncoder pass) : pass(pass) { }
	
	void SetPipeline(WGPUComputePipeline computePipeline) {
		wgpuComputePassEncoderSetPipeline(pass, computePipeline);
	}
	
	void SetDispatchWorkgroups(uint32_t countX, uint32_t countY=1, uint32_t countZ=1) {
		wgpuComputePassEncoderDispatchWorkgroups(pass, countX, countY, countZ);
	}
	
	void SetBindGroup(WGPUBindGroup group, uint32_t groupIndex=0){
		wgpuComputePassEncoderSetBindGroup(pass, groupIndex, group, 0, nullptr);
	}
	
	void End() { wgpuComputePassEncoderEnd(pass); }
};
