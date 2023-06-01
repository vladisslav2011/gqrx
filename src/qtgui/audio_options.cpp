/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2013-2016 Alexandru Csete OZ9AEC.
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
#include <QFileDialog>
#include <QPalette>
#include <QDebug>
#include <QComboBox>

#include "audio_options.h"
#include "ui_audio_options.h"

CAudioOptions::CAudioOptions(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CAudioOptions)
{
    ui->setupUi(this);

    work_dir = new QDir();

    error_palette = new QPalette();
    error_palette->setColor(QPalette::Text, Qt::red);
    grid_init(ui->gridLayout,ui->gridLayout->rowCount(),0/*ui->gridLayout->columnCount()*/);
    set_observer(C_AUDIO_FFT_RANGE_LOCKED,&CAudioOptions::audioFFTLockObserver);
    set_observer(C_AUDIO_REC_DIR, &CAudioOptions::recDirObserver);
    set_observer(C_AUDIO_REC_DIR_BTN, &CAudioOptions::recDirButtonObserver);
    set_observer(C_AUDIO_DEDICATED_ON, &CAudioOptions::dedicatedAudioSinkObserver);
}

CAudioOptions::~CAudioOptions()
{
    delete work_dir;
    delete error_palette;
    delete ui;
}


/**
 * Catch window close events.
 *
 * This method is called when the user closes the audio options dialog
 * window using the window close icon. We catch the event and hide the
 * dialog but keep it around for later use.
 */
void CAudioOptions::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

void CAudioOptions::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    // Output device
    updateOutDev();
}

/**
 * Called when the recordings directory has changed either
 * because of user input or programmatically.
 */
void CAudioOptions::recDirObserver(const c_id id, const c_def::v_union &value)
{

    if (work_dir->exists(QString::fromStdString(value)))
        getWidget(C_AUDIO_REC_DIR)->setPalette(QPalette());
    else
        getWidget(C_AUDIO_REC_DIR)->setPalette(*error_palette);  // indicate error
}

/** Called when the user clicks on the "Select" button. */
void CAudioOptions::recDirButtonObserver(const c_id id, const c_def::v_union &value)
{
    c_def::v_union tmp;
    get_gui(C_AUDIO_REC_DIR, tmp);
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select a directory"),
                                                    QString::fromStdString(tmp),
                                                    QFileDialog::ShowDirsOnly |
                                                    QFileDialog::DontResolveSymlinks);

    if (!dir.isNull())
    {
        set_gui(C_AUDIO_REC_DIR,dir.toStdString());
        changed_gui(C_AUDIO_REC_DIR,dir.toStdString());
    }
}

void CAudioOptions::updateOutDev()
{
    QComboBox * outDevCombo = dynamic_cast<QComboBox *>(getWidget(C_AUDIO_DEDICATED_DEV));
    c_def::v_union curr;
    get_gui(C_AUDIO_DEDICATED_DEV, curr);
    outDevCombo->blockSignals(true);
    outDevCombo->clear();

    // get list of audio output devices
#if defined(WITH_PULSEAUDIO) || defined(WITH_PORTAUDIO) || defined(Q_OS_DARWIN)
    audio_device_list devices;
    outDevList = devices.get_output_devices();

    outDevCombo->addItem("Default","");
    qDebug() << __FUNCTION__ << ": Available output devices:";
    for (size_t i = 0; i < outDevList.size(); i++)
    {
        qDebug() << "   " << i << ":" << QString(outDevList[i].get_description().c_str());
        outDevCombo->addItem(QString::fromStdString(outDevList[i].get_description()), QString::fromStdString(outDevList[i].get_name()));

    }
#else
    outDevCombo->addItem("default","");
    outDevCombo->setEditable(true);
#endif // WITH_PULSEAUDIO
    outDevCombo->blockSignals(false);
    set_gui(C_AUDIO_DEDICATED_DEV, curr);
}

void CAudioOptions::audioFFTLockObserver(const c_id id, const c_def::v_union &value)
{
    if (bool(value)) {
        c_def::v_union tmp;
        set_gui(C_AUDIO_FFT_WF_MAX_DB,c_def::all()[C_AUDIO_FFT_WF_MAX_DB].max());
        set_gui(C_AUDIO_FFT_WF_MIN_DB,c_def::all()[C_AUDIO_FFT_WF_MIN_DB].min());
        get_gui(C_AUDIO_FFT_PAND_MIN_DB,tmp);
        set_gui(C_AUDIO_FFT_WF_MIN_DB,tmp);
        get_gui(C_AUDIO_FFT_PAND_MAX_DB,tmp);
        set_gui(C_AUDIO_FFT_WF_MAX_DB,tmp);
        changed_gui(C_AUDIO_FFT_WF_MAX_DB,tmp);
        get_gui(C_AUDIO_FFT_PAND_MIN_DB,tmp);
        changed_gui(C_AUDIO_FFT_WF_MIN_DB,tmp);
    }
}

void CAudioOptions::dedicatedAudioSinkObserver(const c_id id, const c_def::v_union &value)
{
    getWidget(C_AUDIO_DEDICATED_DEV)->setEnabled(!bool(value));
}
