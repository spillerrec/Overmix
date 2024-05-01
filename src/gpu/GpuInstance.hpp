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
#include "GpuDevice.hpp"

#include <webgpu/webgpu.h>

#include <mutex>
#include <thread>
#include <memory>

class GpuInstance {
	private:
		static std::once_flag has_instance;
		static std::unique_ptr<GpuInstance> instance;
		
		GpuHandle<WGPUInstance,wgpuInstanceRelease> gpu_instance;
		GpuHandle<WGPUAdapter,wgpuAdapterRelease> adapter;
		std::unique_ptr<GpuDevice> device;

		GpuInstance();
		
		std::unique_ptr<GpuDevice> RequestDevice();

	public:
		static GpuInstance& GetInstance();
		static GpuDevice& GetDevice();
};