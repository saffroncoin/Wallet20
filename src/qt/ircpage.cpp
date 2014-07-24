#include "ircpage.h"
#include "ui_ircpage.h"

#include "clientmodel.h"
#include "walletmodel.h"
#include "optionsmodel.h"
#include "guiutil.h"
#include "guiconstants.h"

#include <QAbstractItemDelegate>
#include <QPainter>


#include "ircpage.moc"

IRCPage::IRCPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IRCPage),
    clientModel(0)
    
{
    ui->setupUi(this);

 
   
}


IRCPage::~IRCPage()
{
    delete ui;
}


void IRCPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
   
}


