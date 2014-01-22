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

#ifndef PLANE_RENDER_HPP
#define PLANE_RENDER_HPP

#include "ARender.hpp"

#include "MultiPlaneIterator.hpp"

class PlaneRender : public ARender{
	protected:
		virtual void* data() const{ return nullptr; }
		typedef void pixel_func( MultiPlaneLineIterator &it );
		virtual pixel_func* pixel() const = 0;
	public:
		virtual ImageEx* render( const AImageAligner& aligner, unsigned max_count=-1, AProcessWatcher* watcher=nullptr ) const override;
		
};

#endif