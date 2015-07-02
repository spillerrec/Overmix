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

#ifndef CONFIG_CHOOSER_HPP
#define CONFIG_CHOOSER_HPP

#include "AConfig.hpp"

#include <memory>
#include <vector>


template<class Config>
class ConfigChooser : public AConfig{
	//NOTE: no Q_OBJECT, because fake-shit
	
	private:
		std::vector<std::unique_ptr<Config>> configs;
		bool expand; //Should show the sub-config below (true), or in pop-up (false)?
		//TODO: dropbox
		
	protected:
		Config& getSelected() const{
			//TODO: assert range
			int selected = 0; //TODO: get value from dropbox
			return *(configs[selected]);
		}
		
	public:
		ConfigChooser( QObject* parent ) : AConfig( parent ){
			//Add vertical layout
			//Top: dropbox
			//Bottom: sub-editor if expand==true
		}
		
		void addConfig( std::unique_ptr<Config> config ){
			configs.emplace_back( std::move( config ) );
			//TODO: add to drop-box
		}
};


#endif

