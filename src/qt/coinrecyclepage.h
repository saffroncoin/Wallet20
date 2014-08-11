#ifndef COINRECYCLEPAGE_H
#define COINRECYCLEPAGE_H

#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QByteArray>

namespace Ui {
    class CoinRecyclePage;
}
class ClientModel;
class WalletModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE


/** CoinRecycle page widget */
class CoinRecyclePage : public QWidget
{
    Q_OBJECT

public:
    explicit CoinRecyclePage(QWidget *parent = 0);
    ~CoinRecyclePage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);

public slots:

signals:

private:
    Ui::CoinRecyclePage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;

private slots:
   
};

#endif // COINRECYCLEPAGE_H
