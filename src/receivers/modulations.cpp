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




