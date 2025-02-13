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
#include <QString>
#include <QComboBox>
#include <thread>
#include "dockfft.h"
#include "ui_dockfft.h"

DockFft::DockFft(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockFft)
{
    ui->setupUi(this);

    m_sample_rate = 0.f;

    grid_init(ui->gridLayout,ui->gridLayout->rowCount(),0/*ui->gridLayout->columnCount()*/);
    set_observer(C_FFT_SPLIT,&DockFft::fftSplitObserver);
}


DockFft::~DockFft()
{
    delete ui;
}

void DockFft::finalize()
{
    dcontrols_ui::finalize();
    QComboBox * threadsComboBox = dynamic_cast<QComboBox *>(getWidget(C_WF_BG_THREADS));
    threadsComboBox->addItem(tr("Auto"),0);
    for (unsigned k = 1; k <= std::thread::hardware_concurrency(); k++)
        threadsComboBox->addItem(QString::number(k), k);
}

void DockFft::setSampleRate(float sample_rate)
{
    if (sample_rate < 0.1f)
        return;

    m_sample_rate = sample_rate;
    c_def::v_union rate;
    get_gui(C_FFT_RATE, rate);
    c_def::v_union size;
    get_gui(C_FFT_SIZE, size);
    updateInfoLabels(rate, size);
}

void DockFft::setFftLag(bool l)
{
    qvariant_cast<QWidget *>(getWidget(C_FFT_RATE)->property("title"))->setStyleSheet(l?"background:red;":"");
}

/** Update RBW and FFT overlab labels */
void DockFft::updateInfoLabels(int rate, int size_in)
{
    float   interval_ms;
    float   interval_samples;
    float   size = size_in;
    float   rbw;
    float   ovr;

    if (m_sample_rate == 0.f)
        return;

    rbw = m_sample_rate / size;

    if (rbw < 1.e3f)
        set_gui(C_FFT_RBW_LABEL, QString("RBW:%1Hz").arg((double)rbw, 0, 'f', (rbw > 1.0f) ? 1 : 3).toStdString());
    else if (rbw < 1.e6f)
        set_gui(C_FFT_RBW_LABEL, QString("RBW:%1kHz").arg(1.e-3 * (double)rbw, 0, 'f', (rbw<100.0f)?1:0).toStdString());
    else
        set_gui(C_FFT_RBW_LABEL, QString("RBW:%1MHz").arg(1.e-6 * (double)rbw, 0, 'f', (rbw<1e5f)?1:0).toStdString());

    if (rate == 0)
        ovr = 0;
    else
    {
        interval_ms = 1000 / rate;
        interval_samples = m_sample_rate * (interval_ms / 1000.0f);
        if (interval_samples >= size)
            ovr = 0;
        else
            ovr = 100 * (1.f - interval_samples / size);
    }
    set_gui(C_FFT_OVERLAP_LABEL, ovr);
}

void DockFft::fftSplitObserver(const c_id id, const c_def::v_union &value)
{
    const c_def & d = c_def::all()[C_FFT_SPLIT];
    QSlider * antSelector=dynamic_cast<QSlider *>(getWidget(C_FFT_SPLIT));
    antSelector->setToolTip(QString("%1 %2\%").arg(d.title()).arg(int(value)));
}
