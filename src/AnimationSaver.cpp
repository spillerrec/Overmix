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


#include "AnimationSaver.hpp"

#include <fstream>

using namespace std;


void AnimationSaver::write( QString output_name ){
	ofstream file( output_name.toLocal8Bit().constData() );
	
	sort( frames.begin(), frames.end(), [](pair<int,int> first, pair<int,int> second){
			return first.first < second.first;
		} );
	
	for( auto frame : frames ){
		file << "<stack><layer src=\"data/" << images[frame.second].name.toUtf8().constData() << "\" ";
		file << "x=\"" << images[frame.second].x << "\" ";
		file << "y=\"" << images[frame.second].y << "\"/></stack>";
	}
	
	file.close();
}