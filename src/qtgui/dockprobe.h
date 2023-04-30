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

    void setNewFftData(float *fftData, int size, qint64 ts);
    void setInvertScrolling(bool enabled);
    void setFftColor(QColor color);
    void setFftFill(bool enabled);
    void setDecimOsr(int,int);
    void setCenterOffset(qint64 freq, qint64 ofs);
    void setSampleRate(int sampleRate);
    void setWfColormap(const QString &cmap);

private:
    void updateCenter();
private:
    Ui::DockProbe *ui;
    int m_sampleRate;
    int m_decim;
    int m_osr;
    qint64 m_offset;
    qint64 m_center;
};

#endif // DOCKPROBE_H
