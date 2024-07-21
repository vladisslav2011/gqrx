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
#include <cmath>
#include <QDebug>
#include <QDateTime>
#include <QShortcut>
#include <QSlider>
#include <QPushButton>
#include <QDir>
#include "dockaudio.h"
#include "ui_dockaudio.h"

DockAudio::DockAudio(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockAudio),
    autoSpan(true),
    rx_freq(144000000),
    m_suppressPandUpdate(false)
{
    ui->setupUi(this);

    audioOptions = new CAudioOptions(this);

    ui->audioSpectrum->setFreqUnits(1000);
    ui->audioSpectrum->setSampleRate(48000);  // Full bandwidth
    ui->audioSpectrum->setSpanFreq(12000);
    ui->audioSpectrum->setCenterFreq(0);
    ui->audioSpectrum->setPercent2DScreen(c_def::all()[C_AUDIO_FFT_SPLIT].def());
    ui->audioSpectrum->setFftCenterFreq(6000);
    ui->audioSpectrum->setDemodCenterFreq(0);
    ui->audioSpectrum->setFilterBoxEnabled(false);
    ui->audioSpectrum->setCenterLineEnabled(false);
    ui->audioSpectrum->setBookmarksEnabled(false);
    ui->audioSpectrum->setBandPlanEnabled(false);
    ui->audioSpectrum->setFftRange(-80., 0.);
    ui->audioSpectrum->setVdivDelta(40);
    ui->audioSpectrum->setHdivDelta(40);
    ui->audioSpectrum->setFreqDigits(1);

    grid_init(ui->gridLayout,ui->gridLayout->rowCount(),0/*ui->gridLayout->columnCount()*/);
    ui_windows[W_CHILD]=audioOptions;
    set_observer(C_AUDIO_FFT_SPLIT,&DockAudio::audioFFTSplitObserver);
    set_observer(C_AUDIO_FFT_PAND_MAX_DB,&DockAudio::audioFFTPandMaxObserver);
    set_observer(C_AUDIO_FFT_PAND_MIN_DB,&DockAudio::audioFFTPandMinObserver);
    set_observer(C_AUDIO_FFT_WF_MAX_DB,&DockAudio::audioFFTWfMaxObserver);
    set_observer(C_AUDIO_FFT_WF_MIN_DB,&DockAudio::audioFFTWfMinObserver);
    set_observer(C_AUDIO_FFT_RANGE_LOCKED,&DockAudio::audioFFTLockObserver);
    set_observer(C_AUDIO_FFT_ZOOM,&DockAudio::audioFFTZoomObserver);
    set_observer(C_AUDIO_FFT_CENTER,&DockAudio::audioFFTCenterObserver);
    set_observer(C_AUDIO_REC_SQUELCH_TRIGGERED,&DockAudio::audioRecSquelchTriggeredObserver);
    set_observer(C_GLOBAL_MUTE,&DockAudio::globalMuteObserver);
    set_observer(C_AUDIO_UDP_STREAMING,&DockAudio::audioStreamObserver);
    set_observer(C_AUDIO_REC, &DockAudio::audioRecObserver);
    set_observer(C_AUDIO_REC_FILENAME, &DockAudio::audioRecFilenameObserver);
    set_observer(C_AUDIO_PLAY, &DockAudio::audioPlayObserver);
    set_observer(C_AUDIO_OPTIONS, &DockAudio::audioOptionsObserver);
    set_observer(C_AGC_MAN_GAIN_UP, &DockAudio::increaseAudioGainObserver);
    set_observer(C_AGC_MAN_GAIN_DOWN, &DockAudio::decreaseAudioGainObserver);
}

DockAudio::~DockAudio()
{
    delete ui;
}

void DockAudio::setFftRange(quint64 minf, quint64 maxf)
{
    if (autoSpan)
    {
        qint32 span = (qint32)(maxf - minf);
        quint64 fc = minf + (maxf - minf)/2;

        ui->audioSpectrum->setFftCenterFreq(fc);
        ui->audioSpectrum->setSpanFreq(span);
        ui->audioSpectrum->setCenterFreq(0);
        changed_gui(C_AUDIO_FFT_ZOOM, float(ui->audioSpectrum->getSampleRate())/float(span), false);
        changed_gui(C_AUDIO_FFT_CENTER, int64_t(fc), false);
    }
}

void DockAudio::setFftSampleRate(quint64 rate)
{
    ui->audioSpectrum->setSampleRate(rate);
}

void DockAudio::setNewFftData(float *fftData, int size)
{
    ui->audioSpectrum->setNewFftData(fftData, size);
}

