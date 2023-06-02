/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2016 Alexandru Csete OZ9AEC.
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
#ifndef DOCKAUDIO_H
#define DOCKAUDIO_H

#include <QColor>
#include <QDockWidget>
#include <QSettings>
#include <QMenu>
#include "audio_options.h"
#include "applications/gqrx/dcontrols_ui.h"

namespace Ui {
    class DockAudio;
}


/*! \brief Dock window with audio options.
 *  \ingroup UI
 *
 * This dock widget encapsulates the audio options.
 * The UI itself is in the dockaudio.ui file.
 *
 * This class also provides the signal/slot API necessary to connect
 * the encapsulated widgets to the rest of the application.
 */
class DockAudio : public QDockWidget, public dcontrols_ui
{
    Q_OBJECT

public:
    explicit DockAudio(QWidget *parent = 0);
    ~DockAudio();

    void setFftRange(quint64 minf, quint64 maxf);
    void setFftSampleRate(quint64 rate);
    void setNewFftData(float *fftData, int size);
    void setInvertScrolling(bool enabled);
    int  fftRate() const { return 10; }

    void setGainEnabled(bool state);

    void setAudioRecButtonState(bool checked);

    void setFftColor(QColor color);
    void setFftFill(bool enabled);

public slots:
    void setRxFrequency(qint64 freq);
    void setWfColormap(const QString &cmap);

signals:
    /*! \brief Signal emitted when audio gain has changed. Gain is in dB. */
    void audioGainChanged(float gain);

    /*! \brief FFT rate changed. */
    void fftRateChanged(int fps);

private:
    void audioFFTSplitObserver(const c_id id, const c_def::v_union &value);
    void audioFFTPandMinObserver(const c_id id, const c_def::v_union &value);
    void audioFFTPandMaxObserver(const c_id id, const c_def::v_union &value);
    void audioFFTWfMinObserver(const c_id id, const c_def::v_union &value);
    void audioFFTWfMaxObserver(const c_id id, const c_def::v_union &value);
    void audioFFTLockObserver(const c_id id, const c_def::v_union &value);
    void audioRecSquelchTriggeredObserver(const c_id id, const c_def::v_union &value);
    void globalMuteObserver(const c_id id, const c_def::v_union &value);
    void audioStreamObserver(const c_id id, const c_def::v_union &value);
    void audioRecObserver(const c_id id, const c_def::v_union &value);
    void audioRecFilenameObserver(const c_id id, const c_def::v_union &value);
    void audioPlayObserver(const c_id id, const c_def::v_union &value);
    void audioOptionsObserver(const c_id id, const c_def::v_union &value);
    void increaseAudioGainObserver(const c_id, const c_def::v_union &);
    void decreaseAudioGainObserver(const c_id, const c_def::v_union &);
    // No spacer at bottom here
    void finalizeInner() override {}

private slots:
    void on_audioSpectrum_pandapterRangeChanged(float,float);

private:
    Ui::DockAudio *ui;
    CAudioOptions *audioOptions; /*! Audio options dialog. */
    QString        last_audio;   /*! Last audio recording. */

    bool           autoSpan;     /*! Whether to allow mode-dependent auto span. */

    qint64         rx_freq;      /*! RX frequency used in filenames. */

    bool           m_suppressPandUpdate;
};

#endif // DOCKAUDIO_H
