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
#ifndef AUDIO_SINK_H
#define AUDIO_SINK_H

#ifdef WITH_PULSEAUDIO
#include "pulseaudio/pa_sink.h"
#elif WITH_PORTAUDIO
#include "portaudio/portaudio_sink.h"
#else
#include <gnuradio/audio/sink.h>
#endif

#include <gnuradio/basic_block.h>
#include <string>


static inline gr::basic_block_sptr create_audio_sink(std::string audio_device, int audio_rate, std::string sink_name)
{
#ifdef WITH_PULSEAUDIO
    return make_pa_sink(audio_device, audio_rate, "GQRX", sink_name);
#elif WITH_PORTAUDIO
    return  make_portaudio_sink(audio_device, audio_rate, "GQRX", sink_name);
#else
    return  gr::audio::sink::make(audio_rate, audio_device, true);
#endif

}

#endif //AUDIO_SINK_H

