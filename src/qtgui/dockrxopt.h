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

    void setFilterParam(int lo, int hi);
    void setCurrentFilter(int index);
    int  currentFilter() const;

    void setCurrentFilterShape(int index);
    int  currentFilterShape() const;

    void setHwFreq(qint64 freq_hz);
    void setRxFreqRange(qint64 min_hz, qint64 max_hz);

    void setResetLowerDigits(bool enabled);
    void setInvertScrolling(bool enabled);

    Modulations::idx  currentDemod() const;
    QString currentDemodAsString();

    double currentSquelchLevel() const;

    double  getSqlLevel(void) const;

    bool    getAgcOn();
    void    setAgcOn(bool on);

    void    setNoiseBlanker(int nbid, bool on);

    void    setFreqLock(bool lock);
    bool    getFreqLock();

public slots:
    void setRxFreq(qint64 freq_hz);
    void setCurrentDemod(Modulations::idx demod);
    void setFilterOffset(qint64 freq_hz);
    void setSquelchLevel(double level);

private:
    void updateHwFreq();
    unsigned int filterIdxFromLoHi(int lo, int hi) const;

    void modeOffShortcut();
    void modeRawShortcut();
    void modeAMShortcut();
    void modeNFMShortcut();
    void modeWFMmonoShortcut();
    void modeWFMstereoShortcut();
    void modeLSBShortcut();
    void modeUSBShortcut();
    void modeCWLShortcut();
    void modeCWUShortcut();
    void modeWFMoirtShortcut();
    void modeAMsyncShortcut();
    void filterNarrowShortcut();
    void filterNormalShortcut();
    void filterWideShortcut();
    void setAgcPresetFromParams(int decay);
    void agcDecayObserver(const c_id id, const c_def::v_union & v);
    void agcOnObserver(const c_id id, const c_def::v_union & v);

signals:
    /** Signal emitted when receiver frequency has changed */
    void rxFreqChanged(qint64 freq_hz);

    /** Signal emitted when the channel filter frequency has changed. */
    void filterOffsetChanged(qint64 freq_hz);

    /** Signal emitted when new demodulator is selected. */
    void demodSelected(Modulations::idx demod);

    /** Signal emitted when baseband gain has changed. Gain is in dB. */
    //void bbGainChanged(float gain);

    /** Signal emitted when squelch level has changed. Level is in dBFS. */
    void sqlLevelChanged(double level);

    /**
     * Signal emitted when auto squelch level is clicked.
     *
     * @note Need current signal/noise level returned
     */
    double sqlAutoClicked(bool global);

    /** Signal emitted when squelch reset all popup menu item is clicked. */
    void sqlResetAllClicked();

    /** Signal emitted when noise blanker status has changed. */
    void noiseBlankerChanged(int nbid, bool on);

    /** Signal emitted when freq lock mode changed. */
    void freqLock(bool lock, bool all);

private slots:
    void on_freqSpinBox_valueChanged(double freq);
    void on_filterFreq_newFrequency(qint64 freq);
    void on_filterCombo_activated(int index);
    void on_modeSelector_activated(int index);
    void on_modeButton_clicked();
    void on_agcButton_clicked();
    void on_autoSquelchButton_clicked();
    void on_autoSquelchButton_customContextMenuRequested(const QPoint& pos);
    void menuSquelchAutoAll();
    void on_resetSquelchButton_clicked();
    void menuSquelchResetAll();
    //void on_agcPresetCombo_activated(int index);
    void on_agcPresetCombo_currentIndexChanged(int index);
    void on_sqlSpinBox_valueChanged(double value);
    void on_nb1Button_toggled(bool checked);
    void on_nb2Button_toggled(bool checked);
    void on_nb3Button_toggled(bool checked);
    void on_nbOptButton_clicked();
    void on_freqLockButton_clicked();
    void on_freqLockButton_customContextMenuRequested(const QPoint& pos);
    void menuFreqLockAll();
    void menuFreqUnlockAll();

private:
    Ui::DockRxOpt *ui;        /** The Qt designer UI file. */
    CDemodOptions *demodOpt;  /** Demodulator options. */
    CAgcOptions   *agcOpt;    /** AGC options. */
    CNbOptions    *nbOpt;     /** Noise blanker options. */
    QMenu         *freqLockButtonMenu;
    QMenu         *squelchButtonMenu;

    bool agc_is_on;

    qint64 hw_freq_hz;   /** Current PLL frequency in Hz. */
};

#endif // DOCKRXOPT_H
