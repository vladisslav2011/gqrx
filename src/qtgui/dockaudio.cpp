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

    muteButtonMenu = new QMenu(this);
    // MenuItem Global mute
    {
        muteAllAction = new QAction("Global Mute", this);
        muteAllAction->setCheckable(true);
        muteButtonMenu->addAction(muteAllAction);
        connect(muteAllAction, SIGNAL(toggled(bool)), this, SLOT(menuMuteAll(bool)));
    }
    ui->audioMuteButton->setContextMenuPolicy(Qt::CustomContextMenu);

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

    QShortcut *rec_toggle_shortcut = new QShortcut(QKeySequence(Qt::Key_R), this);
    QShortcut *mute_toggle_shortcut = new QShortcut(QKeySequence(Qt::Key_M), this);
    QShortcut *audio_gain_increase_shortcut1 = new QShortcut(QKeySequence(Qt::Key_Plus), this);
    QShortcut *audio_gain_decrease_shortcut1 = new QShortcut(QKeySequence(Qt::Key_Minus), this);

    QObject::connect(rec_toggle_shortcut, &QShortcut::activated, this, &DockAudio::recordToggleShortcut);
    QObject::connect(mute_toggle_shortcut, &QShortcut::activated, this, &DockAudio::muteToggleShortcut);
    QObject::connect(audio_gain_increase_shortcut1, &QShortcut::activated, this, &DockAudio::increaseAudioGainShortcut);
    QObject::connect(audio_gain_decrease_shortcut1, &QShortcut::activated, this, &DockAudio::decreaseAudioGainShortcut);
    grid_init(ui->gridLayout,ui->gridLayout->rowCount(),0/*ui->gridLayout->columnCount()*/);
    ui_windows[W_CHILD]=audioOptions;
    set_observer(C_AUDIO_FFT_SPLIT,&DockAudio::audioFFTSplitObserver);
    set_observer(C_AUDIO_FFT_PAND_MAX_DB,&DockAudio::audioFFTPandMaxObserver);
    set_observer(C_AUDIO_FFT_PAND_MIN_DB,&DockAudio::audioFFTPandMinObserver);
    set_observer(C_AUDIO_FFT_WF_MAX_DB,&DockAudio::audioFFTWfMaxObserver);
    set_observer(C_AUDIO_FFT_WF_MIN_DB,&DockAudio::audioFFTWfMinObserver);
    set_observer(C_AUDIO_FFT_RANGE_LOCKED,&DockAudio::audioFFTLockObserver);
    set_observer(C_AUDIO_REC_SQUELCH_TRIGGERED,&DockAudio::audioRecSquelchTriggeredObserver);
    
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

