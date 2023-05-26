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
    void updateRDS(QString text, int type);
    void showEnabled();
    void showDisabled();
    void setEnabled();
    void setDisabled();
    void setRDSmode(bool cmd);

private:
    void ClearTextFields();

signals:
    void rdsDecoderToggled(bool);
    void rdsPI(QString text);

private slots:
    void on_rdsCheckbox_clicked(bool checked);

private:
    Ui::DockRDS *ui;        /*! The Qt designer UI file. */
};

#endif // DOCKRDS_H
