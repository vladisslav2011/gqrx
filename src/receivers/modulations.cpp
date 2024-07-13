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
#include "receivers/defines.h"
#include "receivers/modulations.h"


constexpr std::array<Modulations::mode, Modulations::MODE_COUNT> Modulations::modes;

// Lookup table for conversion from old settings
static constexpr Modulations::idx old2new[] = {
    Modulations::MODE_OFF,
    Modulations::MODE_RAW,
    Modulations::MODE_AM,
    Modulations::MODE_NFM,
    Modulations::MODE_WFM_MONO,
    Modulations::MODE_WFM_STEREO,
    Modulations::MODE_LSB,
    Modulations::MODE_USB,
    Modulations::MODE_CWL,
    Modulations::MODE_CWU,
    Modulations::MODE_WFM_STEREO_OIRT,
    Modulations::MODE_AM_SYNC
};

bool Modulations::IsModulationValid(QString strModulation)
{
    for(auto & mode: modes)
    {
        const QString& strModulation = mode.name;
        if (strModulation.compare(strModulation, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

Modulations::idx Modulations::GetEnumForModulationString(QString param)
{
    int iModulation = -1;
    for(int i = 0; i < Modulations::MODE_COUNT; ++i)
    {
        const QString& strModulation = modes[i].name;
        if (param.compare(strModulation, Qt::CaseInsensitive) == 0)
        {
            iModulation = i;
            break;
        }
    }
    if(iModulation == -1)
    {
        std::cout << "Modulation '" << param.toStdString() << "' is unknown." << std::endl;
        iModulation = MODE_OFF;
    }
    return idx(iModulation);
}

bool Modulations::GetFilterPreset(Modulations::idx iModulationIndex, int preset, int& low, int& high)
{
    if (iModulationIndex >= MODE_COUNT)
        iModulationIndex = MODE_AM;
    if (preset == FILTER_PRESET_USER)
        return false;
    low = modes[iModulationIndex].presets[preset].lo;
    high = modes[iModulationIndex].presets[preset].hi;
    return true;
}

int Modulations::FindFilterPreset(Modulations::idx iModulationIndex, int lo, int hi)
{
    if (lo == modes[iModulationIndex].presets[FILTER_PRESET_WIDE].lo &&
        hi == modes[iModulationIndex].presets[FILTER_PRESET_WIDE].hi)
        return FILTER_PRESET_WIDE;
    else if (lo == modes[iModulationIndex].presets[FILTER_PRESET_NORMAL].lo &&
             hi == modes[iModulationIndex].presets[FILTER_PRESET_NORMAL].hi)
        return FILTER_PRESET_NORMAL;
    else if (lo == modes[iModulationIndex].presets[FILTER_PRESET_NARROW].lo &&
             hi == modes[iModulationIndex].presets[FILTER_PRESET_NARROW].hi)
        return FILTER_PRESET_NARROW;

    return FILTER_PRESET_USER;
}

void Modulations::GetFilterRanges(Modulations::idx iModulationIndex, int& lowMin, int& lowMax, int& highMin, int& highMax)
{
    if (iModulationIndex >= MODE_COUNT)
        iModulationIndex = MODE_AM;
    lowMin = modes[iModulationIndex].ranges.lo.min;
    lowMax = modes[iModulationIndex].ranges.lo.max;
    highMin = modes[iModulationIndex].ranges.hi.min;
    highMax = modes[iModulationIndex].ranges.hi.max;
}

bool Modulations::IsFilterSymmetric(Modulations::idx iModulationIndex)
{
    if (iModulationIndex >= MODE_COUNT)
        iModulationIndex = MODE_AM;
    return (-modes[iModulationIndex].ranges.lo.min == modes[iModulationIndex].ranges.hi.max);
}

void Modulations::UpdateFilterRange(Modulations::idx iModulationIndex, int& low, int& high)
{
    if (iModulationIndex >= MODE_COUNT)
        iModulationIndex = MODE_AM;
    #if 0
    if (IsFilterSymmetric(iModulationIndex))
        if (high != (high - low) / 2)
        {
            if (high > -low)
                low = -high;
            else
                high = -low;
        }
    #endif
    if (low < modes[iModulationIndex].ranges.lo.min)
        low = modes[iModulationIndex].ranges.lo.min;
    if (low > modes[iModulationIndex].ranges.lo.max)
        low = modes[iModulationIndex].ranges.lo.max;
    if (high < modes[iModulationIndex].ranges.hi.min)
        high = modes[iModulationIndex].ranges.hi.min;
    if (high > modes[iModulationIndex].ranges.hi.max)
        high = modes[iModulationIndex].ranges.hi.max;
}

Modulations::idx Modulations::ConvertFromOld(int old)
{
    if (old < 0)
        return old2new[0];
    if (old >= int(sizeof(old2new) / sizeof(old2new[0])))
        return old2new[2];
    return old2new[old];
}

bool Modulations::UpdateTw(const int low, const int high, int& tw)
{
    int sharp = std::abs(high - low) * 0.1;
    if (tw == sharp)
        return false;
    if (tw < std::abs(high - low) * 0.15)
    {
        tw = sharp;
        return true;
    }
    int normal = std::abs(high - low) * 0.2;
    if (tw == normal)
        return false;
    if (tw < std::abs(high - low) * 0.25)
    {
        tw = normal;
        return true;
    }
    int soft = std::abs(high - low) * 0.5;
    if(tw == soft)
        return false;
    tw = soft;
    return true;
}

Modulations::filter_shape Modulations::FilterShapeFromTw(const int low, const int high, const int tw)
{
    Modulations::filter_shape shape = FILTER_SHAPE_SOFT;

    if (tw < std::abs(high - low) * 0.25)
        shape = FILTER_SHAPE_NORMAL;
    if (tw < std::abs(high - low) * 0.15)
        shape = FILTER_SHAPE_SHARP;

    return shape;
}

int Modulations::TwFromFilterShape(const int low, const int high, const Modulations::filter_shape shape)
{
    float trans_width = RX_FILTER_MIN_WIDTH * 0.1;
    if ((low >= high) || (std::abs(high - low) < RX_FILTER_MIN_WIDTH))
        return trans_width;

    switch (shape) {

    case Modulations::FILTER_SHAPE_SOFT:
        trans_width = std::abs(high - low) * 0.5;
        break;

    case Modulations::FILTER_SHAPE_SHARP:
        trans_width = std::abs(high - low) * 0.1;
        break;

    case Modulations::FILTER_SHAPE_NORMAL:
    default:
        trans_width = std::abs(high - low) * 0.2;
        break;

    }

    return trans_width;
}

Modulations::rx_chain Modulations::get_rxc(Modulations::idx demod)
{
    switch (demod)
    {
    case MODE_OFF:
        return RX_CHAIN_NONE;
        break;

    case MODE_RAW:
    case MODE_AM:
    case MODE_AM_SYNC:
    case MODE_NFM:
    case MODE_NFMPLL:
    case MODE_LSB:
    case MODE_USB:
    case MODE_CWL:
    case MODE_CWU:
        return RX_CHAIN_NBRX;
        break;

    case MODE_WFM_MONO:
    case MODE_WFM_STEREO:
    case MODE_WFM_STEREO_OIRT:
        return RX_CHAIN_WFMRX;
        break;

    default:
        return rx_chain(-1);
        break;
    }
}


