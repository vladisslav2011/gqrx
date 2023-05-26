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
