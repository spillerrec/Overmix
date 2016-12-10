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

#ifndef CLUSTER_ALIGNER_HPP
#define CLUSTER_ALIGNER_HPP

#include "AAligner.hpp"

namespace Overmix{

class ClusterAligner : public AAligner{
	private:
		unsigned min_groups{  1 };
		unsigned max_groups{ 20 };
	
	public:
		ClusterAligner() { }
		ClusterAligner( unsigned min_groups, unsigned max_groups )
			: min_groups(min_groups), max_groups(max_groups) { }
		virtual void align( class AContainer& container, class AProcessWatcher* watcher=nullptr ) const override;
};

}

#endif
