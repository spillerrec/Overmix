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

#include "ImageEx.hpp"

#include "JpegWrapper.hpp"
#include "../color.hpp"
#include "../debug.hpp"

#include "../degraders/JpegDegrader.hpp"

#include <QFile>

#include <vector>

using namespace Overmix;


JpegDegrader ImageEx::getJpegDegrader( QString path ){
	JpegDegrader deg;
	
	QFile f( path );
	if( !f.open( QIODevice::ReadOnly ) )
		return deg;
	
	ImageEx().from_jpeg( f, &deg );
	return deg;
}


using namespace std;
class RawReader{
	private:
		JpegDecompress& jpeg;
		ImageEx& img;
		
		vector<PlaneBase<uint8_t>> plane_buf;
		vector<vector<uint8_t*>> row_bufs;
		vector<uint8_t**> buf_access;
		
		
	public:
		RawReader( JpegDecompress& jpeg, ImageEx& img )
			: jpeg(jpeg), img(img){
				//Create buffers
				for( int i=0; i<jpeg.outComponents(); i++ )
					plane_buf.emplace_back( jpeg[i].sizePadded() );
				
				//Create row accesses
				for( int i=0; i<jpeg.outComponents(); i++ ){
					row_bufs.emplace_back( plane_buf[i].get_height(), nullptr );
					for( unsigned iy=0; iy<row_bufs[i].size(); iy++ )
						row_bufs[i][iy] = plane_buf[i].scan_line( iy ).begin();
				}
				
				//Apply "fake" crop
				for( int i=0; i<jpeg.outComponents(); i++ )
					plane_buf[i].crop( {0,0}, jpeg[i].size() );
			}
		
		///Prepare buffer for copy to lines starting from iy
		void prepare_buffer( int iy ){
			buf_access.clear();
			for( unsigned c=0; c<plane_buf.size(); c++ ){
				auto local_y = iy * jpeg[c].sampling().y / jpeg.cinfo.max_v_samp_factor;
				buf_access.push_back( row_bufs[c].data() + local_y );
			}
		}
		
		//Read a set of lines
		void readLine(){
			auto maxSize = jpeg.sizePadded();
			
			prepare_buffer( jpeg.cinfo.output_scanline );
			auto remaining = maxSize.height() - jpeg.cinfo.output_scanline;
 			assert( remaining >= jpeg.cinfo.max_v_samp_factor*DCTSIZE );
			jpeg_read_raw_data( &jpeg.cinfo, buf_access.data(), remaining );
			//TODO: chroma is offset of some reason...
		}
		
		void readAll(){
			while( jpeg.cinfo.output_scanline < jpeg.cinfo.output_height )
				readLine();
			
			for( auto& p : plane_buf )
				img.addPlane( convert( p ) );
		}
		
		Plane convert( const PlaneBase<uint8_t>& p ) const{
			//TODO: make more efficient?
			auto out = Plane(p.to<color_type>());
			for( unsigned iy=0; iy<out.get_height(); iy++ )
				for( unsigned ix=0; ix<out.get_width(); ix++ )
					out.setPixel( {ix,iy}, color::from8bit(out.pixel( {ix,iy} )) );
			return out;
		}
		
};

bool ImageEx::from_jpeg( QIODevice& dev, JpegDegrader* deg ){
	Timer t( "from_jpeg" );
	JpegDecompress jpeg;
	jpeg.setDevice( dev );
	jpeg.readHeader();
	
	jpeg.cinfo.raw_data_out = true;
	jpeg_start_decompress( &jpeg.cinfo );
	
	//TODO: use the correct color space info
	switch( jpeg.cinfo.out_color_components ){ //jpeg_color_space, JCS_YCbCr GRAYSCALE, RGB
		case 1: color_space = { Transform::GRAY, Transfer::SRGB }; qCDebug(LogImageIO) << "Gray-scale JPEG"; break;
		case 3: color_space = { Transform::JPEG, Transfer::SRGB }; break;
		default: qCWarning(LogImageIO) << "Unknown components count:" << jpeg.cinfo.out_color_components;
	}
	
	RawReader reader( jpeg, *this );
	reader.readAll();
	
	if( deg ){
		*deg = JpegDegrader();
		//TODO: set color type
		
		for( int i=0; i<jpeg.cinfo.output_components; i++ )
			deg->addPlane( { QuantTable{jpeg.cinfo.comp_info[i].quant_table->quantval}
					,	jpeg.cinfo.max_h_samp_factor / double(jpeg.cinfo.comp_info[i].h_samp_factor)
					,	jpeg.cinfo.max_v_samp_factor / double(jpeg.cinfo.comp_info[i].v_samp_factor)
				} );
	}
	
	jpeg_finish_decompress( &jpeg.cinfo );
	
	return true;
}
