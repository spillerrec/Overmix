/*
	This file is part of imgviewer.

	imgviewer is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	imgviewer is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with imgviewer.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef COLOR_MANAGER_H
#define COLOR_MANAGER_H

#include <QString>
#include <vector>
#include <lcms2.h>

class QImage;

class colorManager{
	public:
		struct MonitorIcc{
			cmsHPROFILE profile;
			cmsHTRANSFORM transform_srgb;
			
			MonitorIcc( cmsHPROFILE profile ) : profile( profile ) { }
		};
		std::vector<MonitorIcc> monitors;
		
		cmsHPROFILE p_srgb;
		
		
	public:
		colorManager();
		~colorManager();
		
		void do_transform( QImage *img, unsigned monitor, cmsHTRANSFORM transform ) const;
		
		cmsHPROFILE get_profile( unsigned char *data, unsigned len ) const{
			return cmsOpenProfileFromMem( (const void*)data, len );
		}
		cmsHTRANSFORM get_transform( cmsHPROFILE in, unsigned monitor ) const;
		void delete_transform( cmsHTRANSFORM transform ) const{
			if( transform )
				cmsDeleteTransform( transform );
		}
};


#endif