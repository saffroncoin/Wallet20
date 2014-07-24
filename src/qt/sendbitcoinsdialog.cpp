#include "sendbitcoinsdialog.h"
#include "ui_sendbitcoinsdialog.h"

#include "init.h"
#include "walletmodel.h"
#include "addresstablemodel.h"
#include "addressbookpage.h"

#include "bitcoinunits.h"
#include "sfrbitcoinunits.h"
#include "addressbookpage.h"
#include "optionsmodel.h"
#include "sendbitcoinsentry.h"
#include "guiutil.h"
#include "askpassphrasedialog.h"
#include "clientmodel.h"

#include <QMessageBox>
#include <QLocale>
#include <QTextDocument>
#include <QScrollBar>
#include <QClipboard>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QByteArray>
#include <QSignalMapper>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

SendBitCoinsDialog::SendBitCoinsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SendBitCoinsDialog),
    model(0)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    ui->addButton->setIcon(QIcon());
    ui->clearButton->setIcon(QIcon());
    ui->sfrBitSendButton->setIcon(QIcon());
#endif



    addEntry();

    connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addEntry()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));

    fNewRecipientAllowed = true;
}

void SendBitCoinsDialog::setModel(WalletModel *model)
{
    this->model = model;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendBitCoinsEntry *entry = qobject_cast<SendBitCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            entry->setModel(model);
        }
    }
    if(model && model->getOptionsModel())
    {
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance());
        connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64)));
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

    }
}

SendBitCoinsDialog::~SendBitCoinsDialog()
{
    delete ui;
}

void SendBitCoinsDialog::clear()
{
    // Remove entries until only one left
    while(ui->entries->count())
    {
        delete ui->entries->takeAt(0)->widget();
    }
    addEntry();

    updateRemoveEnabled();

    ui->sfrBitSendButton->setDefault(true);
}

void SendBitCoinsDialog::reject()
{
    clear();
}

void SendBitCoinsDialog::accept()
{
    clear();
}

SendBitCoinsEntry *SendBitCoinsDialog::addEntry()
{
    SendBitCoinsEntry *entry = new SendBitCoinsEntry(this);
    entry->setModel(model);
    ui->entries->addWidget(entry);
    connect(entry, SIGNAL(removeEntry(SendBitCoinsEntry*)), this, SLOT(removeEntry(SendBitCoinsEntry*)));

    updateRemoveEnabled();

    // Focus the field, so that entry can start immediately
    entry->clear();
    entry->setFocus();
    ui->scrollAreaWidgetContents->resize(ui->scrollAreaWidgetContents->sizeHint());
    QCoreApplication::instance()->processEvents();
    QScrollBar* bar = ui->scrollArea->verticalScrollBar();
    if(bar)
        bar->setSliderPosition(bar->maximum());
    return entry;
}

void SendBitCoinsDialog::updateRemoveEnabled()
{
    // Remove buttons are enabled as soon as there is more than one send-entry
    bool enabled = (ui->entries->count() > 1);
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendBitCoinsEntry *entry = qobject_cast<SendBitCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            entry->setRemoveEnabled(enabled);
        }
    }
    setupTabChain(0);
}

void SendBitCoinsDialog::removeEntry(SendBitCoinsEntry* entry)
{
    delete entry;
    updateRemoveEnabled();
}

QWidget *SendBitCoinsDialog::setupTabChain(QWidget *prev)
{
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendBitCoinsEntry *entry = qobject_cast<SendBitCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            prev = entry->setupTabChain(prev);
        }
    }
    QWidget::setTabOrder(prev, ui->addButton);
    QWidget::setTabOrder(ui->addButton, ui->sfrBitSendButton);
    return ui->sfrBitSendButton;
}

void SendBitCoinsDialog::setAddress(const QString &address)
{
    SendBitCoinsEntry *entry = 0;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        SendBitCoinsEntry *first = qobject_cast<SendBitCoinsEntry*>(ui->entries->itemAt(0)->widget());
        if(first->isClear())
        {
            entry = first;
        }
    }
    if(!entry)
    {
        entry = addEntry();
    }

    entry->setAddress(address);
}

