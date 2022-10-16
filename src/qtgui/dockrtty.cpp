/*
 * rtty gui widget implementation
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
#include "dockrtty.h"
#include "ui_dockrtty.h"

DockRTTY::DockRTTY(QWidget *parent) :
            QDockWidget(parent),
            ui(new Ui::DockRTTY) {
    ui->setupUi(this);
    ui->mode->addItem(QString("5 bits baudot"));
    ui->mode->addItem(QString("7 bits ascii"));
    ui->mode->addItem(QString("8 bits ascii"));

    ui->parity->addItem(QString("none"));
    ui->parity->addItem(QString("odd"));
    ui->parity->addItem(QString("even"));
    ui->parity->addItem(QString("mark"));
    ui->parity->addItem(QString("space"));
    ui->parity->addItem(QString("dont care"));
}

DockRTTY::~DockRTTY() {
    delete ui;
}

void DockRTTY::update_text(QString text) {
    ui->text->moveCursor(QTextCursor::End);
        ui->text->insertPlainText(text);
}

QString DockRTTY::get_text() {
    return ui->text->toPlainText();
}


void DockRTTY::ClearText() {
    ui->text->setText("");
}

void DockRTTY::show_Enabled() {
    if (!ui->rttyEnable->isChecked()) {
        ui->rttyEnable->blockSignals(true);
        ui->rttyEnable->setChecked(true);
        ui->rttyEnable->blockSignals(false);
    }
}

void DockRTTY::show_Disabled() {
    if (ui->rttyEnable->isChecked()) {
        ui->rttyEnable->blockSignals(true);
        ui->rttyEnable->setChecked(false);
        ui->rttyEnable->blockSignals(false);
    }
}

void DockRTTY::set_Disabled() {
    ui->rttyEnable->setDisabled(false);
    ui->rttyEnable->blockSignals(true);
    ui->rttyEnable->setChecked(false);
    ui->rttyEnable->blockSignals(false);
    ui->baud_rate->setDisabled(true);
    ui->mark_freq->setDisabled(true);
    ui->space_freq->setDisabled(true);
    ui->text->setDisabled(true);
    ui->mode->setDisabled(true);
}

void DockRTTY::set_Enabled() {
    ui->rttyEnable->setDisabled(false);
    ui->baud_rate->setDisabled(false);
    ui->mark_freq->setDisabled(false);
    ui->space_freq->setDisabled(false);
    ui->text->setDisabled(false);
    ui->mode->setDisabled(false);
}

void DockRTTY::set_baud_rate(float baud_rate) {
    ui->baud_rate->setValue(baud_rate);
}

void DockRTTY::set_mark_freq(float mark_freq) {
    ui->mark_freq->setValue(mark_freq);
}

void DockRTTY::set_space_freq(float space_freq) {
    ui->space_freq->setValue(space_freq);
}

/** Enable/disable RTTY decoder */
void DockRTTY::on_rttyEnable_toggled(bool checked) {
    if (checked)
        emit rtty_start_decoder();
    else
        emit rtty_stop_decoder();
}

void DockRTTY::on_baud_rate_editingFinished() {
    if (ui->baud_rate->value()>0)
        emit rtty_baud_rate_Changed(ui->baud_rate->value());
}

void DockRTTY::on_mark_freq_editingFinished() {
    emit rtty_mark_freq_Changed(ui->mark_freq->value());
}

void DockRTTY::on_space_freq_editingFinished() {
    emit rtty_space_freq_Changed(ui->space_freq->value());
}

void DockRTTY::on_mode_currentIndexChanged(int mode) {
    emit rtty_mode_Changed(mode);
}

void DockRTTY::on_parity_currentIndexChanged(int parity) {
    emit rtty_parity_Changed(parity);
}

float DockRTTY::get_baud_rate() {
    return ui->baud_rate->value();
}

float DockRTTY::get_mark_freq() {
    return ui->mark_freq->value();
}

float DockRTTY::get_space_freq() {
    return ui->space_freq->value();
}

int   DockRTTY::get_mode() {
    return ui->mode->currentIndex();
}

int   DockRTTY::get_parity() {
    return ui->parity->currentIndex();
}

void DockRTTY::on_reset_clicked() {
    ui->text->setText("");
    emit rtty_reset_clicked();
}

void DockRTTY::on_swap_clicked() {
    float mark,space;

    mark=ui->space_freq->value();
    space=ui->mark_freq->value();

    ui->mark_freq->setValue(mark);
    ui->space_freq->setValue(space);

    emit rtty_mark_freq_Changed(ui->mark_freq->value());
    emit rtty_space_freq_Changed(ui->space_freq->value());
}

void DockRTTY::on_save_clicked() {
    emit rtty_save_clicked(ui->text->toPlainText());
}
