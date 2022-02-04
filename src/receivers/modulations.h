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
#ifndef MODULATIONS_H
#define MODULATIONS_H
#include <iostream>
#include <QStringList>

class Modulations
{
public:
    /**
     * Mode selector entries.
     *
     * @note If you change this enum, remember to update the TCP interface.
     * @note Keep in same order as the Strings in Strings, see
     *       DockRxOpt.cpp constructor.
     */
    enum rxopt_mode_idx {
        MODE_OFF        = 0, /*!< Demodulator completely off. */
        MODE_RAW        = 1, /*!< Raw I/Q passthrough. */
        MODE_AM         = 2, /*!< Amplitude modulation. */
        MODE_AM_SYNC    = 3, /*!< Amplitude modulation (synchronous demod). */
        MODE_LSB        = 4, /*!< Lower side band. */
        MODE_USB        = 5, /*!< Upper side band. */
        MODE_CWL        = 6, /*!< CW using LSB filter. */
        MODE_CWU        = 7, /*!< CW using USB filter. */
        MODE_NFM        = 8, /*!< Narrow band FM. */
        MODE_WFM_MONO   = 9, /*!< Broadcast FM (mono). */
        MODE_WFM_STEREO = 10, /*!< Broadcast FM (stereo). */
        MODE_WFM_STEREO_OIRT = 11, /*!< Broadcast FM (stereo oirt). */
        MODE_LAST       = 12
    };
    typedef enum rxopt_mode_idx idx;

    static QStringList Strings;

    QString GetStringForModulationIndex(int iModulationIndex);
    bool IsModulationValid(QString strModulation);
    idx GetEnumForModulationString(QString param) const;
    Modulations();
    ~Modulations();
};

#endif // MODULATIONS_H
