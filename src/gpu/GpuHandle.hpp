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

#include <webgpu/webgpu.h>

template<typename HandleType, void (*Deleter)(HandleType)>
class GpuHandle{
	private:
		HandleType handle = nullptr;
	public:
		GpuHandle() = default;
		GpuHandle(HandleType handle) : handle(handle) {
			//std::cout << "Creating " << typeid(HandleType).name() << " = " << handle << std::endl;
		}
		
		GpuHandle(const GpuHandle&) = delete;
		GpuHandle(GpuHandle&& other){
			handle = other.handle;
			other.handle = nullptr;
		}
		GpuHandle<HandleType, Deleter>& operator=(GpuHandle<HandleType, Deleter>& other) = delete;
		GpuHandle<HandleType, Deleter>& operator=(GpuHandle<HandleType, Deleter>&& other)
		{
			if (handle){
			//	std::cout << "Releasing on move" << typeid(HandleType).name() << " = " << handle << std::endl;
				Deleter(handle);
			}
			handle = other.handle;
			other.handle = nullptr;
			return *this;
		}
		
		
		~GpuHandle(){
			if (handle)
			{
			//	std::cout << "Releasing " << typeid(HandleType).name() << " = " << handle << std::endl;
				Deleter(handle);
			}
		}
		
		auto Get() { return handle; }
		
		operator HandleType() { return handle; }
};
