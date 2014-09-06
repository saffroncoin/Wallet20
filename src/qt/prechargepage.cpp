#include "prechargepage.h"
#include "ui_prechargepage.h"

#include "clientmodel.h"
#include "walletmodel.h"
#include "optionsmodel.h"
#include "guiutil.h"
#include "guiconstants.h"

#include <QAbstractItemDelegate>
#include <QPainter>


#include "prechargepage.moc"

PrechargePage::PrechargePage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PrechargePage),
    clientModel(0)
{
    ui->setupUi(this);
}

PrechargePage::~PrechargePage()
{
    delete ui;
}

void PrechargePage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
   
}