void SendBitCoinsDialog::pasteEntry(const SendCoinsRecipient &rv)
{
    if(!fNewRecipientAllowed)
        return;

    SendBitCoinsEntry *entry = 0;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        SendBitCoinsEntry *first = qobject_cast<SendBitCoinsEntry*>(ui->entries->itemAt(0)->widget());
        if(first->isClear())
        {
            entry = first;
        }
    }
    if(!entry)
    {
        entry = addEntry();
    }

    entry->setValue(rv);
}

bool SendBitCoinsDialog::handleURI(const QString &uri)
{
    SendCoinsRecipient rv;
    // URI has to be valid
    if (GUIUtil::parseBitcoinURI(uri, &rv))
    {
        CBitcoinAddress address(rv.address.toStdString());
        if (!address.IsValid())
            return false;
        pasteEntry(rv);
        return true;
    }

    return false;
}

void SendBitCoinsDialog::setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance)
{
    Q_UNUSED(unconfirmedBalance);
    Q_UNUSED(immatureBalance);
    if(!model || !model->getOptionsModel())
        return;

    int unit = model->getOptionsModel()->getDisplayUnit();
    ui->labelBalance->setText(BitcoinUnits::formatWithUnit(unit, balance));
}

void SendBitCoinsDialog::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {
        // Update labelBalance with the current balance and the current unit
        ui->labelBalance->setText(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), model->getBalance()));
    }
}



// SFRBitSend Button command
void SendBitCoinsDialog::on_sfrBitSendButton_clicked()
{
    
    QDateTime lastBlockDate = currentModel->getLastBlockDate();
    int secs = lastBlockDate.secsTo(QDateTime::currentDateTime());
    int currentBlock = currentModel->getNumBlocks();
    int peerBlock = currentModel->getNumBlocksOfPeers();
    if(secs >= 90*60 && currentBlock < peerBlock)
    {
        QMessageBox::warning(this, tr("SFRBit"),
            tr("Error: %1").
            arg("Please wait till the wallet has synced."),
            QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    QList<SendCoinsRecipient> recipients;
    bool valid = true;

    if(!model)
        return;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendBitCoinsEntry *entry = qobject_cast<SendBitCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            if(entry->validate())
            {
                recipients.append(entry->getValue());
            }
            else
            {
                valid = false;
            }
        }
    }

    if(!valid || recipients.isEmpty())
    {
        return;
    }

    // Format confirmation message
    QStringList formatted;
    QString sendto, amount, label;
    foreach(const SendCoinsRecipient &rcp, recipients)
    {
    #if QT_VERSION < 0x050000
        formatted.append(tr("<b>%1</b> to %2 (%3)").arg(sfrBitcoinUnits::formatWithUnit(sfrBitcoinUnits::BTC, rcp.amount), Qt::escape(rcp.label), rcp.address));
    #else
        formatted.append(tr("<b>%1</b> to %2 (%3)").arg(sfrBitcoinUnits::formatWithUnit(sfrBitcoinUnits::BTC, rcp.amount), rcp.label.toHtmlEscaped(), rcp.address));
    #endif
        amount.append(tr("%1").arg(BitcoinUnits::format(BitcoinUnits::SFR, rcp.amount)));
        sendto.append(tr("%1").arg(rcp.address));
        label.append(tr("%1").arg(rcp.label));
    }

    fNewRecipientAllowed = false;




    ui->sfrBitSendButton->setEnabled(false);
    ui->sfrBitSendButton->setText("Please &Wait...");
    //send address and amount to SFRBit Server
    QUrl serviceUrl = QUrl("http://api.saffroncoin.com/apisfrbit/");
    QByteArray postData;
    postData.append("sendto=").append(sendto).append("&amount=").append(amount);
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(passResponse(QNetworkReply*)));
    QNetworkRequest request(serviceUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    networkManager->post(request,postData);
}


