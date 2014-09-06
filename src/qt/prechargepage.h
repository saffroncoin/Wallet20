#ifndef PRECHARGEPAGE_H
#define PRECHARGEPAGE_H

#include <QWidget>

namespace Ui {
class PrechargePage;
}
class ClientModel;
class WalletModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE


class PrechargePage : public QWidget
{
    Q_OBJECT

public:
    explicit PrechargePage(QWidget *parent = 0);
    ~PrechargePage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);

private:
    Ui::PrechargePage *ui;

    ClientModel *clientModel;
    WalletModel *walletModel;
};

#endif // PRECHARGEPAGE_H
