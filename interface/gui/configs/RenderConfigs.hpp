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

#ifndef RENDER_CONFIGS_HPP
#define RENDER_CONFIGS_HPP

#include "ConfigChooser.hpp"

namespace Overmix{

class ARender;

class ARenderConfig : public AConfig{
	Q_OBJECT
	
	public:
		ARenderConfig( QWidget* parent ) : AConfig(parent) { }
		
		virtual std::unique_ptr<ARender> getRender() const = 0;
		
	signals:
		void changed();
};

class RenderConfigChooser : public ConfigChooser<ARenderConfig>{
	Q_OBJECT
	
	public:
		RenderConfigChooser( QWidget* parent, bool expand=false );
		virtual void p_initialize() override;
		
		std::unique_ptr<ARender> getRender() const;
		
		QString name() const override{ return "Render selector"; }
		QString discription() const override{ return "Selects a render"; }
		
	signals:
		void changed();
};

}

#endif