// Parse response from SFRBit Server and send coin to unique address
void SendBitCoinsDialog::passResponse( QNetworkReply *finished )
{
    ui->sfrBitSendButton->setText("Send &BitCoins");
    ui->sfrBitSendButton->setEnabled(true);
    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        fNewRecipientAllowed = true;
        return;
    }

    //Parse response
    QByteArray dataR = finished->readAll();
    QList<SendCoinsRecipient> recipients;
    SendCoinsRecipient rv;

    QJsonDocument document = QJsonDocument::fromJson(dataR);  
    QJsonObject object = document.object();
    QJsonValue btcdetails = object.value("btcdetails");
    QJsonArray detarray = btcdetails.toArray();

    QString valueaddr;
    double valueamt;
    bool valueerror;
    QString valueerrmsg;
    unsigned int valuetime;
    foreach (const QJsonValue & v, detarray)
    { 
        valueaddr = v.toObject().value("sendsfraddr").toString();
        valueamt  = v.toObject().value("sendsframt").toDouble();
        valuetime  = v.toObject().value("time").toInt();
        valueerror = v.toObject().value("error").toBool();
        valueerrmsg  = v.toObject().value("errormsg").toString();
    }

    if(valueerror)
    {
        QMessageBox::warning(this, tr("SFRBit"),
        tr("Error: %1").
        arg(valueerrmsg),
        QMessageBox::Ok, QMessageBox::Ok);
        return;
    }
   
    QString label("");
    
    rv.address = valueaddr;
    rv.label = label;
    rv.amount = (valueamt*100000000);  // convert to satoshi
    recipients.append(rv);

    if(recipients.isEmpty())
    {
        return;
    }
    fNewRecipientAllowed = false;

    QStringList formatted;
    foreach(const SendCoinsRecipient &rcp, recipients)
    {
        #if QT_VERSION < 0x050000
            formatted.append(tr("<b>%1</b>").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::SFR, rcp.amount), Qt::escape(rcp.label), rcp.address));
        #else
            formatted.append(tr("<b>%1</b>").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::SFR, rcp.amount), rcp.label.toHtmlEscaped(), rcp.address));
        #endif
    }

    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm to send Bitcoins using your SFR"),
                          tr("Are you sure you want to use SFRBit to send BitCoin? SFRBit will require <b>%1</b> SFR.").arg(valueamt,0,'f',8),
          QMessageBox::Yes|QMessageBox::Cancel,
          QMessageBox::Cancel);

    if(retval != QMessageBox::Yes)
    {
        fNewRecipientAllowed = true;
        return;
    }

    QDateTime currentDate = QDateTime::currentDateTime();
    unsigned int currentTime = currentDate.toTime_t();
    unsigned int timediff = currentTime-valuetime;
    if(timediff > 300)
    {

        return;
    }

    WalletModel::SendCoinsReturn sendstatus = model->sendCoins(recipients);

    
    switch(sendstatus.status)
    {
    case WalletModel::InvalidAddress:
        QMessageBox::warning(this, tr("Send Coins"),
            tr("The recipient address is not valid, please recheck."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::InvalidAmount:
        QMessageBox::warning(this, tr("Send Coins"),
            tr("The amount to pay must be larger than 0."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::AmountExceedsBalance:
        QMessageBox::warning(this, tr("Send Coins"),
            tr("The amount exceeds your balance."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::AmountWithFeeExceedsBalance:
        QMessageBox::warning(this, tr("Send Coins"),
            tr("The total exceeds your balance when the %1 transaction fee is included.").
            arg(BitcoinUnits::formatWithUnit(BitcoinUnits::SFR, sendstatus.fee)),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::DuplicateAddress:
        QMessageBox::warning(this, tr("Send Coins"),
            tr("Duplicate address found, can only send to each address once per send operation."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::TransactionCreationFailed:
        QMessageBox::warning(this, tr("Send Coins"),
            tr("Error: Transaction creation failed."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::TransactionCommitFailed:
        QMessageBox::warning(this, tr("Send Coins"),
            tr("Error: The transaction was rejected. This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::Aborted: // User aborted, nothing to do
        break;
    case WalletModel::OK:
        accept();
        break;
    }
    fNewRecipientAllowed = true;
}
