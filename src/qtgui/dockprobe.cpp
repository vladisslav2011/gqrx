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
#include <QDir>
#include "dockprobe.h"
#include "ui_dockprobe.h"

#define DEFAULT_FFT_SPLIT 50

DockProbe::DockProbe(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockProbe),
    m_sampleRate(20000)
{
    ui->setupUi(this);

    //connect(audioOptions, SIGNAL(newFftSplit(int)), ui->spectrum, SLOT(setPercent2DScreen(int)));
    //connect(audioOptions, SIGNAL(newPandapterRange(int,int)), this, SLOT(pandapterRange_changed(int,int)));

    connect(ui->spectrum, SIGNAL(pandapterRangeChanged(float,float)), ui->spectrum, SLOT(setWaterfallRange(float,float)));

    ui->spectrum->setFreqUnits(1000);
    setSampleRate(200000);
    ui->spectrum->setCenterFreq(0);
    ui->spectrum->setPercent2DScreen(DEFAULT_FFT_SPLIT);
    ui->spectrum->setFftCenterFreq(0);
    ui->spectrum->setDemodCenterFreq(0);
    ui->spectrum->setFilterBoxEnabled(false);
    ui->spectrum->setCenterLineEnabled(false);
    ui->spectrum->setBookmarksEnabled(false);
    ui->spectrum->setBandPlanEnabled(false);
    ui->spectrum->setFftRange(-80., 0.);
    ui->spectrum->setVdivDelta(40);
    ui->spectrum->setHdivDelta(40);
    ui->spectrum->setFreqDigits(1);

}

DockProbe::~DockProbe()
{
    delete ui;
}

void DockProbe::setCenterFreq(qint64 freq)
{
    m_center = freq;
    updateCenter();
}

void DockProbe::updateCenter()
{
    int decim = ui->decimSpinBox->value();
    int osr = ui->osrSpinBox->value();
    qint64 input = ui->inputSpinBox->value();
    if(osr == 0)
        osr = 1;
    if(decim == 0)
        decim = 1;
    input %= (decim * osr);
    int add = 0;
    if(input % osr > 2)
        add = 1;
    if(input > decim * osr / 2)
        input -= decim * osr;
    input /= osr;
    input += 1;
    ui->spectrum->setCenterFreq(m_center + input * m_sampleRate / decim);
    ui->spectrum->setSampleRate(m_sampleRate / decim);  // Full bandwidth
    ui->spectrum->setSpanFreq(m_sampleRate / decim);
}

void DockProbe::setFftRange(quint64 minf, quint64 maxf)
{
    if (true)
    {
        qint32 span = (qint32)(maxf - minf);
        quint64 fc = minf + (maxf - minf)/2;

        ui->spectrum->setFftCenterFreq(fc);
        ui->spectrum->setSpanFreq(span);
        ui->spectrum->setCenterFreq(0);
    }
}

void DockProbe::setNewFftData(float *fftData, int size)
{
    ui->spectrum->setNewFftData(fftData, size);
}

void DockProbe::setInvertScrolling(bool enabled)
{
    ui->spectrum->setInvertScrolling(enabled);
}

/*! Set FFT plot color. */
void DockProbe::setFftColor(QColor color)
{
    ui->spectrum->setFftPlotColor(color);
}

/*! Enable/disable filling area under FFT plot. */
void DockProbe::setFftFill(bool enabled)
{
    ui->spectrum->setFftFill(enabled);
}

void DockProbe::setSampleRate(int sampleRate)
{
    m_sampleRate = sampleRate;
    updateCenter();
}

void DockProbe::setWfColormap(const QString &cmap)
{
    ui->spectrum->setWfColormap(cmap);
}

void DockProbe::setInputChannels(int input, int inputs)
{
    ui->inputSpinBox->setMinimum(-inputs / 2);
    ui->inputSpinBox->setMaximum(inputs / 2);
    ui->inputSpinBox->setValue(input);
}

void DockProbe::pandapterRange_changed(int min, int max)
{
    ui->spectrum->setPandapterRange(min, max);
}

void DockProbe::waterfallRange_changed(int min, int max)
{
    ui->spectrum->setWaterfallRange(min, max);
}

void DockProbe::on_inputSpinBox_valueChanged(int value)
{
    emit inputChanged(value);
    updateCenter();
}

void DockProbe::on_decimSpinBox_valueChanged(int value)
{
    emit decimChanged(value);
    setSampleRate(m_sampleRate);
    updateCenter();
}

void DockProbe::on_osrSpinBox_valueChanged(int value)
{
    emit osrChanged(value);
    updateCenter();
}

void DockProbe::on_fparamSpinBox_valueChanged(double value)
{
    emit filterParamChanged(value);
    updateCenter();
}
