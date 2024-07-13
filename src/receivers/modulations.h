/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2016 Alexandru Csete OZ9AEC.
 * Copyright 2022 vladisslav2011@gmail.com.
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
#include <array>
#include <QStringList>

//FIXME: Convert to enum?
#define FILTER_PRESET_WIDE      0
#define FILTER_PRESET_NORMAL    1
#define FILTER_PRESET_NARROW    2
#define FILTER_PRESET_USER      3
#define FILTER_PRESET_COUNT     3

class Modulations
{
public:
    /** Filter shape (convenience wrappers for "transition width"). */
    enum filter_shape {
        FILTER_SHAPE_SOFT = 0,   /*!< Soft: Transition band is TBD of width. */
        FILTER_SHAPE_NORMAL = 1, /*!< Normal: Transition band is TBD of width. */
        FILTER_SHAPE_SHARP = 2,   /*!< Sharp: Transition band is TBD of width. */
        FILTER_SHAPE_COUNT = 3
    };
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
        MODE_NFMPLL     = 9, /*!< Narrow band FM PLL. */
        MODE_WFM_MONO   = 10, /*!< Broadcast FM (mono). */
        MODE_WFM_STEREO = 11, /*!< Broadcast FM (stereo). */
        MODE_WFM_STEREO_OIRT = 12, /*!< Broadcast FM (stereo oirt). */
        MODE_COUNT       = 13
    };
    enum rxopt_mode_group {
        GRP_OFF=0,
        GRP_RAW,
        GRP_AM,
        GRP_AM_SYNC,
        GRP_SSB,
        GRP_CW,
        GRP_NFM,
        GRP_NFMPLL,
        GRP_WFM_MONO,
        GRP_WFM_STEREO,
        GRP_WFM_STEREO_OIRT,
        GRP_COUNT
    };
    typedef enum rxopt_mode_idx idx;
    typedef enum rxopt_mode_group grp;
    struct filter_preset
    {
        int lo;
        int hi;
    };
    struct freq_range
    {
        int min;
        int max;
    };
    struct freq_ranges
    {
        freq_range lo;
        freq_range hi;
    };
    typedef filter_preset filter_presets[FILTER_PRESET_COUNT];
    struct mode
    {
        grp group;
        const char * shortcut;
        const char * name;
        int click_res;
        int fft_lo;
        int fft_hi;
        filter_presets presets;
        freq_ranges ranges;
    };
    static constexpr std::array<mode, MODE_COUNT> modes
    {(const mode)
        {GRP_OFF,"!","Demod Off",
            10,
            -24000,24000,
            {{      0,      0}, {     0,     0}, {     0,     0}},  // MODE_OFF
            {{      0,      0}, {     0,     0}},  // MODE_OFF
        },
        {GRP_RAW,"I","Raw I/Q",
            100,
            -24000,24000,
            {{ -15000,  15000}, { -5000,  5000}, { -1000,  1000}},  // MODE_RAW
            {{ -40000,   -200}, {   200, 40000}},  // MODE_RAW
        },
        {GRP_AM,"A","AM",
            100,
            0,6000,
            {{ -10000,  10000}, { -5000,  5000}, { -2500,  2500}},  // MODE_AM
            {{ -40000,   -200}, {   200, 40000}},  // MODE_AM
        },
        {GRP_AM_SYNC,"Shift+A","AM-Sync",
            100,
            0,6000,
            {{ -10000,  10000}, { -5000,  5000}, { -2500,  2500}},  // MODE_AMSYNC
            {{ -40000,   -200}, {   200, 40000}},  // MODE_AMSYNC
        },
        {GRP_SSB,"S","LSB",
            100,
            0,3000,
            {{  -4000,   -100}, { -2800,  -100}, { -2400,  -300}},  // MODE_LSB
            {{ -40000,   -100}, { -5000,     0}},  // MODE_LSB
        },
        {GRP_SSB,"Shift+S","USB",
            100,
            0,3000,
            {{    100,   4000}, {   100,  2800}, {   300,  2400}},  // MODE_USB
            {{      0,   5000}, {   100, 40000}},  // MODE_USB
        },
        {GRP_CW,"C","CW-L",
            10,
            0,1500,
            {{  -1000,   1000}, {  -250,   250}, {  -100,   100}},  // MODE_CWL
            {{  -5000,   -100}, {   100,  5000}},  // MODE_CWL
        },
        {GRP_CW,"Shift+C","CW-U",
            10,
            0,1500,
            {{  -1000,   1000}, {  -250,   250}, {  -100,   100}},  // MODE_CWU
            {{  -5000,   -100}, {   100,  5000}},  // MODE_CWU
        },
        {GRP_NFM,"N","Narrow FM",
            100,
            0,5000,
            {{ -10000,  10000}, { -5000,  5000}, { -2500,  2500}},  // MODE_NFM
            {{ -40000,   -200}, {   200, 40000}},  // MODE_NFM
        },
        {GRP_NFMPLL,"Shift+N","NFM PLL",
            100,
            0,5000,
            {{ -10000,  10000}, { -5000,  5000}, { -2500,  2500}},  // MODE_NFMPLL
            {{ -40000,   -200}, {   200, 40000}},  // MODE_NFMPLL
        },
        {GRP_WFM_MONO,"W","WFM (mono)",
            1000,
            0,24000,
            {{-100000, 100000}, {-80000, 80000}, {-60000, 60000}},  // MODE_WFM_MONO
            {{-120000, -10000}, { 10000,120000}},  // MODE_WFM_MONO
        },
        {GRP_WFM_STEREO,"Shift+W","WFM (stereo)",
            1000,
            0,24000,
            {{-100000, 100000}, {-80000, 80000}, {-60000, 60000}},  // MODE_WFM_STEREO
            {{-120000, -10000}, { 10000,120000}},  // MODE_WFM_STEREO
        },
        {GRP_WFM_STEREO_OIRT,"O","WFM (oirt)",
            1000,
            0,24000,
            {{-100000, 100000}, {-80000, 80000}, {-60000, 60000}},  // MODE_WFM_STEREO_OIRT
            {{-120000, -10000}, { 10000,120000}},  // MODE_WFM_STEREO_OIRT
        },
    };

    static constexpr const char * GetStringForModulationIndex(int iModulationIndex)
    {
        return modes[iModulationIndex].name;
    }
    static bool IsModulationValid(QString strModulation);
    static idx GetEnumForModulationString(QString param);
    static idx ConvertFromOld(int old);
    static bool GetFilterPreset(idx iModulationIndex, int preset, int& low, int& high);
    static int FindFilterPreset(idx mode_index, int lo, int hi);
    static void GetFilterRanges(idx iModulationIndex, int& lowMin, int& lowMax, int& highMin, int& highMax);
    static bool IsFilterSymmetric(idx iModulationIndex);
    static void UpdateFilterRange(idx iModulationIndex, int& low, int& high);
    static bool UpdateTw(const int low, const int high, int& tw);
    static filter_shape FilterShapeFromTw(const int low, const int high, const int tw);
    static int TwFromFilterShape(const int low, const int high, const filter_shape shape);
};

#endif // MODULATIONS_H
