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

#ifndef A_RENDER_PIPE_HPP
#define A_RENDER_PIPE_HPP

#include "planes/ImageEx.hpp"

#include <utility>

namespace Overmix{

class ARenderPipe{
	private:
		ImageEx cache;
		
	protected:
		virtual bool renderNeeded() const = 0;
		virtual ImageEx render( const ImageEx& img ) const = 0;
		
		template<typename T>
		void set( T& out, T in ){
			if( !( out == in ) ){
				invalidate();
				out = in;
			}
		}
		
	public:
		typedef std::pair<const ImageEx&, bool> ImageCache;
		
		ImageCache get( ImageCache img ){
			if( !renderNeeded() )
				return img;
			
			if( img.second || !cache.is_valid() )
				cache = render( img.first );
			
			return ImageCache( cache, true );
		}
		
		/** Invalidates any caches */
		void invalidate(){ cache = ImageEx(); }
		
		virtual ~ARenderPipe(){ }
};

}

#endif