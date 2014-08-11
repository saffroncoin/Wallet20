#include "sfrpaypage.h"
#include "ui_sfrpaypage.h"

#include "clientmodel.h"
#include "walletmodel.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"
#include "transactiontablemodel.h"
#include "transactionfilterproxy.h"
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
#define DECORATION_SIZE 64


#include "sfrpaypage.moc"


SFRPayPage::SFRPayPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SFRPayPage),
    walletModel(0),
    clientModel(0)
{
    ui->setupUi(this);      
    ui->comboBoxVoucher->addItem("100");
    ui->comboBoxVoucher->addItem("200");
    ui->comboBoxVoucher->addItem("500");
    ui->comboBoxVoucher->addItem("1000");
    ui->comboBoxVoucher->addItem("2000"); 
    ui->comboBoxVoucher->addItem("3000"); 
    ui->comboBoxVoucher->addItem("5000");
    ui->comboBoxVoucher->addItem("7000");
    ui->comboBoxVoucher->addItem("10000");
    
    //ui->webViewSFRPay->setHidden(true);
   
}

SFRPayPage::~SFRPayPage()
{
    delete ui;
}

void SFRPayPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
    }
}

void SFRPayPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
}

void SFRPayPage::on_pushButtonSFRPayBuy_clicked()
{
  ui->statusLabel_SFRPay->clear();
  ui->labelRecentStatus->clear();
  QString sfrpay_email = ui->sfrPayEmail->text().trimmed();
  QString amount;
  if(sfrpay_email.isEmpty())
    {
      ui->statusLabel_SFRPay->setStyleSheet("QLabel { color: red; }");
      ui->statusLabel_SFRPay->setText(QString("<nobr>") + tr("You haven't provided an Email.") + QString("</nobr>"));
      return;
    }
  if(sfrpay_email != ui->sfrPayConfirmEmail->text())
  {
    ui->statusLabel_SFRPay->setStyleSheet("QLabel { color: red; }");
    ui->statusLabel_SFRPay->setText(QString("<nobr>") + tr("The Email Addresses do not match.") + QString("</nobr>"));
    return;
  }
  
  amount = ui->comboBoxVoucher->currentText();
 
  ui->pushButtonSFRPayBuy->setEnabled(false);
  ui->pushButtonSFRPayBuy->setText("Please &Wait...");
  ui->labelRecentAmount->setText(amount);
  ui->labelRecentEmail->setText(sfrpay_email);
  QUrl serviceUrl = QUrl("http://saffroncoin.com/sfrpay/process/");
  QByteArray postData;
  
  postData.append("amount=").append(amount).append("&email=").append(sfrpay_email);
  QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
  connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(passSFRPayResponse(QNetworkReply*)));
  QNetworkRequest request(serviceUrl);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");   
  networkManager->post(request,postData);
  ui->labelRecentStatus->setStyleSheet("QLabel { color: green; }");
  ui->labelRecentStatus->setText("The payment should be made in your external default web browser");

}

void SFRPayPage::passSFRPayResponse( QNetworkReply *finished )
{
    ui->pushButtonSFRPayBuy->setEnabled(true);
    ui->pushButtonSFRPayBuy->setText("Initiate &Request");
    QByteArray dataR = finished->readAll();
    QJsonDocument document = QJsonDocument::fromJson(dataR);  
    QJsonObject object = document.object();
    QJsonValue sfrpayresponse = object.value("response");
    QJsonArray detarray = sfrpayresponse.toArray();
    bool valueerror;
    QString valueerrmsg;
    QString valuetoken;
    QString valuefiat;
    foreach (const QJsonValue & v, detarray)
    { 
        valueerror = v.toObject().value("error").toBool();
        valueerrmsg  = v.toObject().value("errormsg").toString();
        valuetoken  = v.toObject().value("token").toString();
        valuefiat  = v.toObject().value("fiatamt").toString();
    }

    if(valueerror)
    {
        ui->statusLabel_SFRPay->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel_SFRPay->setText(tr("Error: %1").arg(valueerrmsg));
        return;
    }
    QString link="http://saffroncoin.com/sfrpay/checkout/?";
    link.append(valuetoken);
    QDesktopServices::openUrl(QUrl(link));
    ui->labelRecentFiat->setText(valuefiat);
    ui->statusLabel_SFRPay->clear();
    ui->sfrPayEmail->clear();
    ui->sfrPayConfirmEmail->clear();
}

void SFRPayPage::on_pushButtonSFRPayReset_clicked()
{
  ui->statusLabel_SFRPay->clear();
  ui->sfrPayEmail->clear();
  ui->sfrPayConfirmEmail->clear();
  ui->labelRecentStatus->clear();
}

void SFRPayPage::on_pushButtonRedeem_clicked()
{
  CBitcoinAddress addr(ui->lineEditRedeemAddress->text().toStdString());
    if (!addr.IsValid())
    {
        ui->lineEditRedeemAddress->setValid(false);
        ui->statusLabelRedeem->setStyleSheet("QLabel { color: red; }");
        ui->statusLabelRedeem->setText(tr("The entered address is invalid.") + QString(" ") + tr("Please check the address and try again."));
        return;
    }
  ui->pushButtonRedeem->setEnabled(false);
  ui->pushButtonRedeem->setText("Please &Wait...");
  QString redeemcode = ui->lineEditRedeemCode->text().trimmed();
  QString redeemaddress = ui->lineEditRedeemAddress->text();
  QUrl serviceUrl = QUrl("http://saffroncoin.com/sfrpay/redeem/");
  QByteArray postData;
  
  postData.append("code=").append(redeemcode).append("&address=").append(redeemaddress);
  QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
  connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(passRedeemResponse(QNetworkReply*)));
  QNetworkRequest request(serviceUrl);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");   
  networkManager->post(request,postData);
  ui->statusLabelRedeem->setStyleSheet("QLabel { color: orange; }");
  ui->statusLabelRedeem->setText("This code is being verified...");
}

void SFRPayPage::passRedeemResponse(QNetworkReply *finished)
{
    ui->pushButtonRedeem->setEnabled(true);
    ui->pushButtonRedeem->setText("Redeem");
    QByteArray dataR = finished->readAll();
    QJsonDocument document = QJsonDocument::fromJson(dataR);  
    QJsonObject object = document.object();
    QJsonValue redeemresponse = object.value("response");
    QJsonArray detarray = redeemresponse.toArray();
    bool valueerror;
    QString valueerrmsg;
    double valueamount;
    foreach (const QJsonValue & v, detarray)
    { 
        valueerror = v.toObject().value("error").toBool();
        valueerrmsg  = v.toObject().value("errormsg").toString();
        valueamount  = v.toObject().value("amount").toDouble();
    }
    
    if(valueerror)
    {
        ui->statusLabelRedeem->setStyleSheet("QLabel { color: red; }");
        ui->statusLabelRedeem->setText(tr("Error: %1").arg(valueerrmsg));
        return;
    }
    else
    {
      ui->statusLabelRedeem->setStyleSheet("QLabel { color: green; }");
      ui->statusLabelRedeem->setText(tr("This code is successfully verified. %1 SFR will be sent shortly.").arg(valueamount,0,'g',8));
      ui->lineEditRedeemCode->clear();
      ui->lineEditRedeemAddress->clear();

    }
}