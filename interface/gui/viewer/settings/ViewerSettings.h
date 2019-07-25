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

#ifndef SETTINGS__VIEWER_SETTINGS_H
#define SETTINGS__VIEWER_SETTINGS_H

#include "Setting.h"

class ViewerSettings{
	private:
		QSettings& settings;
		
	public:
		ViewerSettings( QSettings& settings ) : settings(settings) {}
		
		Setting<bool> auto_aspect_ratio()  { return { settings, "viewer/aspect_ratio"   , true  }; }
		Setting<bool> auto_downscale_only(){ return { settings, "viewer/downscale"      , true  }; }
		Setting<bool> auto_upscale_only()  { return { settings, "viewer/upscale"        , false }; }
		
		Setting<bool> restrict_viewpoint() { return { settings, "viewer/restrict"       , true  }; }
		Setting<bool> initial_resize()     { return { settings, "viewer/initial_resize" , true  }; }
		Setting<bool> keep_resize()        { return { settings, "viewer/keep_resize"    , false }; }
		
		Setting<bool> smooth_scaling()     { return { settings, "viewer/smooth_scaling" , true }; }
		
};

#endif
