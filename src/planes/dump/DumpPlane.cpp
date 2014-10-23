/*	This file is part of dump-tools.

	dump-tools is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	dump-tools is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with dump-tools.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "DumpPlane.hpp"

#include <zlib.h>
#include <lzma.h>

using namespace std;

bool DumpPlane::read( QIODevice &dev ){
	width  = read_32( dev );
	height = read_32( dev );
	depth  = read_16( dev );
	config = read_16( dev );
//	cout << "settings: " << width << "x" << height << "@" << depth << "p : " << config << endl;
	
	if( width == 0 || height == 0 || depth > 32 )
		return false;
	
	if( config & 0x1 ){
		uint32_t lenght = read_32( dev );
		if( lenght == 0 )
			return false;
		
		vector<char> buf( lenght );
		dev.read( buf.data(), lenght );
		
		uLongf uncompressed = size();
		data.resize( uncompressed );
		
		if( uncompress( (Bytef*)data.data(), &uncompressed, (Bytef*)buf.data(), lenght ) != Z_OK )
			return false;
	}
	else if( config & 0x2 ){
		//Initialize decoder
		lzma_stream strm = LZMA_STREAM_INIT;
		if( lzma_stream_decoder( &strm, UINT64_MAX, 0 ) != LZMA_OK )
			return false;
		
		//Read data
		uint32_t lenght = read_32( dev );
		if( lenght == 0 )
			return false;
		
		vector<char> buf( lenght );
		dev.read( buf.data(), lenght );
		
		data.resize( size() );
		
		//Decompress
		strm.next_in = (uint8_t*)buf.data();
		strm.avail_in = buf.size();
		
		strm.next_out = data.data();
		strm.avail_out = data.size();
		
		if( lzma_code( &strm, LZMA_FINISH ) != LZMA_STREAM_END ){
			cout << "Shit, didn't finish decompressing!" << endl;
			return false;
		}
		
		lzma_end(&strm);
	}
	else{
		data.resize( size() );
		return dev.read( (char*)data.data(), size() ) == size();
	}
	
	return true;
}

bool DumpPlane::write( QIODevice &dev, DumpPlane::Compression compression ){
	if( compression > LZMA )
		return false;
	
	write_32( dev, width );
	write_32( dev, height );
	write_16( dev, depth );
	write_16( dev, compression );
	
	if( compression == LZMA ){
		lzma_stream strm = LZMA_STREAM_INIT;
		if( lzma_easy_encoder( &strm, 9 | LZMA_PRESET_EXTREME, LZMA_CHECK_CRC64 ) != LZMA_OK )
			return false;
		
		strm.next_in = data.data();
		strm.avail_in = size();
		
		auto buf_size = strm.avail_in * 2;
		strm.avail_out = buf_size;
		vector<uint8_t> buf( buf_size );
		strm.next_out = buf.data();
		
		lzma_ret ret = lzma_code( &strm, LZMA_FINISH );
		if( ret != LZMA_STREAM_END ){
			cout << "Nooo, didn't finish compressing!" << endl;
			lzma_end( &strm ); //TODO: Use RAII
			return false;
		}
		
		auto final_size = buf_size - strm.avail_out;
		write_32( dev, final_size );
		dev.write( (char*)buf.data(), final_size );
		
		lzma_end( &strm );
		
		compression_ratio( final_size );
	}
	else if( compression == LZIP ){
		
		uLongf buf_size = compressBound( size() );
		vector<uint8_t> buf( buf_size );
		
		if( compress( (Bytef*)buf.data(), &buf_size, (Bytef*)data.data(), size() ) != Z_OK )
			return false;
		
		write_32( dev, buf_size );
		dev.write( (char*)buf.data(), buf_size );
		
		compression_ratio( buf_size );
	}
	else{
		dev.write( (char*)data.data(), size() );
	}
	return true;
}
