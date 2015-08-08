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

#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <string>
#include <fstream>

#include "Geometry.hpp"

#include <QLoggingCategory>
#include <QElapsedTimer>

Q_DECLARE_LOGGING_CATEGORY(LogTiming)
Q_DECLARE_LOGGING_CATEGORY(LogDelta)

class ImageContainer;
class QImage;

struct Timer{
	QElapsedTimer t;
	QString name;
	Timer( QString name ) : name(name) { t.start(); }
	~Timer(){ print(name); }
	void print( QString name )
		{ qCDebug(LogTiming) << name << " completed in: " << t.restart(); }
};

namespace debug{
	
	void make_slide( QImage image, QString dir, double scale );
	
	void make_low_res( QImage image, QString dir, unsigned scale );
	
	void output_transfers_functions( QString path );
	
	void output_rectable( ImageContainer& imgs, Rectangle<> area );
	
	
	class CsvFile{
		private:
			std::ofstream file;
			
		public:	
			CsvFile( std::string filename ) : file( filename ) { }
			~CsvFile(){ file.close(); }
			
			CsvFile& add( const char* const value ){
				file << "\"" << value << "\",";
				return *this;
			}
			CsvFile& add( std::string value ){ return add( value.c_str() ); }
			
			template<typename T>
			CsvFile& add( T value ){
				file << value << ",";
				return *this;
			}
			
			void stop(){ file << std::endl; }
	};
}

#endif