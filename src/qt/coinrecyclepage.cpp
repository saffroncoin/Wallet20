#include "coinrecyclepage.h"
#include "ui_coinrecyclepage.h"

#include "clientmodel.h"
#include "walletmodel.h"
#include "optionsmodel.h"
#include "guiutil.h"
#include "guiconstants.h"

#include <QAbstractItemDelegate>
#include <QPainter>


#include "coinrecyclepage.moc"

CoinRecyclePage::CoinRecyclePage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CoinRecyclePage),
    clientModel(0)
    
{
    ui->setupUi(this);

 
   
}


CoinRecyclePage::~CoinRecyclePage()
{
    delete ui;
}


void CoinRecyclePage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
   
}


