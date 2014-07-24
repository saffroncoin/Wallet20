#ifndef IRCPAGE_H
#define IRCPAGE_H

#include <QWidget>


#include <QtNetwork>

namespace Ui {
    class IRCPage;
}
class ClientModel;
class WalletModel;
class TxViewDelegate;
class TransactionFilterProxy;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** IRC #SaffronCoin page widget */
class IRCPage : public QWidget
{
    Q_OBJECT

public:
    explicit IRCPage(QWidget *parent = 0);
    ~IRCPage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
    void showOutOfSyncWarning(bool fShow);

public slots:
    //void setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance);

//signals:
  //  void transactionClicked(const QModelIndex &index);

private:
    Ui::IRCPage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    //qint64 currentBalance;
    //qint64 currentUnconfirmedBalance;
    //qint64 currentImmatureBalance;

    //TxViewDelegate *txdelegate;
    //TransactionFilterProxy *filter;

private slots:
    //void updateDisplayUnit();
    //void handleTransactionClicked(const QModelIndex &index);
    //void updateAlerts(const QString &warnings);
};

#endif // IRCPAGE_H
