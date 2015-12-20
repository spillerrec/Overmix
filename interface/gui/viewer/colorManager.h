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

#include <lcms2.h>
#include <QString>
#include <memory>
#include <vector>


//TODO: see if we can safely use std::unique_ptr to handle it, or make our own class
class ColorTransform{
	private:
		cmsHTRANSFORM transform{ nullptr };
		
	public:
		ColorTransform() { }
		ColorTransform( cmsHTRANSFORM transform ) : transform(transform) { }
		ColorTransform( const ColorTransform& copy ) = delete;
		ColorTransform( ColorTransform&& other ){
			transform = other.transform;
			other.transform = nullptr;
		}
		~ColorTransform(){ cmsDeleteTransform( transform ); }
		operator bool() const{ return transform; }
		
		void execute( const void* input_buffer, void* output_buffer, unsigned size )
			{ cmsDoTransform( transform, input_buffer, output_buffer, size ); }
};

class ColorProfile{
	private:
		cmsHPROFILE profile{ nullptr };
		ColorProfile( cmsHPROFILE profile ) : profile(profile) { }
		
	public:
		ColorProfile() { }
		ColorProfile( const ColorProfile& copy ) = delete;
		ColorProfile( ColorProfile&& other ){
			profile = other.profile;
			other.profile = nullptr;
		}
		ColorProfile& operator=( ColorProfile&& other ){
			cmsCloseProfile( profile );
			profile = other.profile;
			other.profile = nullptr;
			return *this;
		}
		~ColorProfile(){ cmsCloseProfile( profile ); }
		
		operator bool() const{ return profile; }
		
		static ColorProfile fromMem( const void* data, unsigned len )
			{ return { cmsOpenProfileFromMem( data, len ) }; }
		
		static ColorProfile fromFile( const char* path, const char* options )
			{ return { cmsOpenProfileFromFile( path, options ) }; }
		
		static ColorProfile sRgb(){ return { cmsCreate_sRGBProfile() }; }
		
		ColorTransform transformTo( const ColorProfile& to, unsigned in_format, unsigned out_format, unsigned intent, unsigned flags=0 ) const
			{ return ColorTransform( cmsCreateTransform( profile, in_format, to.profile, out_format, intent, flags ) ); }
};

class colorManager{
	public:
		std::vector<ColorProfile> monitors;
		
		ColorProfile p_srgb{ ColorProfile::sRgb() };
		
		
	public:
		colorManager();
		
		void doTransform( class QImage& img, const ColorProfile& in, unsigned monitor ) const;
};


#endif