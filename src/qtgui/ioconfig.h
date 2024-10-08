/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2014 Alexandru Csete OZ9AEC.
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
#ifndef IOCONFIG_H
#define IOCONFIG_H

#include <QDialog>
#include <QSettings>
#include <QString>
#include <vector>
#include "audio_device_list.h"


namespace Ui {
    class CIoConfig;
}

/** @brief Input/output device configurator. */
class CIoConfig : public QDialog
{
    Q_OBJECT

public:
    explicit CIoConfig(QSettings *settings, std::map<QString, QVariant> &devList, QWidget *parent = 0);
    virtual ~CIoConfig();
    static void getDeviceList(std::map<QString, QVariant> &devList);

private slots:
    void saveConfig();
    void inputDeviceSelected(int index);
    void inputDevstrChanged(const QString &text);
    void inputRateChanged(const QString &text);
    void decimationChanged(int index);
    void onScanButtonClicked();

private:
    void updateInputSampleRates(int rate);
    void updateDecimations(void);
    void updateInDev(const QSettings *settings, const std::map<QString, QVariant> &devList);
    void updateOutDev();
    int  idx2decim(int idx) const;
    int  decim2idx(int decim) const;
    static std::string escapeDevstr(std::string devstr);

private:
    Ui::CIoConfig  *ui;
    QSettings      *m_settings;
    QPushButton    *m_scanButton;
    std::map<QString, int> recent_devs;
    std::map<QString, QVariant> *m_devList; // will point to devList from constructor

    std::vector<audio_device>           outDevList;
};

#endif // IOCONFIG_H