void DockAudio::setAudioMute(bool on)
{
    ui->audioMuteButton->setChecked(on);
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

/*! \brief Streaming button clicked.
 *  \param checked Whether streaming is ON or OFF.
 */
void DockAudio::on_audioStreamButton_clicked(bool checked)
{
    if (checked)
        emit audioStreamingStarted();
    else
        emit audioStreamingStopped();
}

/*! \brief Record button clicked.
 *  \param checked Whether recording is ON or OFF.
 *
 * We use the clicked signal instead of the toggled which allows us to change the
 * state programmatically using toggle() without triggering the signal.
 */
void DockAudio::on_audioRecButton_clicked(bool checked)
{
    if (checked) {

        // emit signal and start timer
        emit audioRecStart();
    }
    else {
        emit audioRecStop();
    }
}

/*! \brief Playback button clicked.
 *  \param checked Whether playback is ON or OFF.
 *
 * We use the clicked signal instead of the toggled which allows us to change the
 * state programmatically using toggle() without triggering the signal.
 */
void DockAudio::on_audioPlayButton_clicked(bool checked)
{
    if (checked) {
        QFileInfo info(last_audio);

        if(info.exists()) {
            // emit signal and start timer
            emit audioPlayStarted(last_audio);

            ui->audioRecLabel->setText(info.fileName());
            ui->audioPlayButton->setToolTip(tr("Stop audio playback"));
            ui->audioRecButton->setEnabled(false); // prevent recording while we play
        }
        else {
            ui->audioPlayButton->setChecked(false);
            ui->audioPlayButton->setEnabled(false);
        }
    }
    else {
        ui->audioRecLabel->setText("<i>DSP</i>");
        ui->audioPlayButton->setToolTip(tr("Start playback of last recorded audio file"));
        emit audioPlayStopped();

        ui->audioRecButton->setEnabled(true);
    }
}

/*! \brief Configure button clicked. */
void DockAudio::on_audioConfButton_clicked()
{
    audioOptions->show();
}

/*! \brief Mute audio. */
void DockAudio::on_audioMuteButton_clicked(bool checked)
{
    emit audioMuteChanged(checked, false);
}

void DockAudio::on_audioMuteButton_customContextMenuRequested(const QPoint& pos)
{
    muteButtonMenu->popup(ui->audioMuteButton->mapToGlobal(pos));
}

/*! \brief Set status of audio record button. */
void DockAudio::setAudioRecButtonState(bool checked)
{
    if (checked == ui->audioRecButton->isChecked()) {
        /* nothing to do */
        return;
    }

    // toggle the button and set the state of the other buttons accordingly
    ui->audioRecButton->toggle();
    bool isChecked = ui->audioRecButton->isChecked();

    ui->audioRecButton->setToolTip(isChecked ? tr("Stop audio recorder") : tr("Start audio recorder"));
    ui->audioPlayButton->setEnabled(!isChecked);
}

void DockAudio::setAudioStreamState(bool running)
{
    setAudioStreamButtonState(running);
}

/*! \brief Set status of audio record button. */
void DockAudio::setAudioStreamButtonState(bool checked)
{
    if (checked == ui->audioStreamButton->isChecked()) {
        /* nothing to do */
        return;
    }

    // toggle the button and set the state of the other buttons accordingly
    ui->audioStreamButton->toggle();
    bool isChecked = ui->audioStreamButton->isChecked();

    ui->audioStreamButton->setToolTip(isChecked ? tr("Stop audio streaming") : tr("Start audio streaming"));
    //TODO: disable host/port controls
}

/*! \brief Set status of audio record button. */
void DockAudio::setAudioPlayButtonState(bool checked)
{
    if (checked == ui->audioPlayButton->isChecked()) {
        // nothing to do
        return;
    }

    // toggle the button and set the state of the other buttons accordingly
    ui->audioPlayButton->toggle();
    bool isChecked = ui->audioPlayButton->isChecked();

    ui->audioPlayButton->setToolTip(isChecked ? tr("Stop audio playback") : tr("Start playback of last recorded audio file"));
    ui->audioRecButton->setEnabled(!isChecked);
    //ui->audioRecConfButton->setEnabled(!isChecked);
}

/*! \brief Slot called when audio recording is started after clicking rec or being triggered by squelch. */
void DockAudio::audioRecStarted(const QString filename)
{
    last_audio = filename;
    QFileInfo info(last_audio);
    ui->audioRecLabel->setText(info.fileName());
    ui->audioRecButton->setToolTip(tr("Stop audio recorder"));
    ui->audioPlayButton->setEnabled(false); /* prevent playback while recording */
    setAudioRecButtonState(true);
}

void DockAudio::audioRecStopped()
{
    ui->audioRecLabel->setText("<i>DSP</i>");
    ui->audioRecButton->setToolTip(tr("Start audio recorder"));
    ui->audioPlayButton->setEnabled(true);
    setAudioRecButtonState(false);
}


void DockAudio::recordToggleShortcut() {
    ui->audioRecButton->click();
}

void DockAudio::muteToggleShortcut() {
    ui->audioMuteButton->click();
}

void DockAudio::increaseAudioGainShortcut() {
    QSlider * audioGainSlider = dynamic_cast<QSlider *>(getWidget(C_AGC_MAN_GAIN));
    if(audioGainSlider->isEnabled())
        audioGainSlider->triggerAction(QSlider::SliderPageStepAdd);
}

void DockAudio::decreaseAudioGainShortcut() {
    QSlider * audioGainSlider = dynamic_cast<QSlider *>(getWidget(C_AGC_MAN_GAIN));
    if(audioGainSlider->isEnabled())
        audioGainSlider->triggerAction(QSlider::SliderPageStepSub);
}

void DockAudio::menuMuteAll(bool checked)
{
    emit audioMuteChanged(checked, true);
    ui->audioMuteButton->setStyleSheet(checked?"color: rgb(255,0,0)":"");
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

void DockAudio::audioRecSquelchTriggeredObserver(const c_id id, const c_def::v_union &value)
{
    ui->audioRecButton->setStyleSheet(bool(value)?"color: rgb(255,0,0)":"");
}
