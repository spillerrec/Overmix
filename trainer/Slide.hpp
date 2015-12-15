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

#ifndef SLIDE_HPP
#define SLIDE_HPP

#include <vector>
#include <QString>

namespace Overmix{

struct ImageInfo{
	QString filename;
	bool interlazed{ false };
	bool interlaze_predicted{ false };
	ImageInfo( QString filename, bool interlazed )
		: filename(filename), interlazed(interlazed) { }
	
	bool interlazeTest();
};

class Slide{
	public:
		std::vector<ImageInfo> images;
		
	public:
		void add( QString filename, bool interlazed )
			{ images.emplace_back( filename, interlazed ); }
		
		QString saveXml( QString filename );
		QString loadXml( QString filename );
};

}

#endif