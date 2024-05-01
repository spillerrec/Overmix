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


#include "GpuInstance.hpp"

#include <stdexcept>
#include <iostream>

std::once_flag GpuInstance::has_instance;
std::unique_ptr<GpuInstance> GpuInstance::instance;

template<typename ReturnType>
class CallbackWaiter {
	private:
		ReturnType result;
		std::mutex done;
		
	public:
		CallbackWaiter() { done.lock(); }
		
		void SetResult(ReturnType result) {
			this->result = result;
			done.unlock();
		}
		
		ReturnType GetResult() {
			done.lock();
			return result;
		}
};

static WGPUAdapter requestAdapter(WGPUInstance instance, WGPURequestAdapterOptions const * options) {
	 CallbackWaiter<WGPUAdapter> userData;

    auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const * message, void * pUserData) {
        auto& userData = *reinterpret_cast<CallbackWaiter<WGPUAdapter>*>(pUserData);
        if (status == WGPURequestAdapterStatus_Success) {
            userData.SetResult(adapter);
        }
		else {
			userData.SetResult(nullptr);
			std::cout << "Could not get WebGPU adapter: " << message << std::endl;
        }
    };

    // Call to the WebGPU request adapter procedure
    wgpuInstanceRequestAdapter(
        instance /* equivalent of navigator.gpu */,
        options,
        onAdapterRequestEnded,
        (void*)&userData
    );

	return userData.GetResult();
}


WGPUDevice requestDevice(WGPUAdapter adapter, WGPUDeviceDescriptor const * descriptor) {
	CallbackWaiter<WGPUDevice> userData;

    auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, char const * message, void * pUserData) {
        auto& userData = *reinterpret_cast<CallbackWaiter<WGPUDevice>*>(pUserData);
        if (status == WGPURequestDeviceStatus_Success) {
            userData.SetResult(device);
        } else {
            userData.SetResult(nullptr);
            std::cout << "Could not get WebGPU device: " << message << std::endl;
        }
    };

    wgpuAdapterRequestDevice(
        adapter,
        descriptor,
        onDeviceRequestEnded,
        (void*)&userData
    );

	return userData.GetResult();
}

GpuInstance::GpuInstance() {
	// Create instance
	WGPUInstanceDescriptor desc = {};
	desc.nextInChain = nullptr;
	gpu_instance = wgpuCreateInstance(&desc);
	if (!gpu_instance)
		throw std::runtime_error("Could not initialize WebGPU!");
	
	WGPURequestAdapterOptions adapterOpts = {};
	adapter = requestAdapter(gpu_instance, &adapterOpts);

	device = RequestDevice();
}


std::unique_ptr<GpuDevice> GpuInstance::RequestDevice(){
	WGPUDeviceDescriptor deviceDesc = {};
	
	WGPURequiredLimits requiredLimits = {};
	requiredLimits.limits.maxComputeWorkgroupSizeX = 32;
	requiredLimits.limits.maxComputeWorkgroupSizeY = 1;
	requiredLimits.limits.maxComputeWorkgroupSizeZ = 1;
	requiredLimits.limits.maxComputeInvocationsPerWorkgroup = 32;
	requiredLimits.limits.maxComputeWorkgroupsPerDimension = 2;
	
	requiredLimits.limits.maxStorageBuffersPerShaderStage = 8;
	requiredLimits.limits.maxStorageBufferBindingSize = 2048;
	
	WGPUSupportedLimits supportedLimits{};
	supportedLimits.nextInChain = nullptr;

	wgpuAdapterGetLimits(adapter, &supportedLimits);
	requiredLimits.limits.maxBindGroups = 8;
	requiredLimits.limits.maxBindingsPerBindGroup = 8;
	requiredLimits.limits.maxBufferSize = 1024 * 1024 * 1024;
	requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
	requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;

	
	
	deviceDesc.nextInChain = nullptr;
	deviceDesc.label = "My Device"; // anything works here, that's your call
	deviceDesc.requiredFeatureCount = 0; // we do not require any specific feature
	deviceDesc.requiredLimits = nullptr;// &requiredLimits; // we do not require any specific limit
	deviceDesc.defaultQueue.nextInChain = nullptr;
	deviceDesc.defaultQueue.label = "The default queue";
	
	auto device = std::make_unique<GpuDevice>(requestDevice(adapter, &deviceDesc));
	
	// This should probably be moved to the actual device so we can transfer the error message?
	auto onDeviceError = [](WGPUErrorType type, char const* message, void* /* pUserData */) {
		std::cout << "Uncaptured device error: type " << type;
		if (message) std::cout << " (" << message << ")";
			std::cout << std::endl;
	};
	wgpuDeviceSetUncapturedErrorCallback(device->Get(), onDeviceError, nullptr /* pUserData */);
	
	return device;
}


GpuInstance& GpuInstance::GetInstance() {
	std::call_once(GpuInstance::has_instance, [](){ GpuInstance::instance = std::unique_ptr<GpuInstance>(new GpuInstance()); });
	return *instance;
}

GpuDevice& GpuInstance::GetDevice() {
	std::call_once(GpuInstance::has_instance, [](){ GpuInstance::instance = std::unique_ptr<GpuInstance>(new GpuInstance()); });
	return *instance->device;
}

