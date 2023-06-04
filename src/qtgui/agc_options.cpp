/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2012-2013 Alexandru Csete OZ9AEC.
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
#include <QString>
#include "agc_options.h"
#include "ui_agc_options.h"

CAgcOptions::CAgcOptions(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CAgcOptions)
{
    ui->setupUi(this);
    grid_init(ui->gridLayout,ui->gridLayout->rowCount(),0/*ui->gridLayout->columnCount()*/);
    set_observer(C_AGC_PANNING_AUTO, &CAgcOptions::panningAutoObserver);
}

CAgcOptions::~CAgcOptions()
{
    delete ui;
}

/*! \brief Catch window close events.
 *
 * This method is called when the user closes the dialog window using the
 * window close icon. We catch the event and hide the dialog but keep it
 * around for later use.
 */
void CAgcOptions::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

const CAgcOptions::agc_preset & CAgcOptions::findPreset(const std::string & pr)
{
    for(auto & pp : presets)
        if(pr == pp.key)
            return pp;
    return presets[4];
}

const CAgcOptions::agc_preset & CAgcOptions::findPreset(const int attack, const int decay, const int hang)
{
    for(auto & pp : presets)
        if(attack == int(pp.attack))
            if(decay == int(pp.decay))
                if(hang == int(pp.hang))
                    return pp;
    return presets[3];
}

void CAgcOptions::enableControls(bool state1, bool state2)
{
    getWidget(C_AGC_ATTACK)->setEnabled(state1);
    getWidget(C_AGC_ATTACK_LABEL)->setEnabled(state1);
    getWidget(C_AGC_DECAY)->setEnabled(state1);
    getWidget(C_AGC_DECAY_LABEL)->setEnabled(state1);
    getWidget(C_AGC_HANG)->setEnabled(state1);
    getWidget(C_AGC_HANG_LABEL)->setEnabled(state1);
    getWidget(C_AGC_MAX_GAIN)->setEnabled(state2);
    getWidget(C_AGC_MAX_GAIN_LABEL)->setEnabled(state2);
    getWidget(C_AGC_TARGET)->setEnabled(state2);
    getWidget(C_AGC_TARGET_LABEL)->setEnabled(state2);
}

void CAgcOptions::panningAutoObserver(const c_id id, const c_def::v_union &value)
{
    getWidget(C_AGC_PANNING)->setEnabled(!value);
    getWidget(C_AGC_PANNING_LABEL)->setEnabled(!value);
}

const std::array<CAgcOptions::agc_preset, 5> CAgcOptions::presets{
    CAgcOptions::agc_preset({.key="Fast",  .attack=20, .decay=100, .hang=0,.off=false,.user=false}),
    CAgcOptions::agc_preset({.key="Medium",.attack=50, .decay=500, .hang=0,.off=false,.user=false}),
    CAgcOptions::agc_preset({.key="Slow",  .attack=100,.decay=2000,.hang=0,.off=false,.user=false}),
    CAgcOptions::agc_preset({.key="User",  .attack=0,  .decay=0,   .hang=0,.off=false,.user=true}),
    CAgcOptions::agc_preset({.key="Off",   .attack=0,  .decay=0,   .hang=0,.off=true, .user=false}),
};