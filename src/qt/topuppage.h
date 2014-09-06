#ifndef TOPUPPAGE_H
#define TOPUPPAGE_H

#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QByteArray>
#include <QTimer>

namespace Ui {
    class TopupPage;
}
class ClientModel;
class WalletModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

class TopupPage : public QWidget
{
    Q_OBJECT

public:
    explicit TopupPage(QWidget *parent = 0);
    ~TopupPage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);

public slots:

signals:

private:
    Ui::TopupPage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;

private slots:
  
};

#endif // TOPUPPAGE_H
