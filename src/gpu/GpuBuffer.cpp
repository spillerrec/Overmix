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

#include "GpuBuffer.hpp"
#include "GpuDevice.hpp"

#include <webgpu/webgpu.h>

#define WEBGPU_BACKEND_WGPU


WGPUBufferMapAsyncStatus MapAsyncWaiter::Wait(GpuDevice& device) {
	while (!done){
		
		#ifdef WEBGPU_BACKEND_WGPU
			 // Non-standardized behavior: submit empty queue to flush callbacks
			 // (wgpu-native also has a wgpuDevicePoll but its API is more complex)
			 auto queue = device.MakeQueue();
			 queue.Submit();
		#else
			 // Non-standard Dawn way
			 wgpuDeviceTick(device.Get());
		#endif
		
	}
	return status;
}