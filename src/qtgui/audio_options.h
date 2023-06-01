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
#ifndef AUDIO_OPTIONS_H
#define AUDIO_OPTIONS_H

#include <QCloseEvent>
#include <QDialog>
#include <QDir>
#include <QPalette>
#include <QShowEvent>
#include "audio_device_list.h"
#include "applications/gqrx/dcontrols_ui.h"

namespace Ui {
    class CAudioOptions;
}

/*! \brief GUI widget for configuring audio options. */
class CAudioOptions : public QDialog, public dcontrols_ui_tabbed
{
    Q_OBJECT

public:
    explicit CAudioOptions(QWidget *parent = 0);
    ~CAudioOptions();

    void closeEvent(QCloseEvent *event) override;

private:
    void showEvent(QShowEvent *event) override;
    void updateOutDev();
    void audioFFTLockObserver(const c_id id, const c_def::v_union &value);
    void recDirObserver(const c_id id, const c_def::v_union &value);
    void recDirButtonObserver(const c_id id, const c_def::v_union &value);
    void dedicatedAudioSinkObserver(const c_id id, const c_def::v_union &value);

    Ui::CAudioOptions *ui;                   /*!< The user interface widget. */
    QDir              *work_dir;             /*!< Used for validating chosen directory. */
    QPalette          *error_palette;        /*!< Palette used to indicate an error. */
    bool               m_pand_last_modified; /*!< Flag to indicate which slider was changed last */
    std::vector<audio_device> outDevList;
};

#endif // AUDIO_OPTIONS_H
