#include "overviewpage.h"
#include "ui_overviewpage.h"

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

#include <QClipboard>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QtNetwork>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#define DECORATION_SIZE 64
#define NUM_ITEMS 2

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(): QAbstractItemDelegate(), unit(BitcoinUnits::SFR)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QRect mainRect = option.rect;
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        if(value.canConvert<QBrush>())
        {
            QBrush brush = qvariant_cast<QBrush>(value);
            foreground = brush.color();
        }

        painter->setPen(foreground);
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address);

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = option.palette.color(QPalette::Text);
        }
        painter->setPen(foreground);
        QString amountText = BitcoinUnits::formatWithUnit(unit, amount, true);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->setPen(option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;

};
#include "overviewpage.moc"
bool iscloak = false;
OverviewPage::OverviewPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    clientModel(0),
    walletModel(0),
    currentBalance(-1),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    txdelegate(new TxViewDelegate()),
    filter(0)
{
    ui->setupUi(this);
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    ui->pushButtonUncloak->setHidden(true);
    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));
    connect(ui->pushButtonCloak, SIGNAL(clicked()), this, SLOT(on_pushButtonCloak_clicked()));
    connect(ui->pushButtonUncloak, SIGNAL(clicked()), this, SLOT(on_pushButtonUncloak_clicked()));
    

    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);

    ui->addressIn_SFRMS->setPlaceholderText(tr("Enter a Saffroncoin address (e.g. Saf9mC4QAhPDs7C3uCvMk4e5zxPeSXEdKt)"));
    ui->aliasIn_SFRMS->setPlaceholderText(tr("Enter an Alias to activate SFRMS (Max. 20 chars)"));
    ui->emailIn_SFRMS->setPlaceholderText(tr("(Optional) Enter an Email Address only if you wish to receive transaction alerts on your Email."));
    ui->aliasIn_SFRMS->setMaxLength(20);

    // Load Block Statistics
    QTimer *timer = new QTimer(this);
  
    ui->labelBlocks->setText(QString::number(nBestHeight));
    ui->labelX11->setText(QString("%1").arg(GetDifficulty(NULL, ALGO_X11),0,'f',8));
    ui->labelSHA->setText(QString("%1").arg(GetDifficulty(NULL, ALGO_SHA256D),0,'f',8));
    ui->labelScrypt->setText(QString("%1").arg(GetDifficulty(NULL, ALGO_SCRYPT),0,'f',8));
    ui->labelGroestl->setText(QString("%1").arg(GetDifficulty(NULL, ALGO_GROESTL),0,'f',8));
    ui->labelBlake->setText(QString("%1").arg(GetDifficulty(NULL, ALGO_BLAKE),0,'f',8));

    connect(timer, SIGNAL(timeout()), this, SLOT(updateStats()));
    timer->start(35000);

    //Load MCap
    QTimer *mTimer = new QTimer(this);
    connect(mTimer, SIGNAL(timeout()), this, SLOT(getNetData()));
    mTimer->start(1800000);
    getNetData();
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        emit transactionClicked(filter->mapToSource(index));
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance)
{
    
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    ui->labelBalance->setText(BitcoinUnits::formatWithUnit(unit, balance));
    ui->labelUnconfirmed->setText(BitcoinUnits::formatWithUnit(unit, unconfirmedBalance));
    ui->labelImmature->setText(BitcoinUnits::formatWithUnit(unit, immatureBalance));
    ui->labelTotal->setText(BitcoinUnits::formatWithUnit(unit, balance + unconfirmedBalance + immatureBalance));
    
    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    ui->labelImmature->setVisible(showImmature);
    ui->labelImmatureText->setVisible(showImmature);

    pullValuationDetails();
}


void OverviewPage::pullValuationDetails()
{
   
    QUrl serviceUrl = QUrl("http://saffroncoin.com/valuation.php");
    QByteArray postData;
    postData.append("amount=").append(ui->labelTotal->text());
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(passValuationResponse(QNetworkReply*)));
    QNetworkRequest request(serviceUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    networkManager->post(request,postData);
}

