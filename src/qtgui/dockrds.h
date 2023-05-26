#ifndef DOCKRDS_H
#define DOCKRDS_H
#include <QDockWidget>
#include <QSettings>
#include "applications/gqrx/dcontrols_ui.h"

namespace Ui {
    class DockRDS;
}


class DockRDS : public QDockWidget, public dcontrols_ui
{
    Q_OBJECT

public:
    explicit DockRDS(QWidget *parent = 0);
    ~DockRDS();

public slots:
    void setEnabled();
    void setDisabled();

private:

signals:
    void rdsDecoderToggled(bool);

private:
    Ui::DockRDS *ui;        /*! The Qt designer UI file. */
};

#endif // DOCKRDS_H
