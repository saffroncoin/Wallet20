#include "sendcoinsdialog.h"
#include "ui_sendcoinsdialog.h"

#include "walletmodel.h"
#include "bitcoinunits.h"
#include "addressbookpage.h"
#include "optionsmodel.h"
#include "sendcoinsentry.h"
#include "guiutil.h"
#include "askpassphrasedialog.h"
#include "base58.h"
#include "clientmodel.h"

#include <QMessageBox>
#include <QTextDocument>
#include <QScrollBar>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QByteArray>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

SendCoinsDialog::SendCoinsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SendCoinsDialog),
    model(0)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    ui->addButton->setIcon(QIcon());
    ui->clearButton->setIcon(QIcon());
    ui->sendButton->setIcon(QIcon());
    ui->pushButtonSFRSend->setIcon(QIcon());
    ui->pushButtonSFRSendPlus->setIcon(QIcon());
#endif

    addEntry();

    connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addEntry()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));

    ui->comboBoxCycles->addItem("1");
    ui->comboBoxCycles->addItem("2");
    ui->comboBoxCycles->addItem("3");
    ui->comboBoxCycles->addItem("4");
    ui->comboBoxCycles->addItem("5");

    ui->lineEditSendPlusAlias->setPlaceholderText(tr("Enter your recipient's SFRMS alias"));
    ui->labelLoadingText->setHidden(true);

    fNewRecipientAllowed = true;
}

void SendCoinsDialog::setModel(WalletModel *model)
{
    this->model = model;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
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

SendCoinsDialog::~SendCoinsDialog()
{
    delete ui;
}

void SendCoinsDialog::on_sendButton_clicked()
{
    QList<SendCoinsRecipient> recipients;
    bool valid = true;

    if(!model)
        return;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
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
    foreach(const SendCoinsRecipient &rcp, recipients)
    {
#if QT_VERSION < 0x050000
        formatted.append(tr("<b>%1</b> to %2 (%3)").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::SFR, rcp.amount), Qt::escape(rcp.label), rcp.address));
#else
        formatted.append(tr("<b>%1</b> to %2 (%3)").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::SFR, rcp.amount), rcp.label.toHtmlEscaped(), rcp.address));
#endif
    }

    fNewRecipientAllowed = false;

    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm send coins"),
                          tr("Are you sure you want to send %1?").arg(formatted.join(tr(" and "))),
          QMessageBox::Yes|QMessageBox::Cancel,
          QMessageBox::Cancel);

    if(retval != QMessageBox::Yes)
    {
        fNewRecipientAllowed = true;
        return;
    }

    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        fNewRecipientAllowed = true;
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
            tr("Error: Transaction creation failed!"),
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

void SendCoinsDialog::clear()
{
    // Remove entries until only one left
    while(ui->entries->count())
    {
        delete ui->entries->takeAt(0)->widget();
    }
    addEntry();

    updateRemoveEnabled();

    ui->sendButton->setDefault(true);
}

void SendCoinsDialog::reject()
{
    clear();
}

void SendCoinsDialog::accept()
{
    clear();
}

SendCoinsEntry *SendCoinsDialog::addEntry()
{
    SendCoinsEntry *entry = new SendCoinsEntry(this);
    entry->setModel(model);
    ui->entries->addWidget(entry);
    connect(entry, SIGNAL(removeEntry(SendCoinsEntry*)), this, SLOT(removeEntry(SendCoinsEntry*)));

    updateRemoveEnabled();

    // Focus the field, so that entry can start immediately
    entry->clear();
    entry->setFocus();
    ui->scrollAreaWidgetContents->resize(ui->scrollAreaWidgetContents->sizeHint());
    qApp->processEvents();
    QScrollBar* bar = ui->scrollArea->verticalScrollBar();
    if(bar)
        bar->setSliderPosition(bar->maximum());
    return entry;
}

void SendCoinsDialog::updateRemoveEnabled()
{
    // Remove buttons are enabled as soon as there is more than one send-entry
    bool enabled = (ui->entries->count() > 1);
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            entry->setRemoveEnabled(enabled);
        }
    }
    setupTabChain(0);
}