void DockAudio::setInvertScrolling(bool enabled)
{
    ui->audioSpectrum->setInvertScrolling(enabled);
}

/*! \brief Set audio gain slider state.
 *  \param state new slider state.
 */
void DockAudio::setGainEnabled(bool state)
{
    getWidget(C_AGC_MAN_GAIN)->setEnabled(state);
}

/*! Set FFT plot color. */
void DockAudio::setFftColor(QColor color)
{
    ui->audioSpectrum->setFftPlotColor(color);
}

/*! Enable/disable filling area under FFT plot. */
void DockAudio::setFftFill(bool enabled)
{
    ui->audioSpectrum->setFftFill(enabled);
}

/*! Public slot to set new RX frequency in Hz. */
void DockAudio::setRxFrequency(qint64 freq)
{
    rx_freq = freq;
}

void DockAudio::setWfColormap(const QString &cmap)
{
    ui->audioSpectrum->setWfColormap(cmap);
}

void DockAudio::audioStreamObserver(const c_id id, const c_def::v_union &value)
{
    dynamic_cast<QWidget *>(getWidget(C_AUDIO_UDP_HOST))->setEnabled(!bool(value));
    dynamic_cast<QWidget *>(getWidget(C_AUDIO_UDP_PORT))->setEnabled(!bool(value));
    dynamic_cast<QWidget *>(getWidget(C_AUDIO_UDP_STEREO))->setEnabled(!bool(value));
    dynamic_cast<QWidget *>(getWidget(C_AUDIO_UDP_STREAMING))->setToolTip(bool(value) ? tr("Stop audio streaming") : tr("Start audio streaming"));
}

void DockAudio::audioRecObserver(const c_id id, const c_def::v_union &value)
{
    bool isChecked = value;

    QWidget * audioRecButton = getWidget(C_AUDIO_REC);
    audioRecButton->setToolTip(isChecked ? tr("Stop audio recorder") : tr("Start audio recorder"));
    getWidget(C_AUDIO_PLAY)->setEnabled(!isChecked);
    if(!isChecked)
        set_gui(C_AUDIO_REC_FILENAME,"DSP");
}

void DockAudio::audioRecFilenameObserver(const c_id id, const c_def::v_union &value)
{
    last_audio = QString::fromStdString(value);
    QFileInfo info(last_audio);
    c_def::v_union is_rec;
    get_gui(C_AUDIO_REC, is_rec);
    getWidget(C_AUDIO_PLAY)->setEnabled(!((last_audio=="DSP")||(last_audio=="")||is_rec));
}

void DockAudio::audioPlayObserver(const c_id id, const c_def::v_union &value)
{
    getWidget(C_AUDIO_REC)->setEnabled(!bool(value));
    if(bool(value))
    {
        getWidget(C_AUDIO_PLAY)->setToolTip(tr("Stop audio playback"));
    }else{
        set_gui(C_AUDIO_REC_FILENAME,"DSP");
        getWidget(C_AUDIO_PLAY)->setToolTip(tr("Start playback of last recorded audio file"));
    }
}

void DockAudio::increaseAudioGainObserver(const c_id, const c_def::v_union &)
{
    QSlider * audioGainSlider = dynamic_cast<QSlider *>(getWidget(C_AGC_MAN_GAIN));
    if(audioGainSlider->isEnabled())
        audioGainSlider->triggerAction(QSlider::SliderPageStepAdd);
}

void DockAudio::decreaseAudioGainObserver(const c_id, const c_def::v_union &)
{
    QSlider * audioGainSlider = dynamic_cast<QSlider *>(getWidget(C_AGC_MAN_GAIN));
    if(audioGainSlider->isEnabled())
        audioGainSlider->triggerAction(QSlider::SliderPageStepSub);
}

void DockAudio::globalMuteObserver(const c_id id, const c_def::v_union &value)
{
    dynamic_cast<QPushButton *>(getWidget(C_AGC_MUTE))->setStyleSheet(bool(value)?"color: rgb(255,0,0)":"");
}

void DockAudio::audioFFTSplitObserver(const c_id id, const c_def::v_union &value)
{
    ui->audioSpectrum->setPercent2DScreen(value);
}

void DockAudio::audioFFTPandMinObserver(const c_id id, const c_def::v_union &value)
{
    c_def::v_union max(0);
    get_gui(C_AUDIO_FFT_PAND_MAX_DB,max);
    ui->audioSpectrum->setPandapterRange(int(value), int(max));
    c_def::v_union locked(0);
    get_gui(C_AUDIO_FFT_RANGE_LOCKED,locked);
    if(locked )
    {
        c_def::v_union min(0);
        ui->audioSpectrum->setWaterfallRange(int(value), int(max));
        if(id == C_AUDIO_FFT_PAND_MIN_DB)
        {
            get_gui(C_AUDIO_FFT_WF_MIN_DB,min);
            if(min == value)
                return;
            set_gui(C_AUDIO_FFT_WF_MIN_DB,value,false);
        }else{
            get_gui(C_AUDIO_FFT_PAND_MIN_DB,min);
            if(min == value)
                return;
            set_gui(C_AUDIO_FFT_PAND_MIN_DB,value,false);
        }
    }
}

