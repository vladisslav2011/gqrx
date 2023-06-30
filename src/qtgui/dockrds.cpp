/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2013 Alexandru Csete OZ9AEC.
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
#include <QDebug>
#include <QVariant>
#include "dockrds.h"
#include "ui_dockrds.h"

DockRDS::DockRDS(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockRDS)
{
    ui->setupUi(this);

    ui->scrollArea->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);
    grid_init(ui->gridLayout,ui->gridLayout->rowCount(),0/*ui->gridLayout->columnCount()*/);
    set_observer(C_RDS_BIT_ERRORS, &DockRDS::bitErrorsObserver);
}

DockRDS::~DockRDS()
{
    delete ui;
}


void DockRDS::setDisabled()
{
    getWidget(C_RDS_ON)->setDisabled(true);
}

void DockRDS::setEnabled()
{
    getWidget(C_RDS_ON)->setDisabled(false);
}

void DockRDS::bitErrorsObserver(c_id, const c_def::v_union & v)
{
    int n_errors = v;
    QString style="color:#7f0000;";
    switch(n_errors)
    {
    case 0:
        style="color:#007f00;";
    break;
    case 1:
        style="color:#7f7f00;";
    break;
    case 2:
        style="color:#7f5f00;";
    break;
    case 3:
        style="color:#7f3000;";
    break;
    case 4:
        style="color:#7f1f00;";
    break;
    }
    getWidget(C_RDS_PI)->setStyleSheet(style);
}
