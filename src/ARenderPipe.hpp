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

class ImageEx;

class ARenderPipe{
	private:
		ImageEx* cache{ nullptr };
		const ImageEx* base{ nullptr };
		
	protected:
		virtual ImageEx* render( const ImageEx& img ) const = 0;
		
		template<typename T>
		void set( T& out, T in ){
			if( !( out == in ) ){
				invalidate();
				out = in;
			}
		}
		
	public:
		const ImageEx& get( const ImageEx& img ){
			if( !cache || base != &img ){
				invalidate();
				cache = render( img );
				base = &img;
			}
			return *cache;
		}
		
		/** Invalidates any caches */
		void invalidate(){
			delete cache;
			cache = nullptr;
		}
		virtual ~ARenderPipe(){
			invalidate();
		}
};

#endif