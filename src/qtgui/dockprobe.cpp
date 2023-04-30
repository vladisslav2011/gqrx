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

#define DEFAULT_FFT_SPLIT 80

DockProbe::DockProbe(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockProbe),
    m_sampleRate(20000),
    m_decim(1),
    m_osr(1),
    m_offset(0),
    m_center(0)
{
    ui->setupUi(this);

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
    ui->spectrum->enableBandPlan(false);
    ui->spectrum->setFftRange(-100., -40.);
    ui->spectrum->setVdivDelta(40);
    ui->spectrum->setFreqDigits(1);
    ui->spectrum->setRunningState(true);

}

DockProbe::~DockProbe()
{
    delete ui;
}

void DockProbe::setDecimOsr(int decim, int osr)
{
    m_decim = decim;
    m_osr = osr;
    updateCenter();
}

void DockProbe::setCenterOffset(qint64 freq, qint64 ofs)
{
    m_center = freq;
    m_offset = ofs;
    updateCenter();
}

void DockProbe::updateCenter()
{
    qint64 offs = (m_offset>=0)?m_sampleRate/2:m_sampleRate/-2;
    qint64 ch=(m_offset*m_decim+offs)/m_sampleRate;
    qint64 ch_width=m_sampleRate/m_decim;
    ui->spectrum->setCenterFreq(m_center+ch_width*ch);
    ui->spectrum->setSampleRate(m_sampleRate/m_decim);
    ui->spectrum->setSpanFreq(m_sampleRate/m_decim);
}

void DockProbe::setNewFftData(float *fftData, int size, qint64 ts)
{
    ui->spectrum->setNewFftData(fftData, size, ts);
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
    ui->spectrum->enableFftFill(enabled);
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
