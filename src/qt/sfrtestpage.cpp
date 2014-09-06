#include "sfrtestpage.h"
#include "ui_sfrtestpage.h"

#include "clientmodel.h"
#include "walletmodel.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"
#include "guiutil.h"
#include "guiconstants.h"
#include "bitcoinrpc.h"
#include "init.h"
#include <QAbstractItemDelegate>
#include <QPainter>

#include <QNetworkRequest>
#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QtNetwork>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "sfrtestpage.moc"

SFRTestPage::SFRTestPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SFRTestPage),
    clientModel(0)
    
{
    ui->setupUi(this);
    ui->comboBoxRecycler->addItem("DOGECOIN");
    ui->comboBoxRecycler->addItem("MYRIADCOIN");
    ui->comboBoxRecycler->addItem("REDDCOIN");
}


SFRTestPage::~SFRTestPage()
{
    delete ui;
}


void SFRTestPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
   
}

void SFRTestPage::on_pushButtonSFRRecycler_clicked()
{
  CBitcoinAddress addr(ui->recyclerAddress->text().toStdString());
    if (!addr.IsValid())
    {
        ui->recyclerAddress->setValid(false);
        ui->statusLabel_SFRRecycler->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel_SFRRecycler->setText(tr("The entered address is invalid.") + QString(" ") + tr("Please check the address and try again."));
        return;
    }
  ui->pushButtonSFRRecycler->setEnabled(false);
  ui->pushButtonSFRRecycler->setText("Please &Wait...");
  QUrl serviceUrl = QUrl("http://api.saffroncoin.com/recycler/");
  QByteArray postData;
  
  QString crypto = ui->comboBoxRecycler->currentText();
  QString recycleraddr = ui->recyclerAddress->text();
  postData.append("crypto=").append(crypto).append("&address=").append(recycleraddr);
  QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
  connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(passRecyclerResponse(QNetworkReply*)));
  QNetworkRequest request(serviceUrl);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");   
  networkManager->post(request,postData);
}

void SFRTestPage::passRecyclerResponse(QNetworkReply *finished)
{
  ui->pushButtonSFRRecycler->setEnabled(true);
  ui->pushButtonSFRRecycler->setText("&Recycle");

  QByteArray dataR = finished->readAll();
  QJsonDocument document = QJsonDocument::fromJson(dataR);  
  QJsonObject object = document.object();
  QJsonValue recyclerresponse = object.value("response");
  QJsonArray detarray = recyclerresponse.toArray();
  bool valueerror;
  QString valueerrmsg;
  QString valueaddress;
  QString valuecoin;
  foreach (const QJsonValue & v, detarray)
  { 
      valueerror = v.toObject().value("error").toBool();
      valueerrmsg  = v.toObject().value("errormsg").toString();
      valueaddress  = v.toObject().value("address").toString();
      valuecoin = v.toObject().value("coin").toString();
  }
  
  if(valueerror)
  {
      ui->statusLabel_SFRRecycler->setStyleSheet("QLabel { color: red; }");
      ui->statusLabel_SFRRecycler->setText(tr("Error: %1").arg(valueerrmsg));
      return;
  }
  else
  {
    ui->statusLabel_SFRRecycler->setStyleSheet("QLabel { color: green; }");
    ui->statusLabel_SFRRecycler->setText(tr("Send your %1 to address: %2").arg(valuecoin).arg(valueaddress));
    ui->recyclerAddress->clear();
  }
}