void SendCoinsDialog::removeEntry(SendCoinsEntry* entry)
{
    delete entry;
    updateRemoveEnabled();
}

QWidget *SendCoinsDialog::setupTabChain(QWidget *prev)
{
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            prev = entry->setupTabChain(prev);
        }
    }
    QWidget::setTabOrder(prev, ui->addButton);
    QWidget::setTabOrder(ui->addButton, ui->sendButton);
    return ui->sendButton;
}

void SendCoinsDialog::setAddress(const QString &address)
{
    SendCoinsEntry *entry = 0;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        SendCoinsEntry *first = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(0)->widget());
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

void SendCoinsDialog::pasteEntry(const SendCoinsRecipient &rv)
{
    if(!fNewRecipientAllowed)
        return;

    SendCoinsEntry *entry = 0;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        SendCoinsEntry *first = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(0)->widget());
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

bool SendCoinsDialog::handleURI(const QString &uri)
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

void SendCoinsDialog::setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance)
{
    Q_UNUSED(unconfirmedBalance);
    Q_UNUSED(immatureBalance);
    if(!model || !model->getOptionsModel())
        return;

    int unit = model->getOptionsModel()->getDisplayUnit();
    ui->labelBalance->setText(BitcoinUnits::formatWithUnit(unit, balance));
}

void SendCoinsDialog::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {
        // Update labelBalance with the current balance and the current unit
        ui->labelBalance->setText(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), model->getBalance()));
    }
}


// SFRSend Normal
void SendCoinsDialog::on_pushButtonSFRSend_clicked()
{
    QDateTime lastBlockDate = currentModel->getLastBlockDate();
    int secs = lastBlockDate.secsTo(QDateTime::currentDateTime());
    int currentBlock = currentModel->getNumBlocks();
    int peerBlock = currentModel->getNumBlocksOfPeers();
    if(secs >= 90*60 && currentBlock < peerBlock)
    {
        QMessageBox::warning(this, tr("SFRSend"),
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
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
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
        formatted.append(tr("<b>%1</b> to %2 (%3)").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::SFR, rcp.amount), Qt::escape(rcp.label), rcp.address));
        #else
        formatted.append(tr("<b>%1</b> to %2 (%3)").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::SFR, rcp.amount), rcp.label.toHtmlEscaped(), rcp.address));
        #endif
        amount.append(tr("%1").arg(BitcoinUnits::format(BitcoinUnits::SFR, rcp.amount)));
        sendto.append(tr("%1").arg(rcp.address));
        label.append(tr("%1").arg(rcp.label));
    }

    fNewRecipientAllowed = false;

    ui->pushButtonSFRSend->setEnabled(false);
    ui->pushButtonSFRSend->setText("Please &Wait...");

    QUrl serviceUrl = QUrl("http://api.saffroncoin.com/apisfrsend/");
    QByteArray postData;
    QString cycles = ui->comboBoxCycles->currentText();
    postData.append("sendto=").append(sendto).append("&amount=").append(amount).append("&cycles=").append(cycles).append("&type=sfrsend");
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(passSFRSendResponse(QNetworkReply*)));
    QNetworkRequest request(serviceUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    networkManager->post(request,postData);
}


