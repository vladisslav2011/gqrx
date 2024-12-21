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
#include <QShortcut>
#include <QPushButton>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <iostream>
#include "dockrxopt.h"
#include "ui_dockrxopt.h"

DockRxOpt::DockRxOpt(qint64 filterOffsetRange, QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockRxOpt)
{
    ui->setupUi(this);

    ui->filterFreq->setup(7, -filterOffsetRange/2, filterOffsetRange/2, 1,
                          FCTL_UNIT_KHZ);
    ui->filterFreq->setFrequency(0);

    // demodulator options dialog
    demodOpt = new CDemodOptions(this);

    // AGC options dialog
    agcOpt = new CAgcOptions(this);

    // Noise blanker options
    nbOpt = new CNbOptions(this);

    grid_init(ui->gridLayout,ui->gridLayout->rowCount(),0/*ui->gridLayout->columnCount()*/);
    ui_windows[W_DEMOD_OPT]=demodOpt;
    ui_windows[W_NB_OPT]=nbOpt;
    ui_windows[W_AGC_OPT]=agcOpt;
    demodOpt->setCurrentIndex(0);
    set_observer(C_AGC_ON,&DockRxOpt::agcOnObserver);
    set_observer(C_NB_OPT,&DockRxOpt::nbOptObserver);
    set_observer(C_AGC_OPT,&DockRxOpt::agcOptObserver);
    set_observer(C_AGC_PRESET,&DockRxOpt::agcPresetObserver);
    set_observer(C_MODE,&DockRxOpt::modeObserver);
    set_observer(C_MODE_OPT,&DockRxOpt::modeOptObserver);
    set_observer(C_FILTER_LO, &DockRxOpt::filterLoObserver);
    set_observer(C_FILTER_HI, &DockRxOpt::filterHiObserver);
}

DockRxOpt::~DockRxOpt()
{
    delete ui;
    delete demodOpt;
    delete agcOpt;
    delete nbOpt;
}

/**
 * @brief Set value of channel filter offset selector.
 * @param freq_hz The frequency in Hz
 */
void DockRxOpt::setFilterOffset(qint64 freq_hz)
{
    ui->filterFreq->setFrequency(freq_hz);
}

/**
 * @brief Set filter offset range.
 * @param range_hz The new range in Hz.
 */
void DockRxOpt::setFilterOffsetRange(qint64 range_hz)
{
    int num_digits;

    if (range_hz <= 0)
        return;

    range_hz /= 2;
    if (range_hz < 100e3)
        num_digits = 5;
    else if (range_hz < 1e6)
        num_digits = 6;
    else if (range_hz < 1e7)
        num_digits = 7;
    else if (range_hz < 1e8)
        num_digits = 8;
    else
        num_digits = 9;

    ui->filterFreq->setup(num_digits, -range_hz, range_hz, 1, FCTL_UNIT_KHZ);
}

/**
 * Get filter index from filter LO / HI values.
 * @param lo The filter low cut frequency.
 * @param hi The filter high cut frequency.
 *
 * Given filter low and high cut frequencies, this function checks whether the
 * filter settings correspond to one of the presets in filter_preset_table and
 * returns the corresponding index to ui->filterCombo;
 */
unsigned int DockRxOpt::filterIdxFromLoHi(int lo, int hi) const
{
    c_def::v_union tmp;
    get_gui(C_MODE,tmp);
    Modulations::idx mode_index = Modulations::idx(int(tmp));
    return Modulations::FindFilterPreset(mode_index, lo, hi);
}

/**
 * @brief Set filter parameters
 * @param lo Low cutoff frequency in Hz
 * @param hi High cutoff frequency in Hz.
 *
 * This function will automatically select the "User" preset in the
 * combo box.
 */
void DockRxOpt::setFilterParam(int lo, int hi)
{
    int filter_index = filterIdxFromLoHi(lo, hi);

    set_gui(C_FILTER_WIDTH, filter_index);
    if (filter_index == FILTER_PRESET_USER)
    {
        const float width_f = abs((hi-lo)/1000.f);
        float width_o = (hi+lo)/2000.f;
        if(abs(width_o) < 0.001f)
            width_o=0.f;
        char sign_o = (width_o>0) ? '+' : '-';
        width_o = abs(width_o);
        if(width_o > 0.f)
            dynamic_cast<QComboBox *>(getWidget(C_FILTER_WIDTH))->setItemText(FILTER_PRESET_USER, QString("User (%1%2%3 k)")
                                    .arg((double)width_f).arg(sign_o).arg((double)width_o));
        else
            dynamic_cast<QComboBox *>(getWidget(C_FILTER_WIDTH))->setItemText(FILTER_PRESET_USER, QString("User (%1 k)")
                                        .arg((double)width_f));
    }
}

