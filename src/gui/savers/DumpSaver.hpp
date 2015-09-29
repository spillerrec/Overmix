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

#ifndef DUMP_SAVER_HPP
#define DUMP_SAVER_HPP

#include "ASaver.hpp"

#include "ui_DumpSaver.h"

namespace Overmix{

class DumpSaver: public ASaver, private Ui::DumpSaver{
	Q_OBJECT
	
	public:
		explicit DumpSaver( const ImageEx& image, QString filename );
		virtual void save( const ImageEx& image, QString filename ) override;
};

}

#endif