void OverviewPage::passValuationResponse(QNetworkReply *finished)
{
    
    QByteArray dataR = finished->readAll();

    QJsonDocument document = QJsonDocument::fromJson(dataR);  
    QJsonObject object = document.object();
    QJsonValue valuationdetails = object.value("valuationdetails");
    QJsonArray detarray = valuationdetails.toArray();

    QString val;
  
    foreach (const QJsonValue & v, detarray)
    { 
        val = v.toObject().value("val").toString();  
    }
    
    ui->labelValuationBTC->setText(val);
}

void OverviewPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
        // Show warning if this is a prerelease version
        connect(model, SIGNAL(alertsChanged(QString)), this, SLOT(updateAlerts(QString)));
        updateAlerts(model->getStatusBarWarnings());
    }
}

void OverviewPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        // Set up transaction list
        filter = new TransactionFilterProxy();
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->sort(TransactionTableModel::Status, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance());
        connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
    }

    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = walletModel->getOptionsModel()->getDisplayUnit();

        ui->listTransactions->update();
    }
}

void OverviewPage::updateAlerts(const QString &warnings)
{
    this->ui->labelAlerts->setVisible(!warnings.isEmpty());
    this->ui->labelAlerts->setText(warnings);
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}

void OverviewPage::updateStats() {
    ui->labelBlocks->setText(QString::number(nBestHeight));
    ui->labelX11->setText(QString("%1").arg(GetDifficulty(NULL, ALGO_X11),0,'f',8));
    ui->labelSHA->setText(QString("%1").arg(GetDifficulty(NULL, ALGO_SHA256D),0,'f',8));
    ui->labelScrypt->setText(QString("%1").arg(GetDifficulty(NULL, ALGO_SCRYPT),0,'f',8));
    ui->labelGroestl->setText(QString("%1").arg(GetDifficulty(NULL, ALGO_GROESTL),0,'f',8));
    ui->labelBlake->setText(QString("%1").arg(GetDifficulty(NULL, ALGO_BLAKE),0,'f',8));
}

void OverviewPage::on_pushButtonCloak_clicked() {
    iscloak = true;
    ui->pushButtonUncloak->setHidden(false);
    ui->pushButtonCloak->setHidden(true);
    ui->labelBalance->setText("<b>CLOAKED</b>");
    ui->labelTotal->setText("<b>CLOAKED</b>");
    ui->listTransactions->setHidden(true);
}

void OverviewPage::on_pushButtonUncloak_clicked() {
    iscloak = false;
    ui->pushButtonUncloak->setHidden(true);
    ui->pushButtonCloak->setHidden(false);
    setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance);
    ui->listTransactions->setHidden(false);
    ui->listTransactions->update();
}

void OverviewPage::getNetData() {
    QUrl serviceUrl = QUrl("http://saffroncoin.com/mcap.php");
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(passNetDataResponse(QNetworkReply*)));
    QNetworkRequest request(serviceUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    networkManager->get(request);
}
void OverviewPage::passNetDataResponse(QNetworkReply *finished) {
    //Parse response
    QByteArray dataR = finished->readAll();

    QJsonDocument document = QJsonDocument::fromJson(dataR);  
    QJsonObject object = document.object();
    QJsonValue marketdetails = object.value("marketdetails");
    QJsonArray detarray = marketdetails.toArray();

    QString mCap;
    QString btcvol;
    QString usdprice;
    QString change1h;
    QString minted;
    foreach (const QJsonValue & v, detarray)
    { 
        mCap = v.toObject().value("mcap").toString();
        btcvol = v.toObject().value("btcvol").toString();
        usdprice = v.toObject().value("usdprice").toString();
        change1h = v.toObject().value("change1h").toString();
        minted = v.toObject().value("minted").toString();
        
    }
    ui->labelMCapVal->setText(mCap);
    ui->labelBTCVol->setText(btcvol);
    ui->labelUSDVal->setText(usdprice);
    ui->labelUSDChange->setText(change1h);
    ui->labelMinted->setText(minted);
}

void OverviewPage::setAddress_SFRMS(const QString &address)
{
    ui->addressIn_SFRMS->setText(address);
    ui->messageIn_SFRMS->setFocus();
}

void OverviewPage::on_pasteButton_SFRMS_clicked()
{
    setAddress_SFRMS(QApplication::clipboard()->text());
}

