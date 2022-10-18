/*
 * fsk/afsk demodulator block implementation header
 *
 * Copyright 2022 Marc CAPDEVILLE F4JMZ
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _FSK_DEMOD_IMPL_H_
#define _FSK_DEMOD_IMPL_H_

#include "fsk_demod.h"
#include <gnuradio/filter/fir_filter.h>
#include <mutex>

namespace gr {
    namespace rtty {
        class fsk_demod_impl : public fsk_demod {
            public:
                fsk_demod_impl(float sample_rate,unsigned int decimation, float symbol_rate, float mark_freq,float space_freq);
                ~fsk_demod_impl();

                int work(int noutput_items,
                         gr_vector_const_void_star &input_items,
                         gr_vector_void_star &output_items);

                void set_sample_rate(float sample_rate);
                float sample_rate() const;

                void set_decimation(unsigned int decimation);
                int decimation() const;

                void set_symbol_rate(float symbol_rate);
                float symbol_rate() const;

                void set_mark_freq(float mark_freq);
                float mark_freq() const;

                void set_space_freq(float space_freq);
                float space_freq() const;

            private:
                std::recursive_mutex d_mutex;

                std::vector<float> d_rrc_taps;

                gr::filter::kernel::fir_filter_ccc d_mark_fir;
                gr::filter::kernel::fir_filter_ccc d_space_fir;

                float d_sample_rate;
                unsigned int d_decimation;
                float d_symbol_rate;
                float d_mark_freq;
                float d_space_freq;
    };
  } // namespace rtty
} // namespace gr

#endif // _FSK_DEMOD_IMPL_H_
