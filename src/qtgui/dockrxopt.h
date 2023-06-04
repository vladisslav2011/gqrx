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
#ifndef DOCKRXOPT_H
#define DOCKRXOPT_H

#include <QDockWidget>
#include <QSettings>
#include <QMenu>
#include "qtgui/agc_options.h"
#include "qtgui/demod_options.h"
#include "qtgui/nb_options.h"
#include "receivers/defines.h"
#include "receivers/modulations.h"
#include "applications/gqrx/dcontrols_ui.h"

namespace Ui {
    class DockRxOpt;
}


/**
 * @brief Dock window with receiver options.
 * @ingroup UI
 *
 * This dock widget encapsulates the receiver options. The controls
 * are grouped in a tool box that allows packing many controls in little space.
 * The UI itself is in the dockrxopt.ui file.
 *
 * This class also provides the signal/slot API necessary to connect
 * the encapsulated widgets to the rest of the application.
 */
class DockRxOpt : public QDockWidget, public dcontrols_ui
{
    Q_OBJECT

public:

    explicit DockRxOpt(qint64 filterOffsetRange = 90000, QWidget *parent = 0);
    ~DockRxOpt();

    void setFilterOffsetRange(qint64 range_hz);

    void setRxFreqRange(qint64 min_hz, qint64 max_hz);

    void setResetLowerDigits(bool enabled);
    void setInvertScrolling(bool enabled);

public slots:
    void setFilterOffset(qint64 freq_hz);

private:
    unsigned int filterIdxFromLoHi(int lo, int hi) const;
    void setFilterParam(int lo, int hi);

    void setAgcPresetFromParams(int decay);
    void agcOnObserver(const c_id id, const c_def::v_union & v);
    void nbOptObserver(const c_id id, const c_def::v_union & v);
    void agcOptObserver(const c_id id, const c_def::v_union & v);
    void agcPresetObserver(const c_id id, const c_def::v_union & v);
    void modeObserver(const c_id id, const c_def::v_union & v);
    void modeOptObserver(const c_id id, const c_def::v_union & v);
    void filterLoObserver(const c_id id, const c_def::v_union &value);
    void filterHiObserver(const c_id id, const c_def::v_union &value);

signals:
    /** Signal emitted when the channel filter frequency has changed. */
    void filterOffsetChanged(qint64 freq_hz);

private slots:
    void on_filterFreq_newFrequency(qint64 freq);

private:
    void setAgcPreset(const CAgcOptions::agc_preset &);
    Ui::DockRxOpt *ui;        /** The Qt designer UI file. */
    CDemodOptions *demodOpt;  /** Demodulator options. */
    CAgcOptions   *agcOpt;    /** AGC options. */
    CNbOptions    *nbOpt;     /** Noise blanker options. */
    int m_lo;
    int m_hi;
};

#endif // DOCKRXOPT_H
