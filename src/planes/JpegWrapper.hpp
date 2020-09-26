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

#ifndef JPEG_WRAPPER_HPP
#define JPEG_WRAPPER_HPP

#include "gwenview/iodevicejpegsourcemanager.h"
#include <jpeglib.h>

#include "../Geometry.hpp"

namespace Overmix{

struct JpegComponent{
	jpeg_component_info *info;
	
	explicit JpegComponent( jpeg_component_info& info ) : info(&info) { }
	
	Size<JDIMENSION> size() const
		{ return { info->downsampled_width, info->downsampled_height }; }
	Size<JDIMENSION> sizePadded() const {
		Size<JDIMENSION> dctsize( info->h_samp_factor*DCTSIZE, info->v_samp_factor*DCTSIZE );
		return (size() + dctsize-1) / dctsize * dctsize;
	}
		
	Size<int> sampling() const
		{ return { info->h_samp_factor, info->v_samp_factor }; }
};

class JpegDecompress{
	public: //TODO:
		jpeg_decompress_struct cinfo;
		jpeg_error_mgr jerr;
		
	public:
		JpegDecompress(){
			jpeg_create_decompress( &cinfo );
			cinfo.err = jpeg_std_error( &jerr );
		}
		~JpegDecompress(){ jpeg_destroy_decompress( &cinfo ); }
		
		void setDevice( QIODevice& dev )
			{ Gwenview::IODeviceJpegSourceManager::setup( &cinfo, &dev ); }
		
		void readHeader( bool what=true )
			{ jpeg_read_header( &cinfo, what ); }
		
		int outComponents() const{ return cinfo.output_components; }
		int numComponents() const{ return cinfo.num_components; }
		auto operator[](int i) const{ return JpegComponent{ cinfo.comp_info[i] }; }
		
		Size<unsigned> imageSize() const
			{ return { cinfo.image_width, cinfo.image_height }; }
		
		Size<int> maxSampling() const{
			Size<int> out{ 1, 1 };
			for( int i=0; i<numComponents(); i++ )
				out = out.max( (*this)[i].sampling() );
			return out;
		}
		
		Size<unsigned> blockCount( int channel ) const
			{ return (imageSize() + DCTSIZE-1) / DCTSIZE * (*this)[channel].sampling() / maxSampling(); }
			
		Size<JDIMENSION> size() const{ return { cinfo.image_width, cinfo.image_height }; }
		
		Size<JDIMENSION> sizePadded() const{
			Size<JDIMENSION> mcu_size( cinfo.max_h_samp_factor*DCTSIZE, cinfo.max_v_samp_factor*DCTSIZE );
			return (size() + mcu_size-1) / mcu_size * mcu_size;
		}
};

}

#endif
