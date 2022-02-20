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
#ifndef DOCKPROBE_H
#define DOCKPROBE_H

#include <QColor>
#include <QDockWidget>
#include <QSettings>

namespace Ui {
    class DockProbe;
}


class DockProbe : public QDockWidget
{
    Q_OBJECT

public:
    explicit DockProbe(QWidget *parent = 0);
    ~DockProbe();

    void setFftRange(quint64 minf, quint64 maxf);
    void setNewFftData(float *fftData, int size);
    void setInvertScrolling(bool enabled);
    int  fftRate() const { return 10; }

    void setFftColor(QColor color);
    void setFftFill(bool enabled);

public slots:
    void setCenterFreq(qint64 freq);
    void setSampleRate(int sampleRate);
    void setWfColormap(const QString &cmap);
    void setInputChannels(int input, int inputs);
signals:
    /*! \brief FFT rate changed. */
    void fftRateChanged(int fps);

    /*! \brief Signal emitted when input is changed. */
    void inputChanged(int n);
    void decimChanged(int n);
    void osrChanged(int n);
    void filterParamChanged(float n);

private slots:
    void pandapterRange_changed(int min, int max);
    void waterfallRange_changed(int min, int max);
    void on_inputSpinBox_valueChanged(int value);
    void on_decimSpinBox_valueChanged(int value);
    void on_osrSpinBox_valueChanged(int value);
    void on_fparamSpinBox_valueChanged(double value);

private:
    void updateCenter();
private:
    Ui::DockProbe *ui;
    int m_sampleRate;
    qint64 m_center;
};

#endif // DOCKPROBE_H
