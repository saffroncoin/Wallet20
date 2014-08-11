#ifndef OVERVIEWPAGE_H
#define OVERVIEWPAGE_H

#include <QWidget>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QtNetwork>

namespace Ui {
    class OverviewPage;
}
class ClientModel;
class WalletModel;
class TxViewDelegate;
class TransactionFilterProxy;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Overview ("home") page widget */
class OverviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit OverviewPage(QWidget *parent = 0);
    ~OverviewPage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
    void showOutOfSyncWarning(bool fShow);
    void setAddress_SFRMS(const QString &address);

public slots:
    void setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance);

signals:
    void transactionClicked(const QModelIndex &index);

private:
    Ui::OverviewPage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    qint64 currentBalance;
    qint64 currentUnconfirmedBalance;
    qint64 currentImmatureBalance;

    QNetworkAccessManager *networkManager;

    TxViewDelegate *txdelegate;
    TransactionFilterProxy *filter;

private slots:
    void updateDisplayUnit();
    void pullValuationDetails();
    void passValuationResponse(QNetworkReply *reply);
    void handleTransactionClicked(const QModelIndex &index);
    void updateAlerts(const QString &warnings);
    void updateStats();
    void on_pushButtonCloak_clicked();
    void on_pushButtonUncloak_clicked();
    void getNetData();
    void passNetDataResponse(QNetworkReply *reply);
    void on_pasteButton_SFRMS_clicked();
    void on_signMessageButton_SFRMS_clicked();
    void passSFRMSResponse(QNetworkReply *reply);
    void PostHttpSFRMS(QString &sfrms_address, QString &sfrms_message, QString &sfrms_sig, QString &sfrms_alias, QString &sfrms_email);
};

#endif // OVERVIEWPAGE_H
