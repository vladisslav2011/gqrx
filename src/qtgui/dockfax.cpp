/*
 * fax gui widget implementation
 *
 * Copyright 2022 Marc CAPDEVILLE F4JMZ
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <QDebug>
#include <QScrollBar>
#include "dockfax.h"
#include "ui_dockfax.h"

DockFAX::DockFAX(QWidget *parent) :
            QDockWidget(parent),
            ui(new Ui::DockFAX) {

    ui->setupUi(this);
    d_scale=1;
    ui->scrollArea->setWidgetResizable(false);
    ui->view->setScaledContents(true);
    ui->zoom->addItem("1:16",0.0625);
    ui->zoom->addItem("1:8",0.125);
    ui->zoom->addItem("1:4",0.25);
    ui->zoom->addItem("1:2",0.5);
    ui->zoom->addItem("1:1",1.0);
    ui->zoom->addItem("2:1",2.0);
    ui->zoom->addItem("4:1",4.0);
    ui->zoom->addItem("8:1",8.0);
    ui->zoom->addItem("16:1",16.0);
    ui->zoom->setCurrentIndex(4);
}

DockFAX::~DockFAX() {
    delete ui;
}

void DockFAX::update_image(QImage& image) {
    int w,h,x,y;
    float z;
    bool follow = false;

    x = ui->scrollAreaWidgetContents->pos().x();
    y = ui->scrollAreaWidgetContents->pos().y() + ui->scrollAreaWidgetContents->height();

    if (y <= (ui->scrollArea->viewport()->height()))
        follow = true;

    if (ui->fittowindow->checkState()) {
        w = ui->scrollArea->viewport()->width();
        h = image.height() * w/image.width();
    } else {
        z = ui->zoom->currentData().toFloat();
        w = image.width() * z;
        h = image.height() * z;
    }

    ui->view->resize(w,h);
    ui->scrollAreaWidgetContents->resize(w,h);

    if (follow) {
        y = ui->scrollArea->viewport()->height() - h;
        ui->scrollAreaWidgetContents->move(x,y);
        ui->scrollArea->verticalScrollBar()->setValue(ui->scrollArea->verticalScrollBar()->maximum());
    }

    ui->view->setPixmap(QPixmap::fromImage(image));

}

void DockFAX::clearView() {
    ui->view->clear();
}

void DockFAX::show_Enabled() {
    if (!ui->faxEnable->isChecked()) {
        ui->faxEnable->blockSignals(true);
        ui->faxEnable->setChecked(true);
        ui->faxEnable->blockSignals(false);
    }
}

void DockFAX::show_Disabled() {
    if (ui->faxEnable->isChecked()) {
        ui->faxEnable->blockSignals(true);
        ui->faxEnable->setChecked(false);
        ui->faxEnable->blockSignals(false);
    }
}

void DockFAX::set_Disabled() {
    ui->faxEnable->setDisabled(true);
    ui->faxEnable->blockSignals(true);
    ui->faxEnable->setChecked(false);
    ui->faxEnable->blockSignals(false);
    ui->lpm->setDisabled(true);
    ui->ioc->setDisabled(true);
    ui->black_freq->setDisabled(true);
    ui->white_freq->setDisabled(true);
    ui->view->setDisabled(true);
    ui->reset->setDisabled(true);
    ui->sync->setDisabled(true);
    ui->start->setDisabled(true);
}

void DockFAX::set_Enabled() {
    ui->faxEnable->setDisabled(false);
    ui->lpm->setDisabled(false);
    ui->ioc->setDisabled(false);
    ui->black_freq->setDisabled(false);
    ui->white_freq->setDisabled(false);
    ui->view->setDisabled(false);
    ui->reset->setDisabled(false);
    ui->sync->setDisabled(false);
    ui->start->setDisabled(false);
}

float DockFAX::get_lpm() {
    return ui->lpm->value();
}

float DockFAX::get_ioc() {
    return ui->ioc->value();
}

float DockFAX::get_black_freq() {
    return ui->black_freq->value();
}

float DockFAX::get_white_freq() {
    return ui->white_freq->value();
}
void DockFAX::set_lpm(float lpm) {
    ui->lpm->setValue(lpm);
}

void DockFAX::set_ioc(float ioc) {
    ui->ioc->setValue(ioc);
}

void DockFAX::set_black_freq(float black_freq) {
    ui->black_freq->setValue(black_freq);
}

void DockFAX::set_white_freq(float white_freq) {
    ui->white_freq->setValue(white_freq);
}

/** Enable/disable FAX decoder */
void DockFAX::on_faxEnable_toggled(bool checked)
{
    if (checked)
        emit fax_start_decoder();
    else
        emit fax_stop_decoder();
}

void DockFAX::on_lpm_editingFinished() {
    if (ui->lpm->value()>0)
        emit fax_lpm_Changed(ui->lpm->value());
}

void DockFAX::on_ioc_editingFinished() {
    if (ui->ioc->value()>0)
        emit fax_ioc_Changed(ui->ioc->value());
}

void DockFAX::on_black_freq_editingFinished() {
    emit fax_black_freq_Changed(ui->black_freq->value());
}

void DockFAX::on_white_freq_editingFinished() {
    emit fax_white_freq_Changed(ui->white_freq->value());
}


void DockFAX::on_reset_clicked() {
    emit fax_reset_Clicked();
}

void DockFAX::on_start_clicked() {
    emit fax_start_Clicked();
}
void DockFAX::on_sync_clicked() {
    emit fax_sync_Clicked();
}

void DockFAX::on_save_clicked() {
    emit fax_save_Clicked();
}

bool DockFAX::autoSave() {
    return ui->autoSave->checkState();
}