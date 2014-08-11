#ifndef SFRTESTPAGE_H
#define SFRTESTPAGE_H

#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QByteArray>

namespace Ui {
    class SFRTestPage;
}
class ClientModel;
class WalletModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE


/** SFRTest page widget */
class SFRTestPage : public QWidget
{
    Q_OBJECT

public:
    explicit SFRTestPage(QWidget *parent = 0);
    ~SFRTestPage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);

public slots:

signals:

private:
    Ui::SFRTestPage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;

private slots:
   void on_pushButtonSFRRecycler_clicked();
   void passRecyclerResponse(QNetworkReply *reply);
};

#endif // SFRTESTPAGE_H
