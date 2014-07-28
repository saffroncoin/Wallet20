#ifndef TRADEPAGE_H
#define TRADEPAGE_H

#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QByteArray>
#include <QTimer>

namespace Ui {
    class TradePage;
}
class ClientModel;
class WalletModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Trade page widget */
class TradePage : public QWidget
{
    Q_OBJECT

public:
    explicit TradePage(QWidget *parent = 0);
    ~TradePage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);

public slots:

signals:

private:
    Ui::TradePage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;

private slots:
    void LoadBittrexWebview();
    void LoadCryptsyWebview();
    void LoadMintpalWebview();
    void LoadBittrexWebviewBottom();
    void LoadCryptsyWebviewBottom();
    void LoadMintpalWebviewBottom();
};

#endif // TRADEPAGE_H
