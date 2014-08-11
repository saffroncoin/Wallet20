#include "tradepage.h"
#include "ui_tradepage.h"

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


#include "tradepage.moc"

TradePage::TradePage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TradePage),
    walletModel(0),
    clientModel(0)
{
    ui->setupUi(this);        
    ui->webViewTrade->setHidden(true);
    ui->frameExchangesBottom->setHidden(true);         
    connect(ui->pushButtonBittrex,SIGNAL(clicked()),this,SLOT(LoadBittrexWebview()));
    connect(ui->pushButtonCryptsy,SIGNAL(clicked()),this,SLOT(LoadCryptsyWebview()));
}

TradePage::~TradePage()
{
    delete ui;
}

void TradePage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
    }
}

void TradePage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
}

void TradePage::LoadBittrexWebview() {
  ui->webViewTrade->setHidden(false);
  ui->frameExchangesCenter->setHidden(true);
  ui->frameExchangesBottom->setHidden(false);
  QString url = "https://www.bittrex.com/Market/?MarketName=BTC-SFR";            
  ui->webViewTrade->load(QNetworkRequest(url));
  connect(ui->pushButtonBittrex_bottom,SIGNAL(clicked()),this,SLOT(LoadBittrexWebviewBottom()));
  connect(ui->pushButtonCryptsy_bottom,SIGNAL(clicked()),this,SLOT(LoadCryptsyWebviewBottom()));
}

void TradePage::LoadCryptsyWebview() {
  ui->webViewTrade->setHidden(false);
  ui->frameExchangesCenter->setHidden(true);
  ui->frameExchangesBottom->setHidden(false);
  QString url = "https://www.cryptsy.com/markets/view/270";            
  ui->webViewTrade->load(QNetworkRequest(url));
  connect(ui->pushButtonBittrex_bottom,SIGNAL(clicked()),this,SLOT(LoadBittrexWebviewBottom()));
  connect(ui->pushButtonCryptsy_bottom,SIGNAL(clicked()),this,SLOT(LoadCryptsyWebviewBottom()));
}


void TradePage::LoadBittrexWebviewBottom() {
  ui->webViewTrade->setHidden(false);
  ui->frameExchangesCenter->setHidden(true);
  ui->frameExchangesBottom->setHidden(false);
  QString url = "https://www.bittrex.com/Market/?MarketName=BTC-SFR";            
  ui->webViewTrade->load(QNetworkRequest(url));
}

void TradePage::LoadCryptsyWebviewBottom() {
  ui->webViewTrade->setHidden(false);
  ui->frameExchangesCenter->setHidden(true);
  ui->frameExchangesBottom->setHidden(false);
  QString url = "https://www.cryptsy.com/markets/view/270";            
  ui->webViewTrade->load(QNetworkRequest(url));
}


