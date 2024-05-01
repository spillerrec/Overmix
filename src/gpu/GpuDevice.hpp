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
#include "GpuCommandEncoder.hpp"
#include "GpuBuffer.hpp"

#include <webgpu/webgpu.h>

#include <vector>

class GpuDevice{
private:
	GpuHandle<WGPUDevice,wgpuDeviceRelease> device;

	GpuHandle<WGPUShaderModule,wgpuShaderModuleRelease> CreateShader(const char* const data);
		
public:
	GpuDevice( WGPUDevice device ) : device(device) { }
		
	WGPUDevice Get() { return device; }
	
	GpuQueue MakeQueue() { return GpuQueue(device); }
	
	GpuCommandEncoder MakeEncoder() { return GpuCommandEncoder(device); }
		
	GpuHandle<WGPUComputePipeline,wgpuComputePipelineRelease> MakePipeline(const char* const entryPoint, const char* const sourceCode, WGPUBindGroupLayout layout){
		auto computeShaderModule = CreateShader(sourceCode);

		// Create compute pipeline
		WGPUComputePipelineDescriptor computePipelineDesc = {};
		computePipelineDesc.compute.entryPoint = entryPoint;
		computePipelineDesc.compute.module = computeShaderModule;
		
		WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {};
		pipelineLayoutDesc.bindGroupLayoutCount = 1;
		pipelineLayoutDesc.bindGroupLayouts = &layout;
		auto m_pipelineLayout = GpuHandle<WGPUPipelineLayout, wgpuPipelineLayoutRelease>(wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDesc));
		computePipelineDesc.layout = m_pipelineLayout;
		
		return wgpuDeviceCreateComputePipeline(device, &computePipelineDesc);
	}
	
	template<typename... T>
	GpuHandle<WGPUBindGroupLayout, wgpuBindGroupLayoutRelease> CreateBindLayout(T&&... args){
		
		std::vector<WGPUBindGroupLayoutEntry> entries{BindGroupLayoutEntry(args)...};
		for (uint32_t i=0; i<entries.size(); i++)
			entries[i].binding = i;
		
		WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc;
		bindGroupLayoutDesc.label = nullptr;
		bindGroupLayoutDesc.nextInChain = nullptr;
		bindGroupLayoutDesc.entryCount = (uint32_t)entries.size();
		bindGroupLayoutDesc.entries = entries.data();
		return wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDesc);
	}
	
	
	template<typename... T>
	GpuHandle<WGPUBindGroup,wgpuBindGroupRelease> CreateBindGroup(WGPUBindGroupLayout bindLayout, T&&... args){

		std::vector<WGPUBindGroupEntry> entries{BindGroupEntry(args)...};
		for (uint32_t i=0; i<entries.size(); i++)
			entries[i].binding = i;
		
		WGPUBindGroupDescriptor bindGroupDesc;
		bindGroupDesc.layout = bindLayout;
		bindGroupDesc.entryCount = (uint32_t)entries.size();
		bindGroupDesc.entries = entries.data();
		return wgpuDeviceCreateBindGroup(device.Get(), &bindGroupDesc); //TODO:
	}
	
	
	template<typename T>
	GpuBuffer<T> CreateBuffer(uint32_t amount, WGPUBufferUsage usage, const char* const label=nullptr){
		uint32_t size = amount * sizeof(T);
		WGPUBufferDescriptor bufferDesc = {};
		bufferDesc.nextInChain = nullptr;
		bufferDesc.label = label;
		bufferDesc.usage = usage;
		bufferDesc.size = size;
		bufferDesc.mappedAtCreation = false;
		return GpuBuffer<T>{wgpuDeviceCreateBuffer(device, &bufferDesc), size};
	}

	template<typename T>
	GpuBuffer<T> CreateStruct(const T& data, GpuQueue& queue) {
		auto buf = CreateBuffer<T>(1, WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, typeid(data).name());
		buf.Write(queue, std::span<const T>(&data, 1));
		return buf;
	}
};


inline WGPUBufferUsage operator|(WGPUBufferUsage a, WGPUBufferUsage b)
{
    return static_cast<WGPUBufferUsage>(static_cast<int>(a) | static_cast<int>(b));
}

inline WGPUBindGroupLayoutEntry BindGroupLayoutEntry(WGPUBufferBindingType type){
	WGPUBindGroupLayoutEntry entry = {};
	entry.nextInChain = nullptr;
	entry.binding = 0;
	entry.buffer.nextInChain = nullptr;
	entry.buffer.type = type; // WGPUBufferBindingType_Storage
	entry.visibility = WGPUShaderStage_Compute;
	
	entry.buffer.hasDynamicOffset = false;
	entry.buffer.minBindingSize = 0; //4; TODO: What is this exactly

	entry.sampler.nextInChain = nullptr;
	entry.sampler.type = WGPUSamplerBindingType_Undefined;

	entry.storageTexture.nextInChain = nullptr;
	entry.storageTexture.access = WGPUStorageTextureAccess_Undefined;
	entry.storageTexture.format = WGPUTextureFormat_Undefined;
	entry.storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

	entry.texture.nextInChain = nullptr;
	entry.texture.multisampled = false;
	entry.texture.sampleType = WGPUTextureSampleType_Undefined;
	entry.texture.viewDimension = WGPUTextureViewDimension_Undefined;
	
	return entry;
}

template<typename T>
WGPUBindGroupEntry BindGroupEntry(GpuBuffer<T>& buffer){
	WGPUBindGroupEntry entry = {};
	entry.nextInChain = nullptr;
	entry.binding = 0;
	entry.buffer = buffer.Get();
	entry.offset = 0;
	entry.size = buffer.Size();
	entry.sampler = nullptr;
	return entry;
}
