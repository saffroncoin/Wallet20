#include "topuppage.h"
#include "ui_topuppage.h"

#include "clientmodel.h"
#include "walletmodel.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"
#include "transactiontablemodel.h"
#include "transactionfilterproxy.h"
#include "guiutil.h"
#include "guiconstants.h"

#include <QAbstractItemDelegate>
#include <QPainter>

#define DECORATION_SIZE 64


#include "topuppage.moc"

TopupPage::TopupPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TopupPage),
    walletModel(0),
    clientModel(0)
{
    ui->setupUi(this);
}

TopupPage::~TopupPage()
{
    delete ui;
}

void TopupPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
    }
}

void TopupPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
}
