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
#include "../color.hpp"

class GpuPlane {
	private:
		Overmix::Size<unsigned> size{ 0, 0 };
		unsigned image_width;
		GpuBuffer<float> buffer;

	public:
		GpuPlane(const Overmix::Plane& in, GpuDevice& device, GpuQueue& queue) {
			auto p = in.map([](auto val){ return (float)Overmix::color::asDouble(val); } ); // u16/i16 requires an extension
			size = p.getSize();
			image_width = p.get_line_width();
			auto amount = p.get_height() * p.get_line_width();
			buffer = device.CreateBuffer<float>(amount, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
			buffer.Write(queue, std::span<const float>(p.allPixelsBegin(), amount));
		}

		GpuPlane(const Overmix::Plane& p, GpuDevice& device) {
			auto queue = device.MakeQueue();
			*this = GpuPlane(p, device, queue);
			//queue.Submit();
		}

		
		GpuPlane(Overmix::Size<unsigned> size, GpuDevice& device) {
			this->size = size;
			image_width = size.width();
			buffer = device.CreateBuffer<float>(size.width() * size.height(), WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);
		}

		auto& GetBuffer() { return buffer; }
		auto GetSize() const { return size; }

		Overmix::Plane ToCpu(GpuDevice& device) {
			Overmix::Plane out(size);

			
			auto readBuf = device.CreateBuffer<float>(size.width() * size.height(), WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst);

			auto encoder = device.MakeEncoder();
			encoder.CopyBufferToBuffer(buffer, readBuf);
			
			auto queue = device.MakeQueue();
			queue.Submit(encoder.Finish());
			//queue.Wait();
	
			auto waiter = readBuf.MapAsync();
			
			auto status = waiter->Wait(device);
			if (status != WGPUBufferMapAsyncStatus_Success)
				throw std::runtime_error("GpuPlane::ToCpu wait failed with code: " + std::to_string(status));
			
			auto map = readBuf.GetMapped();
			for (unsigned iy=0; iy<size.height(); iy++)
				for (unsigned ix=0; ix<size.width(); ix++)
					out[iy][ix] = Overmix::color::fromDouble(map[ix + iy*size.width()]);
			
			readBuf.Unmap();

			return out;
		}

		
		struct PlaneInfo {
			uint32_t w, h, s;
		};
		PlaneInfo GetInfoStruct() const { return {size.x, size.y, image_width}; }

};

WGPUBindGroupEntry BindGroupEntry(GpuPlane& plane){
	return BindGroupEntry(plane.GetBuffer());
}

