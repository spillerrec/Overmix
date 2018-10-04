#include "ExceptionCatcher.hpp"
#include "ui_ExceptionCatcher.h"

#include <QSysInfo>
#include <QGuiApplication>
#include <QClipboard>
#include <QDesktopServices>

ExceptionCatcher::ExceptionCatcher( QWidget* parent )
	:	QDialog(parent), ui(new Ui::ExceptionCatcher)
{
    ui->setupUi(this);
	
	//Setup signals
	connect( ui->btn_close,  SIGNAL(clicked(bool)), this, SLOT(close() ) );
	connect( ui->btn_copy,   SIGNAL(clicked(bool)), this, SLOT(copy()  ) );
	connect( ui->btn_report, SIGNAL(clicked(bool)), this, SLOT(report()) );
	
	auto sysinfo = QSysInfo::prettyProductName() + "\n";
	sysinfo += QString( OVERMIX_VERSION_STRING ) + "\n";
	//TODO: Amount of RAM
	
	ui->system_text->setPlainText( sysinfo );
}


ExceptionCatcher::ExceptionCatcher( QString what, QWidget* parent )
	:	ExceptionCatcher( parent )
	{ ui->exception_text->setPlainText( what ); }

ExceptionCatcher::~ExceptionCatcher()
	{ delete ui; }


void ExceptionCatcher::close(){
	accept();
}

void ExceptionCatcher::copy(){
	auto clipboard = QGuiApplication::clipboard();
	clipboard->setText( ui->exception_text->toPlainText() );
}

void ExceptionCatcher::report(){
	QDesktopServices::openUrl( QUrl( "https://github.com/spillerrec/Overmix/issues" ) );
}
