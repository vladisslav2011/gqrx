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
#ifndef DEFINES_H
#define DEFINES_H

/* Maximum number of receivers */
#define RX_MAX 256


#define TARGET_QUAD_RATE 1e6

/* Number of noice blankers */
#define RECEIVER_NB_COUNT 2

// NB: Remember to adjust filter ranges in MainWindow
#define NB_PREF_QUAD_RATE  96000.f

#define WFM_PREF_QUAD_RATE   240e3 // Nominal channel spacing is 200 kHz

#include <memory>
#include <set>
#include <iostream>
#include "modulations.h"

class vfo_s;
static inline bool operator<(const vfo_s & a, const vfo_s & b);
typedef class vfo_s
{
    public:
    typedef std::shared_ptr<vfo_s> sptr;
    static sptr make()
    {
        return sptr(new vfo_s());
    }
    static sptr make(int _offset, int _low, int _high, Modulations::idx _mode, int _index, bool _locked)
    {
        return sptr(new vfo_s(_offset, _low, _high, _mode, _index, _locked));
    }
    vfo_s():
        offset(0),
        low(0),
        high(0),
        mode(Modulations::MODE_OFF),
        index(0),
        locked(false)
    {
    }
    vfo_s(int _offset, int _low, int _high, Modulations::idx _mode, int _index, bool _locked):
        offset(_offset),
        low(_low),
        high(_high),
        mode(_mode),
        index(_index),
        locked(_locked)
    {
    }
    struct comp
    {
        inline bool operator()(const sptr lhs, const sptr rhs) const
        {
            return (*lhs) < (*rhs);
        }
    };
    typedef std::set<sptr, comp> set;
    int offset;
    int low;
    int high;
    Modulations::idx mode;
    int index;
    bool locked;
} vfo;

static inline bool operator<(const vfo_s &a, const vfo_s &b)
{
    return (a.offset == b.offset) ? (a.index < b.index) : (a.offset < b.offset);
}


#endif // DEFINES_H
