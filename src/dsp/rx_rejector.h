/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2024 Vladislav P.
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
#ifndef RX_REJECTOR_H
#define RX_REJECTOR_H

#include <gnuradio/blocks/control_loop.h>
#include <gnuradio/sync_block.h>
#include <gnuradio/math.h>

/*! \brief Naroow-band PLL-aided interference rejector
 *  \ingroup DSP
 *
 */
class rx_rejector_cc : virtual public gr::sync_block,
                       virtual public gr::blocks::control_loop
{
public:
    typedef std::function<void(float freq)> freq_event_t;
#if GNURADIO_VERSION < 0x030900
    typedef boost::shared_ptr<rx_rejector_cc> sptr;
#else
    typedef std::shared_ptr<rx_rejector_cc> sptr;
#endif
/*! \brief Return a shared_ptr to a new instance of rx_rejector.
 *  \param sample_rate The sample rate.
 *  \param offset The filter offset.
 *  \param bw The PLL locking range.
 *  \param alfa The high-pass IIR alfa.
 *
 */
    static sptr make(double sample_rate,
                     double offset=0.0,
                     double bw=5.0,
                     double alfa=0.001);

    ~rx_rejector_cc();
  int work( int noutput_items,
            gr_vector_const_void_star &input_items,
            gr_vector_void_star &output_items );

    void set_sample_rate(double rate);
    void set_offset(double offset);
    void set_bw(double bw);
    void set_alfa(double alfa);
    float get_freq()
    {
        return float(d_sample_rate) * d_freq * (.5f/float(M_PI));
    };
    template <typename T> void set_freq_event_handler(T handler)
    {
        d_freq_event = handler;
    }

private:
    rx_rejector_cc(double sample_rate=96000.0, double offset=0.0, double bw=5.0, double alfa=0.001);
      float mod_2pi(float in)
    {
        if (in > float(M_PI))
            return in - (2.f * float(M_PI));
        else if (in < -float(M_PI))
            return in + (2.f * float(M_PI));
        else
            return in;
    }

    float phase_detector(gr_complex sample, float ref_phase)
    {
        float sample_phase;
        //  sample_phase = atan2(sample.imag(),sample.real());
        sample_phase = gr::fast_atan2f(sample.imag(), sample.real());
        return mod_2pi(sample_phase - ref_phase);
    }
    gr_complex            d_accum;
    float                 d_iir_alfa;
    gr_complex            d_preaccum;
    float                 d_pre_alfa;
    double                d_sample_rate;
    double                d_offset;
    double                d_bw;
    float                 d_filt_freq{0.f};
    int                   d_counter{0};
    freq_event_t          d_freq_event{nullptr};
};

#endif // RX_REJECTOR_H
