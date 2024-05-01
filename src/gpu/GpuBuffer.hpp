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
#include "GpuQueue.hpp"

#include <span>
#include <memory>

class MapAsyncWaiter{
	private:
		bool done = false;
		WGPUBufferMapAsyncStatus status;
		
	public:
		void SetDone(WGPUBufferMapAsyncStatus status) {
			this->status = status;
			done = true;
		}
		
		WGPUBufferMapAsyncStatus Wait(class GpuDevice& instance);
};

template<typename BaseType>
class GpuBuffer{
private:
	GpuHandle<WGPUBuffer, wgpuBufferDestroy> buffer;
	uint32_t size;
	
public:
	GpuBuffer() = default;
	GpuBuffer(WGPUBuffer handle, uint32_t size) : buffer(handle), size(size) { }
		
	void Write(GpuQueue& queue, std::span<const BaseType> data){
		wgpuQueueWriteBuffer(queue.Get(), buffer, 0, data.data(), data.size()*sizeof(BaseType));
	}
	
	auto Get() { return buffer.Get(); }
	auto Size() const { return size; }
	
	auto MapAsync(){
		auto waiter = std::make_shared<MapAsyncWaiter>();
		auto onBuffer2Mapped = [](WGPUBufferMapAsyncStatus status, void* userData) {
			auto waiter = static_cast<MapAsyncWaiter*>(userData);
			waiter->SetDone(status);
		};
		
		wgpuBufferMapAsync(buffer, WGPUMapMode_Read, 0, Size(), onBuffer2Mapped, waiter.get());
		
		return waiter;
	}
	
	std::span<const BaseType> GetMapped() {
		auto buf = wgpuBufferGetConstMappedRange(buffer, 0, size);
		return {static_cast<const BaseType*>(buf), size / sizeof(BaseType)};
	}
	
	void Unmap() { wgpuBufferUnmap(buffer); }
};