void DockAudio::audioFFTPandMaxObserver(const c_id id, const c_def::v_union &value)
{
    c_def::v_union min(0);
    get_gui(C_AUDIO_FFT_PAND_MIN_DB,min);
    ui->audioSpectrum->setPandapterRange(int(min), int(value));
    c_def::v_union locked(0);
    get_gui(C_AUDIO_FFT_RANGE_LOCKED,locked);
    if(locked)
    {
        c_def::v_union max(0);
        ui->audioSpectrum->setWaterfallRange(int(min), int(value));
        if(id == C_AUDIO_FFT_PAND_MAX_DB)
        {
            get_gui(C_AUDIO_FFT_WF_MAX_DB,max);
            if(max == value)
                return;
            set_gui(C_AUDIO_FFT_WF_MAX_DB,value);
        }else{
            get_gui(C_AUDIO_FFT_PAND_MAX_DB,max);
            if(max == value)
                return;
            set_gui(C_AUDIO_FFT_PAND_MAX_DB,value);
        }
    }
}

void DockAudio::on_audioSpectrum_pandapterRangeChanged(float min, float max)
{
    set_gui(C_AUDIO_FFT_PAND_MIN_DB,int(min));
    set_gui(C_AUDIO_FFT_PAND_MAX_DB,int(max));
    c_def::v_union locked(0);
    get_gui(C_AUDIO_FFT_RANGE_LOCKED,locked);
    if(locked)
    {
        set_gui(C_AUDIO_FFT_WF_MIN_DB,int(min));
        set_gui(C_AUDIO_FFT_WF_MAX_DB,int(max));
        ui->audioSpectrum->setWaterfallRange(min, max);
    }
}

void DockAudio::on_audioSpectrum_newZoomLevel(float z)
{
    changed_gui(C_AUDIO_FFT_ZOOM, z, false);
}

void DockAudio::on_audioSpectrum_newFftCenterFreq(qint64 c)
{
    changed_gui(C_AUDIO_FFT_CENTER, c, false);
}

void DockAudio::audioFFTWfMinObserver(const c_id id, const c_def::v_union &value)
{
    c_def::v_union max(0);
    get_gui(C_AUDIO_FFT_WF_MAX_DB,max);
    ui->audioSpectrum->setWaterfallRange(int(value), int(max));
}

void DockAudio::audioFFTWfMaxObserver(const c_id id, const c_def::v_union &value)
{
    c_def::v_union min(0);
    get_gui(C_AUDIO_FFT_WF_MIN_DB,min);
    ui->audioSpectrum->setWaterfallRange(int(min), int(value));
}

void DockAudio::audioFFTLockObserver(const c_id id, const c_def::v_union &value)
{
    if(value)
    {
        set_observer(C_AUDIO_FFT_WF_MAX_DB,&DockAudio::audioFFTPandMaxObserver);
        set_observer(C_AUDIO_FFT_WF_MIN_DB,&DockAudio::audioFFTPandMinObserver);
    }else{
        set_observer(C_AUDIO_FFT_WF_MAX_DB,&DockAudio::audioFFTWfMaxObserver);
        set_observer(C_AUDIO_FFT_WF_MIN_DB,&DockAudio::audioFFTWfMinObserver);
    }
}

void DockAudio::audioFFTZoomObserver(const c_id id, const c_def::v_union &value)
{
    ui->audioSpectrum->blockSignals(true);
    ui->audioSpectrum->zoomOnXAxis(value);
    ui->audioSpectrum->blockSignals(false);
}

void DockAudio::audioFFTCenterObserver(const c_id id, const c_def::v_union &value)
{
    ui->audioSpectrum->setFftCenterFreq(value);
    ui->audioSpectrum->updateOverlay();
}

void DockAudio::audioRecSquelchTriggeredObserver(const c_id id, const c_def::v_union &value)
{
    getWidget(C_AUDIO_REC)->setStyleSheet(bool(value)?"color: rgb(255,0,0)":"");
}

/*! \brief Configure button clicked. */
void DockAudio::audioOptionsObserver(const c_id id, const c_def::v_union &value)
{
    audioOptions->show();
}
