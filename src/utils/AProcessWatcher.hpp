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

#ifndef A_PROCESS_WATCHER_HPP
#define A_PROCESS_WATCHER_HPP

#include <memory>
#include <string>

namespace Overmix{

class AProcessWatcher{
	public:
		virtual ~AProcessWatcher() { }
		virtual void setTotal( int total ) = 0;
		virtual void setCurrent( int current ) = 0;
		virtual int getCurrent() const = 0;
		virtual bool shouldCancel() const = 0;
		
 		virtual AProcessWatcher* makeSubtask( std::string name ) = 0;
		
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

class Progress{
	private:
		AProcessWatcher* watcher;
		int total;
		
	public:
		template<typename Func>
		void loopAll( Func f ){
			for( int i=0; i<total && !shouldCancel(); i++, add() )
				f( i );
		}
		
		Progress( std::string title, int total, AProcessWatcher* watcher )
			:	watcher(watcher), total(total)
			{ if( watcher ) watcher->setTotal( total ); }
		
		template<typename Func>
		Progress( std::string title, int total, AProcessWatcher* watcher, Func f )
			:	Progress( title, total, watcher )
			{ loopAll( f ); }
		
		void add( int amount=1 )      { if( watcher ) watcher->add( amount ); }
		bool shouldCancel()     const { return watcher ? watcher->shouldCancel() : false; }
		void setCurrent( int current ){ if( watcher ) watcher->setCurrent( current ); }
		
};

class MultiProgress{
	private:
		AProcessWatcher* watcher;
		
	public:
		MultiProgress( std::string title, int progress_amount, AProcessWatcher* watcher )
			:	watcher(watcher)
			{
			}
		
		AProcessWatcher* makeWatcher(){ return nullptr; };
		Progress makeProgress( std::string title, int total )
			{ return { title, total, makeWatcher() }; }
			
		bool shouldCancel() const { return watcher ? watcher->shouldCancel() : false; }
};

}

#endif
