/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
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
#ifndef AUDIO_DEVICE_LIST_H
#define AUDIO_DEVICE_LIST_H


#ifdef WITH_PULSEAUDIO
    #include "pulseaudio/pa_device_list.h"
    using audio_device = pa_device;
#elif WITH_PORTAUDIO
    #include "portaudio/device_list.h"
    using audio_device = portaudio_device;
#elif defined(Q_OS_DARWIN)
    #include "osxaudio/device_list.h"
    using audio_device = osxaudio_device;
#else
    using audio_device = int;
#endif

#if defined(Q_OS_DARWIN)
    #ifdef WITH_PORTAUDIO
        using audio_device_list = portaudio_device_list;
    #else
        using audio_device_list = osxaudio_device_list;
    #endif
#else
    #ifdef WITH_PULSEAUDIO
        using audio_device_list = pa_device_list;
    #elif WITH_PORTAUDIO
        using audio_device_list = portaudio_device_list;
    #endif
#endif

#endif //AUDIO_DEVICE_LIST_H
