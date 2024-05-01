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

#include <webgpu/webgpu.h>

#include <mutex>
#include <iostream>

class GpuQueue{
private:
	GpuHandle<WGPUQueue, wgpuQueueRelease> queue;
	
	public:
	GpuQueue(WGPUDevice device) : queue(wgpuDeviceGetQueue(device)){
	}
	
	void Submit(GpuCommandBuffer& buffer){
		auto buf = buffer.Get();
		wgpuQueueSubmit(queue, 1, &buf);
	}
	void Submit(GpuCommandBuffer&& buffer){
		auto buf = buffer.Get();
		wgpuQueueSubmit(queue, 1, &buf);
	}
	void Submit() { wgpuQueueSubmit(queue, 0, nullptr); }
	
	void Wait(){
		// This does not seem to work as expected. In the native guide, it is called before submit.
		// On MDN it is called after the submit: https://developer.mozilla.org/en-US/docs/Web/API/GPUQueue/onSubmittedWorkDone
		//    device.queue.submit([commandEncoder.finish()]);
		//   device.queue.onSubmittedWorkDone().then(() => {
		//     console.log("All submitted commands processed.");
		//   });
		// And this matches from what I understands from the spec:
		//    https://gpuweb.github.io/gpuweb/#dom-gpuqueue-onsubmittedworkdone
		//    "Returns a Promise that resolves once this queue finishes processing all the work submitted up to this moment."
		// But this only seemed to do something when initialized before the submit.
		// Luckily it does not seem to be needed
		struct WaitData{
			std::mutex done;
			WGPUQueueWorkDoneStatus status = WGPUQueueWorkDoneStatus_Success;
		} data;

		std::cout << "Locking queue: " << &data << std::endl;
		data.done.lock();
		auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void* pUserData) {
			auto data = static_cast<WaitData*>(pUserData);
			data->done.unlock();
			std::cout << "Unlocking queue: " << data << std::endl;
			data->status = status;
		};
		wgpuQueueOnSubmittedWorkDone(queue, onQueueWorkDone, &data);
		data.done.lock();
		//std::cout << "Queued work finished with status: " << status << std::endl;
	}
	
	auto Get() { return queue.Get(); }
};