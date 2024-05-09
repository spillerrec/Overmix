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

#include "GpuDevice.hpp"
#include "GpuBuffer.hpp"

#include "../planes/Plane.hpp"

class GpuPlane {
	private:
		Overmix::Size<unsigned> size{ 0, 0 };
		unsigned image_width;
		GpuBuffer<float> buffer;

	public:
		GpuPlane(const Overmix::Plane& in, GpuDevice& device, GpuQueue& queue);

		GpuPlane(const Overmix::Plane& p, GpuDevice& device) {
			auto queue = device.MakeQueue();
			*this = GpuPlane(p, device, queue);
		}

		/// Create uninitialized plane on the GPU
		GpuPlane(Overmix::Size<unsigned> size, GpuDevice& device) {
			this->size = size;
			image_width = size.width();
			buffer = device.CreateBuffer<float>(size.width() * size.height(), WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);
		}

		auto& GetBuffer() { return buffer; }
		auto GetSize() const { return size; }

		Overmix::Plane ToCpu(GpuDevice& device);

		
		struct PlaneInfo {
			uint32_t w, h, s;
		};
		PlaneInfo GetInfoStruct() const { return {size.x, size.y, image_width}; }

};

inline WGPUBindGroupEntry BindGroupEntry(GpuPlane& plane){
	return BindGroupEntry(plane.GetBuffer());
}