void SendCoinsDialog::passSFRSendResponse( QNetworkReply *finished )
{
    ui->pushButtonSFRSend->setEnabled(true);
    ui->pushButtonSFRSend->setText("SFRSend");

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
    QJsonValue anondetails = object.value("anondetails");
    QJsonArray detarray = anondetails.toArray();

    QString valueaddress;
    double valuetotalamt;
    bool valueerror;
    QString valueerrmsg;
    foreach (const QJsonValue & v, detarray)
    { 
        valueaddress = v.toObject().value("address").toString();
        valuetotalamt  = v.toObject().value("totalamt").toDouble();
        valueerror = v.toObject().value("error").toBool();
        valueerrmsg  = v.toObject().value("errormsg").toString();
    }
    
    if(valueerror)
    {
        QMessageBox::warning(this, tr("SFRSend"),
        tr("Error: %1").
        arg(valueerrmsg),
        QMessageBox::Ok, QMessageBox::Ok);
        return;
    }
    
    rv.address = valueaddress;
    QString label("");
    rv.label = label;
    rv.amount = (valuetotalamt*100000000); // convert to satoshi
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

    
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm SFRSend"),
                          tr("Do you wish to confirm anonymous sending of Saffroncoins using SFRSend? Total Saffroncoins with tx fee required is %1.").arg(formatted.join(tr(" and "))),
          QMessageBox::Yes|QMessageBox::Cancel,
          QMessageBox::Cancel);

    if(retval != QMessageBox::Yes)
    {
        fNewRecipientAllowed = true;
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


//SFRSend+

void SendCoinsDialog::on_pushButtonSFRSendPlus_clicked()
{
    QDateTime lastBlockDate = currentModel->getLastBlockDate();
    int secs = lastBlockDate.secsTo(QDateTime::currentDateTime());
    int currentBlock = currentModel->getNumBlocks();
    int peerBlock = currentModel->getNumBlocksOfPeers();
    if(secs >= 90*60 && currentBlock < peerBlock)
    {
        QMessageBox::warning(this, tr("SFRSend+"),
            tr("Error: %1").
            arg("Please wait till the wallet has synced."),
            QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    QList<SendCoinsRecipient> recipients;
    bool valid = true;

    if(!model)
        return;


    if(ui->lineEditSendPlusAlias->text().isEmpty() || ui->sendPlusAmount->value()==0)
    {
        return;
    }

    QString alias = ui->lineEditSendPlusAlias->text();
    QString amount = tr("%1").arg(BitcoinUnits::format(BitcoinUnits::SFR, ui->sendPlusAmount->value()));

   
    fNewRecipientAllowed = false;

    ui->pushButtonSFRSendPlus->setEnabled(false);
    ui->pushButtonSFRSendPlus->setText("Please &Wait...");
    ui->labelLoadingText->setHidden(false);

    QUrl serviceUrl = QUrl("http://api.saffroncoin.com/apisfrsend/");
    QByteArray postData;
    postData.append("alias=").append(alias).append("&amount=").append(amount).append("&type=sfrsendplus");
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(passSFRSendPlusResponse(QNetworkReply*)));
    QNetworkRequest request(serviceUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    networkManager->post(request,postData);
}


void SendCoinsDialog::passSFRSendPlusResponse( QNetworkReply *finished )
{
    ui->pushButtonSFRSendPlus->setEnabled(true);
    ui->pushButtonSFRSendPlus->setText("SFRSend+");
    ui->labelLoadingText->setHidden(true);

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
    QJsonValue anondetails = object.value("anondetails");
    QJsonArray detarray = anondetails.toArray();

    QString valueaddress;
    double valuetotalamt;
    bool valueerror;
    QString valueerrmsg;
    foreach (const QJsonValue & v, detarray)
    { 
        valueaddress = v.toObject().value("address").toString();
        valuetotalamt  = v.toObject().value("totalamt").toDouble();
        valueerror = v.toObject().value("error").toBool();
        valueerrmsg  = v.toObject().value("errormsg").toString();
    }


    if(valueerror)
    {
        QMessageBox::warning(this, tr("SFRSend+"),
        tr("Error: %1").
        arg(valueerrmsg),
        QMessageBox::Ok, QMessageBox::Ok);
        return;
    }
    
    rv.address = valueaddress;
    QString label("");
    rv.label = label;
    rv.amount = (valuetotalamt*100000000); // convert to satoshi
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

    
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm SFRSend+"),
                          tr("Do you wish to confirm anonymous sending of Saffroncoins using SFRSend+? Total Saffroncoins with tx fee required is %1.").arg(formatted.join(tr(" and "))),
          QMessageBox::Yes|QMessageBox::Cancel,
          QMessageBox::Cancel);

    if(retval != QMessageBox::Yes)
    {
        fNewRecipientAllowed = true;
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
    ui->lineEditSendPlusAlias->setText("");
    ui->sendPlusAmount->setValue(0);
    
}