void DockRxOpt::agcOnObserver(const c_id id, const c_def::v_union & v)
{
    if (bool(v))
    {
        c_def::v_union attack(0);
        c_def::v_union decay(0);
        c_def::v_union hang(0);
        get_gui(C_AGC_ATTACK, attack);
        get_gui(C_AGC_DECAY, decay);
        get_gui(C_AGC_HANG, hang);
        auto pp = agcOpt->findPreset(int(attack),int(decay),int(hang));
        set_gui(C_AGC_PRESET,pp.key);
        if(pp.user)
            agcOpt->enableControls(true,true);
        else
            agcOpt->enableControls(false,true);
    }else{
        set_gui(C_AGC_PRESET,"Off");
        agcOpt->enableControls(false,false);
    }
}

void DockRxOpt::nbOptObserver(const c_id id, const c_def::v_union & v)
{
    nbOpt->show();
}

/** Show AGC options. */
void DockRxOpt::agcOptObserver(const c_id id, const c_def::v_union & v)
{
    agcOpt->show();
}

/** AGC preset has changed. */
void DockRxOpt::agcPresetObserver(const c_id id, const c_def::v_union & v)
{
    auto pp = agcOpt->findPreset(v);
    setAgcPreset(pp);
    changed_gui(C_AGC_ON,!pp.off);
    set_gui(C_AGC_PRESET,v);
    if(pp.user)
        agcOpt->enableControls(true,true);
    else if(pp.off)
        agcOpt->enableControls(false,false);
    else
        agcOpt->enableControls(false,true);
}

/*! \brief Set AGC preset. */
void DockRxOpt::setAgcPreset(const CAgcOptions::agc_preset & pp)
{
    if(pp.off)
        agcOpt->enableControls(false,false);
    else if(!pp.user)
    {
        set_gui(C_AGC_DECAY, pp.decay);
        changed_gui(C_AGC_DECAY, pp.decay);
        set_gui(C_AGC_ATTACK, pp.attack);
        changed_gui(C_AGC_ATTACK, pp.attack);
        set_gui(C_AGC_HANG, pp.hang);
        changed_gui(C_AGC_HANG,pp.hang);
        agcOpt->enableControls(false,true);
    }else
        agcOpt->enableControls(true,true);
}

void DockRxOpt::modeObserver(const c_id id, const c_def::v_union & v)
{
    Modulations::idx demod = Modulations::idx(int(v));
    if ((demod >= Modulations::MODE_OFF) && (demod < Modulations::MODE_COUNT))
        demodOpt->setCurrentIndex(demod);
}

void DockRxOpt::modeOptObserver(const c_id id, const c_def::v_union & v)
{
    demodOpt->show();
}

void DockRxOpt::filterLoObserver(const c_id id, const c_def::v_union &v)
{
    m_lo=v;
    setFilterParam(m_lo,m_hi);
}

void DockRxOpt::filterHiObserver(const c_id id, const c_def::v_union &v)
{
    m_hi=v;
    setFilterParam(m_lo,m_hi);
}

void DockRxOpt::setRxFreqRange(qint64 min_hz, qint64 max_hz)
{
    QDoubleSpinBox * freqSpinBox = dynamic_cast<QDoubleSpinBox *>(getWidget(C_VFO_FREQUENCY));
    freqSpinBox->blockSignals(true);
    freqSpinBox->setRange(1.e-3 * (double)min_hz, 1.e-3 * (double)max_hz);
    freqSpinBox->blockSignals(false);
}

void DockRxOpt::setResetLowerDigits(bool enabled)
{
    ui->filterFreq->setResetLowerDigits(enabled);
}

void DockRxOpt::setInvertScrolling(bool enabled)
{
    ui->filterFreq->setInvertScrolling(enabled);
}

/**
 * @brief Channel filter offset has changed
 * @param freq The new filter offset in Hz
 *
 * This slot is activated when a new filter offset has been selected either
 * using the mouse or using the keyboard.
 */
void DockRxOpt::on_filterFreq_newFrequency(qint64 freq)
{
    emit filterOffsetChanged(freq);
}
