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

#include <QComboBox>
#include <QVBoxLayout>


template<class Config>
class ConfigChooser : public AConfig{
	//NOTE: no Q_OBJECT, because fake-shit
	
	private:
		std::vector<std::unique_ptr<Config>> configs;
		bool expand; //Should show the sub-config below (true), or in pop-up (false)?
		QComboBox dropbox;
		
		void setSub( QWidget* edit ){
			if( expand ){
				QLayoutItem* child;
				while( (child = layout()->takeAt(1)) != nullptr ) child->widget()->hide();
				layout()->addWidget( edit );
				edit->show();
			}
		}
		
	protected:
		Config& getSelected() const
			{ return *(configs[dropbox.currentIndex()]); }
		
	public:
		ConfigChooser( QWidget* parent, bool expand=false )
			:	AConfig( parent ), expand(expand), dropbox( parent ){
			
			setLayout( new QVBoxLayout( this ) );
			layout()->addWidget( &dropbox );
			layout()->setContentsMargins( 0,0,0,0 );
			
			//Ugly lambda, as we can't make slots because of moc not supporting templates
			connect( &dropbox, static_cast<void (QComboBox::*)(int)>
(&QComboBox::currentIndexChanged), [&](int){
					auto& config = getSelected();
					config.initialize();
					setSub( &config );
				} );
		}
		
		template<class Config2, typename ... Args>
		void addConfig( Args... args ){
			configs.emplace_back( std::make_unique<Config2>( this, args... ) );
			dropbox.addItem( configs.back()->name() );
			configs.back()->hide();
				
			if( configs.size() == 1 )
				setSub( configs.back().get() );
		}
};


#endif