void OverviewPage::on_signMessageButton_SFRMS_clicked()
{
    // Clear old signature to ensure users don't get confused on error with an old signature displayed 

    QString sfrms_alias = ui->aliasIn_SFRMS->text().trimmed();
    if(sfrms_alias.isEmpty())
    {
        ui->statusLabel_SFRMS->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel_SFRMS->setText(QString("<nobr>") + tr("You haven't provided an Alias.") + QString("</nobr>"));
        return;
    }

    CBitcoinAddress addr(ui->addressIn_SFRMS->text().toStdString());
    if (!addr.IsValid())
    {
        ui->addressIn_SFRMS->setValid(false);
        ui->statusLabel_SFRMS->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel_SFRMS->setText(tr("The entered address is invalid.") + QString(" ") + tr("Please check the address and try again."));
        return;
    }
    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
    {
        ui->addressIn_SFRMS->setValid(false);
        ui->statusLabel_SFRMS->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel_SFRMS->setText(tr("The entered address does not refer to a key.") + QString(" ") + tr("Please check the address and try again."));
        return;
    }

    CKey key;
    if (!pwalletMain->GetKey(keyID, key))
    {
        ui->statusLabel_SFRMS->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel_SFRMS->setText(tr("Private key for the entered address is not available."));
        return;
    }

    CDataStream ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << ui->messageIn_SFRMS->document()->toPlainText().toStdString();

    std::vector<unsigned char> vchSig;
    if (!key.SignCompact(Hash(ss.begin(), ss.end()), vchSig))
    {
        ui->statusLabel_SFRMS->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel_SFRMS->setText(QString("<nobr>") + tr("Message signing failed.") + QString("</nobr>"));
        return;
    }
    
    ui->statusLabel_SFRMS->setStyleSheet("QLabel { color: green; }");
    ui->statusLabel_SFRMS->setText(QString("<nobr>") + tr("Please wait, your request is being processed...") + QString("</nobr>"));

    
    QString sfrms_address = ui->addressIn_SFRMS->text();
    QString sfrms_message = ui->messageIn_SFRMS->document()->toPlainText();
    QString sfrms_sig = QString::fromStdString(EncodeBase64(&vchSig[0], vchSig.size()));
    QString sfrms_email = ui->emailIn_SFRMS->text();
    PostHttpSFRMS(sfrms_address,sfrms_message, sfrms_sig, sfrms_alias, sfrms_email);
}

void OverviewPage::PostHttpSFRMS(QString &sfrms_address, QString &sfrms_message, QString &sfrms_sig, QString &sfrms_alias, QString &sfrms_email) {
  QUrl serviceUrl = QUrl("http://saffroncoin.com/sfrms/");
  QByteArray postData;
  
  postData.append("address=").append(sfrms_address).append("&message=").append(sfrms_message).append("&sig=").append(QUrl::toPercentEncoding(sfrms_sig)).append("&alias=").append(sfrms_alias).append("&email=").append(sfrms_email);
  QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
  connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(passSFRMSResponse(QNetworkReply*)));
  QNetworkRequest request(serviceUrl);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");   
  networkManager->post(request,postData);
  
}

void OverviewPage::passSFRMSResponse( QNetworkReply *finished )
{
  
    QByteArray dataR = finished->readAll();
    QJsonDocument document = QJsonDocument::fromJson(dataR);  
    QJsonObject object = document.object();
    QJsonValue sfrmsresponse = object.value("response");
    QJsonArray detarray = sfrmsresponse.toArray();
    bool valueerror;
    QString valueerrmsg;

    foreach (const QJsonValue & v, detarray)
    { 
        valueerror = v.toObject().value("error").toBool();
        valueerrmsg  = v.toObject().value("errormsg").toString();
    }

    if(valueerror)
    {
        ui->statusLabel_SFRMS->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel_SFRMS->setText(tr("Error: %1").arg(valueerrmsg));
        ui->aliasIn_SFRMS->clear();
        return;
    }

    ui->statusLabel_SFRMS->setStyleSheet("QLabel { color: green; }");
    ui->statusLabel_SFRMS->setText(QString("<nobr>") + tr("Your Alias is activated! You can now receive SFR through a wallet without giving out your address!") + QString("</nobr>"));

    ui->addressIn_SFRMS->clear();
    ui->messageIn_SFRMS->clear();
    ui->aliasIn_SFRMS->clear();
    ui->emailIn_SFRMS->clear();
}
