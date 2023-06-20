/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2020 Oliver Grossmann.
 *
 * Gqrx is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * Gqrx is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Gqrx; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#ifndef DXC_OPTIONS_H
#define DXC_OPTIONS_H


#include <QCloseEvent>
#include <QShowEvent>
#include <QTcpSocket>
#include <QSettings>

#include <QDialog>
#include "applications/gqrx/dcontrols_ui.h"

namespace Ui {
class DXCOptions;
}

class DXCOptions : public QDialog, public dcontrols_ui
{
    Q_OBJECT

public:
    explicit DXCOptions(QWidget *parent = 0);
    ~DXCOptions();

    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent * event) override;

private slots:

    void connected();
    void disconnected();
    void readyToRead();
private:
    void addressObserver(c_id, const c_def::v_union & v){ DXCAddress=QString::fromStdString(v);}
    void portObserver(c_id, const c_def::v_union & v){ DXCPort=v;}
    void usernameObserver(c_id, const c_def::v_union & v){ DXCUsername=QString::fromStdString(v);}
    void timeoutObserver(c_id, const c_def::v_union & v){ DXCSpotTimeout=v;}
    void filterObserver(c_id, const c_def::v_union & v){ DXCFilter=QString::fromStdString(v);}
    void connectObserver(c_id, const c_def::v_union & v);
    void disconnectObserver(c_id, const c_def::v_union & v);
    void finalizeInner() override;

private:
    Ui::DXCOptions *ui;
    QTcpSocket *m_socket;
    QString DXCAddress{"127.0.0.1"};
    int DXCPort{7300};
    QString DXCUsername{"nocall"};
    int DXCSpotTimeout{10};
    QString DXCFilter{""};
};

#endif // DXC_OPTIONS_H
