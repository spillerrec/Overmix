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

#ifndef A_RENDER_HPP
#define A_RENDER_HPP

namespace Overmix{

class AContainer;
class ImageEx;

class AProcessWatcher{
	public:
		virtual void setTotal( int total ) = 0;
		virtual void setCurrent( int current ) = 0;
		virtual int getCurrent() const = 0;
		virtual bool shouldCancel() const = 0;
		
		void add( int amount=1 ){ setCurrent( getCurrent() + amount ); }
		
		template<typename Func>
		void loopAll( int total, Func f ){
			setTotal( total );
			for( int i=0; i<total; i++ ){
				f( i );
				setCurrent( i );
			}
		}
};

class ProgressWrapper : public AProcessWatcher{
	private:
		AProcessWatcher* watcher;
		
	public:
		ProgressWrapper( AProcessWatcher* watcher ) : watcher(watcher) { }
		void setTotal  ( int total   ) override { if( watcher ) watcher->setTotal( total ); }
		void setCurrent( int current ) override { if( watcher ) watcher->setCurrent( current ); }
		int  getCurrent()       const  override { return watcher ? watcher->getCurrent() : 0; }
		bool shouldCancel()     const  override { return watcher ? watcher->shouldCancel() : false; }
};

class ARender{
	public:
		virtual ImageEx render( const AContainer& group, AProcessWatcher* watcher=nullptr ) const = 0;
};

}

#endif