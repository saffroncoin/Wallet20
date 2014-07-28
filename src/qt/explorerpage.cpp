#include "explorerpage.h"
#include "ui_explorerpage.h"

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


#include "explorerpage.moc"

ExplorerPage::ExplorerPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ExplorerPage),
    walletModel(0),
    clientModel(0)
{
    ui->setupUi(this);
    nam = new QNetworkAccessManager(this);           
    ui->webViewExplorer->setHidden(true);
    connect(nam,SIGNAL(finished(QNetworkReply*)),this,SLOT(finished(QNetworkReply*)));          
    connect(ui->submitButton,SIGNAL(clicked()),this,SLOT(GetHttpContent()));
}

ExplorerPage::~ExplorerPage()
{
    delete ui;
}

void ExplorerPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
    }
}

void ExplorerPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
}

void ExplorerPage::finished(QNetworkReply *reply) {
  ui->webViewExplorer->setHidden(false);
  
}

void ExplorerPage::GetHttpContent() {
  ui->webViewExplorer->setHidden(false);
  ui->submitButton->setHidden(true);

  //QUrl url;
  //QNetworkRequest request;
  //url.setUrl("http://explorer.saffroncoin.com/search?q=");
  //request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/x-www-form-urlencoded"));
  QString url = "http://62.210.76.197/";            
  //QString data = ui->searchData->text();
  //QString final = url + data;
  //QByteArray postData;
  //postData.append(data.toLatin1());
 //nam->get(QNetworkRequest(QUrl(final)));
  //ui->webViewExplorer->load(QNetworkRequest(final), QNetworkAccessManager::PostOperation, postData);
  ui->webViewExplorer->load(QNetworkRequest(url));

}
