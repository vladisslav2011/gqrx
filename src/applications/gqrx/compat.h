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
#ifndef COMPAT_H
#define COMPAT_H
#if GNURADIO_VERSION < 0x030900
    #include <boost/shared_ptr.hpp>
#else
    #include <memory>
#endif

#if GNURADIO_VERSION < 0x030900
    #define compat_shared_ptr boost::shared_ptr
#else
    #define compat_shared_ptr std::shared_ptr
#endif


//rx_fft.h, sniffer_f.h

#if GNURADIO_VERSION < 0x030900
#include <gnuradio/fft/fft.h>
namespace gr
{
    namespace fft
    {
        struct fft_complex_fwd : public fft_complex
        {
            fft_complex_fwd(int fft_size): fft_complex(fft_size, 1)
            {
            }
        };
    }
}
#endif
#if GNURADIO_VERSION >= 0x031000
#include <gnuradio/buffer_reader.h>
#endif
#if GNURADIO_VERSION < 0x031000
#include <gnuradio/buffer.h>
namespace gr
{
    inline static buffer_sptr make_buffer(int buffer_size, int item_size, int dummy0, int dummy1)
    {
        return make_buffer(buffer_size, item_size);
    }
}
#endif

// lpf.h, rx_demod_fm.h

#if GNURADIO_VERSION < 0x030800
#include <gnuradio/filter/fir_filter_fff.h>
#else
#include <gnuradio/filter/fir_filter_blk.h>
#endif

//correct_iq_cc.h
#if GNURADIO_VERSION < 0x030800
#include <gnuradio/blocks/sub_cc.h>
#else
#include <gnuradio/blocks/sub.h>
#endif

//downconverter.h
#if GNURADIO_VERSION < 0x030800
#include <gnuradio/filter/freq_xlating_fir_filter_ccf.h>
#else
#include <gnuradio/filter/freq_xlating_fir_filter.h>
#endif

//fir_decim.h
#if GNURADIO_VERSION < 0x030800
#include <gnuradio/filter/fir_filter_ccf.h>
#else
#include <gnuradio/filter/fir_filter_blk.h>
#endif

//rx_filter.h
#if GNURADIO_VERSION < 0x030800
#include <gnuradio/filter/fir_filter_ccc.h>
#include <gnuradio/filter/freq_xlating_fir_filter_ccc.h>
#else
#include <gnuradio/filter/fir_filter_blk.h>
#include <gnuradio/filter/freq_xlating_fir_filter.h>
#endif

//stereo_demod.h
#if GNURADIO_VERSION < 0x030800
#include <gnuradio/filter/fir_filter_fcc.h>
#include <gnuradio/filter/fir_filter_fff.h>
#include <gnuradio/blocks/multiply_cc.h>
#include <gnuradio/blocks/multiply_ff.h>
#include <gnuradio/blocks/multiply_const_ff.h>
#include <gnuradio/blocks/add_ff.h>
#include <gnuradio/blocks/sub_ff.h>
#else
#include <gnuradio/filter/fir_filter_blk.h>
#include <gnuradio/blocks/multiply.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/add_blk.h>
#include <gnuradio/blocks/sub.h>
#endif

//udp_sink.h
#if GNURADIO_VERSION < 0x031000
#include <gnuradio/blocks/udp_sink.h>
namespace gr
{
    namespace network
    {
        using udp_sink = gr::blocks::udp_sink;
    }
}
#else
#include <gnuradio/network/udp_header_types.h>
#include <gnuradio/network/udp_sink.h>
#endif


#endif //COMPAT_H

//RX_RDS conditional includes

#ifdef RX_RDS_H
#ifndef RX_RDS_COMPAT
#define RX_RDS_COMPAT

#if GNURADIO_VERSION < 0x030800
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/freq_xlating_fir_filter_fcf.h>
#include <gnuradio/digital/clock_recovery_mm_cc.h>
#else
#include <gnuradio/filter/fir_filter_blk.h>
#include <gnuradio/filter/freq_xlating_fir_filter.h>
#include <gnuradio/digital/symbol_sync_cc.h>
#endif

#if GNURADIO_VERSION < 0x30800
#include <gnuradio/filter/rational_resampler_base_ccf.h>
#elif GNURADIO_VERSION < 0x30900
#include <gnuradio/filter/rational_resampler_base.h>
#else
#include <gnuradio/filter/rational_resampler.h>
#endif

#if GNURADIO_VERSION < 0x030900
namespace gr
{
    namespace filter
    {
    using rational_resampler_ccf = rational_resampler_base_ccf;
    }
}
#endif

#endif //RX_RDS_COMPAT

#endif //RX_RDS_H
