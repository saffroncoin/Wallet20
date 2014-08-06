#ifndef SFRPAYPAGE_H
#define SFRPAYPAGE_H

#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QByteArray>

namespace Ui {
    class SFRPayPage;
}
class ClientModel;
class WalletModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE


/** SFRPay page widget */
class SFRPayPage : public QWidget
{
    Q_OBJECT

public:
    explicit SFRPayPage(QWidget *parent = 0);
    ~SFRPayPage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);

public slots:

signals:

private:
    Ui::SFRPayPage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;

private slots:
   void on_pushButtonSFRPayBuy_clicked();
   void passSFRPayResponse(QNetworkReply *reply);
   void on_pushButtonSFRPayReset_clicked();
   void on_pushButtonRedeem_clicked();
   void passRedeemResponse(QNetworkReply *reply);
};

#endif // SFRPAYPAGE_H
