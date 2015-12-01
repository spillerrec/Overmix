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

#ifndef A_ALIGNER_HPP
#define A_ALIGNER_HPP

namespace Overmix{

class AAligner{
	public:
		virtual ~AAligner() { }
		
		virtual void align( class AContainer& container, class AProcessWatcher* watcher=nullptr ) const = 0;
};

}

#endif