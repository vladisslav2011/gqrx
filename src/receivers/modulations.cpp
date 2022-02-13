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
#include "receivers/modulations.h"

QStringList Modulations::Strings;
// Lookup table for conversion from old settings
static const int old2new[] = {
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

// Filter preset table per mode, preset and lo/hi
static const int filter_preset_table[Modulations::MODE_LAST][3][2] =
{   //     WIDE             NORMAL            NARROW
    {{      0,      0}, {     0,     0}, {     0,     0}},  // MODE_OFF
    {{ -15000,  15000}, { -5000,  5000}, { -1000,  1000}},  // MODE_RAW
    {{ -10000,  10000}, { -5000,  5000}, { -2500,  2500}},  // MODE_AM
    {{ -10000,  10000}, { -5000,  5000}, { -2500,  2500}},  // MODE_AMSYNC
    {{  -4000,   -100}, { -2800,  -100}, { -2400,  -300}},  // MODE_LSB
    {{    100,   4000}, {   100,  2800}, {   300,  2400}},  // MODE_USB
    {{  -1000,   1000}, {  -250,   250}, {  -100,   100}},  // MODE_CWL
    {{  -1000,   1000}, {  -250,   250}, {  -100,   100}},  // MODE_CWU
    {{ -10000,  10000}, { -5000,  5000}, { -2500,  2500}},  // MODE_NFM
    {{-100000, 100000}, {-80000, 80000}, {-60000, 60000}},  // MODE_WFM_MONO
    {{-100000, 100000}, {-80000, 80000}, {-60000, 60000}},  // MODE_WFM_STEREO
    {{-100000, 100000}, {-80000, 80000}, {-60000, 60000}}   // MODE_WFM_STEREO_OIRT
};

// Filter ranges table per mode
static const int filter_ranges_table[Modulations::MODE_LAST][2][2] =
{   //LOW MIN     MAX  HIGH MIN    MAX
    {{      0,      0}, {     0,     0}},  // MODE_OFF
    {{ -40000,   -200}, {   200, 40000}},  // MODE_RAW
    {{ -40000,   -200}, {   200, 40000}},  // MODE_AM
    {{ -40000,   -200}, {   200, 40000}},  // MODE_AMSYNC
    {{ -40000,   -100}, { -5000,     0}},  // MODE_LSB
    {{      0,   5000}, {   100, 40000}},  // MODE_USB
    {{  -5000,   -100}, {   100,  5000}},  // MODE_CWL
    {{  -5000,   -100}, {   100,  5000}},  // MODE_CWU
    {{ -40000,   -200}, {   200, 40000}},  // MODE_NFM
    {{-120000, -10000}, { 10000,120000}},  // MODE_WFM_MONO
    {{-120000, -10000}, { 10000,120000}},  // MODE_WFM_STEREO
    {{-120000, -10000}, { 10000,120000}}   // MODE_WFM_STEREO_OIRT
};



QString Modulations::GetStringForModulationIndex(int iModulationIndex)
{
    return Modulations::Strings[iModulationIndex];
}

bool Modulations::IsModulationValid(QString strModulation)
{
    return Modulations::Strings.contains(strModulation, Qt::CaseInsensitive);
}

Modulations::idx Modulations::GetEnumForModulationString(QString param) const
{
    int iModulation = -1;
    for(int i=0; i<Modulations::Strings.size(); ++i)
    {
        QString& strModulation = Modulations::Strings[i];
        if(param.compare(strModulation, Qt::CaseInsensitive)==0)
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

bool Modulations::GetFilterPreset(Modulations::idx iModulationIndex, int preset, int& low, int& high) const
{
    if(iModulationIndex >= MODE_LAST)
        iModulationIndex = MODE_AM;
    if(preset == FILTER_PRESET_USER)
        return false;
    low = filter_preset_table[iModulationIndex][preset][0];
    high = filter_preset_table[iModulationIndex][preset][1];
    return true;
}

int Modulations::FindFilterPreset(Modulations::idx mode_index, int lo, int hi) const
{
    if (lo == filter_preset_table[mode_index][FILTER_PRESET_WIDE][0] &&
        hi == filter_preset_table[mode_index][FILTER_PRESET_WIDE][1])
        return FILTER_PRESET_WIDE;
    else if (lo == filter_preset_table[mode_index][FILTER_PRESET_NORMAL][0] &&
             hi == filter_preset_table[mode_index][FILTER_PRESET_NORMAL][1])
        return FILTER_PRESET_NORMAL;
    else if (lo == filter_preset_table[mode_index][FILTER_PRESET_NARROW][0] &&
             hi == filter_preset_table[mode_index][FILTER_PRESET_NARROW][1])
        return FILTER_PRESET_NARROW;

    return FILTER_PRESET_USER;
}

void Modulations::GetFilterRanges(Modulations::idx iModulationIndex, int& lowMin, int& lowMax, int& highMin, int& highMax) const
{
    if(iModulationIndex >= MODE_LAST)
        iModulationIndex = MODE_AM;
    lowMin = filter_ranges_table[iModulationIndex][0][0];
    lowMax = filter_ranges_table[iModulationIndex][0][1];
    highMin = filter_ranges_table[iModulationIndex][1][0];
    highMax = filter_ranges_table[iModulationIndex][1][1];
}

bool Modulations::UpdateFilterRange(Modulations::idx iModulationIndex, int& low, int& high) const
{
    bool updated = false;
    if(iModulationIndex >= MODE_LAST)
        iModulationIndex = MODE_AM;
    if(low < filter_ranges_table[iModulationIndex][0][0])
    {
        low = filter_ranges_table[iModulationIndex][0][0];
        updated = true;
    }
    if(low > filter_ranges_table[iModulationIndex][0][1])
    {
        low = filter_ranges_table[iModulationIndex][0][1];
        updated = true;
    }
    if(high < filter_ranges_table[iModulationIndex][1][0])
    {
        high = filter_ranges_table[iModulationIndex][1][0];
        updated = true;
    }
    if(high > filter_ranges_table[iModulationIndex][1][1])
    {
        high = filter_ranges_table[iModulationIndex][1][1];
        updated = true;
    }
    return updated;
}

Modulations::Modulations()
{
    if (Modulations::Strings.size() == 0)
    {
        // Keep in sync with rxopt_mode_idx and filter_preset_table
        Modulations::Strings.append("Demod Off");
        Modulations::Strings.append("Raw I/Q");
        Modulations::Strings.append("AM");
        Modulations::Strings.append("AM-Sync");
        Modulations::Strings.append("LSB");
        Modulations::Strings.append("USB");
        Modulations::Strings.append("CW-L");
        Modulations::Strings.append("CW-U");
        Modulations::Strings.append("Narrow FM");
        Modulations::Strings.append("WFM (mono)");
        Modulations::Strings.append("WFM (stereo)");
        Modulations::Strings.append("WFM (oirt)");
    }
}

Modulations::~Modulations()
{
}
