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

#ifndef SIMPLE_RENDER_HPP
#define SIMPLE_RENDER_HPP

#include "ARender.hpp"

class SimpleRender : public ARender{
	public:
		enum Filters{
			FOR_MERGING,
			AVERAGE,
			SIMPLE,
			SIMPLE_SLIDE,
			DARK_SELECT
		};
	
	protected:
		Filters filter;
		bool upscale_chroma;
		
	public:
		SimpleRender( Filters filter=AVERAGE, bool upscale_chroma=true ) : filter( filter ), upscale_chroma( upscale_chroma ) { }
		
		virtual ImageEx* render( const AImageAligner& aligner, unsigned max_count=-1, AProcessWatcher* watcher=nullptr ) const override;
		
		void setFilter( Filters f ){ filter = f; }
		void setChromaUpscale( bool upscale ){ upscale_chroma = upscale; }
};

#endif