/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2013 Alexandru Csete OZ9AEC.
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
#include <math.h>
#include <volk/volk.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/gr_complex.h>
#include <gnuradio/fft/fft.h>
#include <gnuradio/thread/thread.h>
#include "dsp/rx_fft.h"
#include "receivers/defines.h"
#include <algorithm>


rx_fft_c_sptr make_rx_fft_c (unsigned int fftsize, double quad_rate, int wintype)
{
    return gnuradio::get_initial_sptr(new rx_fft_c (fftsize, quad_rate, wintype));
}

/*! \brief Create receiver FFT object.
 *  \param fftsize The FFT size.
 *  \param wintype The window type (see gr::fft::window::win_type).
 *
 */
rx_fft_c::rx_fft_c(unsigned int fftsize, double quad_rate, int wintype)
    : gr::sync_block ("rx_fft_c",
          gr::io_signature::make(1, 1, sizeof(gr_complex)),
          gr::io_signature::make(0, 0, 0)),
      d_fftsize(fftsize),
      d_quadrate(quad_rate),
      d_wintype(-1)
{

    /* create FFT object */
#if GNURADIO_VERSION < 0x030900
    d_fft = new gr::fft::fft_complex(d_fftsize, true);
#else
    d_fft = new gr::fft::fft_complex_fwd(d_fftsize);
#endif

    /* allocate circular buffer */
#if GNURADIO_VERSION < 0x031000
    d_writer = gr::make_buffer(MAX_FFT_SIZE * 2, sizeof(gr_complex));
#else
    d_writer = gr::make_buffer(MAX_FFT_SIZE * 2, sizeof(gr_complex), 1, 1);
#endif
    d_reader = gr::buffer_add_reader(d_writer, 0);

    memset(d_writer->write_pointer(), 0, sizeof(gr_complex) * MAX_FFT_SIZE);
    d_writer->update_write_pointer(MAX_FFT_SIZE);

    /* create FFT window */
    set_window_type(wintype);

    d_lasttime = std::chrono::steady_clock::now();
}

rx_fft_c::~rx_fft_c()
{
    delete d_fft;
}

/*! \brief Receiver FFT work method.
 *  \param noutput_items
 *  \param input_items
 *  \param output_items
 *
 * This method does nothing except throwing the incoming samples into the
 * circular buffer.
 * FFT is only executed when the GUI asks for new FFT data via get_fft_data().
 */
int rx_fft_c::work(int noutput_items,
                   gr_vector_const_void_star &input_items,
                   gr_vector_void_star &output_items)
{
    const gr_complex *in = (const gr_complex*)input_items[0];
    (void) output_items;

    /* just throw new samples into the buffer */
    std::lock_guard<std::mutex> lock(d_mutex);

    int items_to_copy = std::min(noutput_items, (int)d_writer->bufsize());
    if (items_to_copy < noutput_items)
        in += (noutput_items - items_to_copy);

    if (d_writer->space_available() < items_to_copy)
        d_reader->update_read_pointer(items_to_copy - d_writer->space_available());
    memcpy(d_writer->write_pointer(), in, sizeof(gr_complex) * items_to_copy);
    d_writer->update_write_pointer(items_to_copy);

    return noutput_items;
}

/*! \brief Get FFT data.
 *  \param fftPoints Buffer to copy FFT data
 *  \param fftSize Current FFT size (output).
 */
void rx_fft_c::get_fft_data(std::complex<float>* fftPoints, unsigned int &fftSize)
{
    std::unique_lock<std::mutex> lock(d_mutex);

    std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = now - d_lasttime;
    diff = std::min(diff, std::chrono::duration<double>(d_writer->bufsize() / d_quadrate));
    d_lasttime = now;

    /* perform FFT */
    d_reader->update_read_pointer(std::min((int)(diff.count() * d_quadrate * 1.001), d_reader->items_available() - MAX_FFT_SIZE));
    apply_window(d_fftsize);
    lock.unlock();

    /* compute FFT */
    d_fft->execute();

    /* get FFT data */
    memcpy(fftPoints, d_fft->get_outbuf(), sizeof(gr_complex)*d_fftsize);
    fftSize = d_fftsize;
}

/*! \brief Compute FFT on the available input data.
 *  \param data_in The data to compute FFT on.
 *  \param size The size of data_in.
 *
 * Note that this function does not lock the mutex since the caller, get_fft_data()
 * has already locked it.
 */
void rx_fft_c::apply_window(unsigned int size)
{
    /* apply window, if any */
    gr_complex * p = (gr_complex *)d_reader->read_pointer();
    p += (MAX_FFT_SIZE - d_fftsize);
    if (d_window.size())
    {
        gr_complex *dst = d_fft->get_inbuf();
        volk_32fc_32f_multiply_32fc(dst, p, &d_window[0], size);
    }
    else
    {
        memcpy(d_fft->get_inbuf(), p, sizeof(gr_complex)*size);
    }
}

/*! \brief Update circular buffer and FFT object. */
void rx_fft_c::set_params()
{
    /* reset window */
    int wintype = d_wintype; // FIXME: would be nicer with a window_reset()
    d_wintype = -1;
    set_window_type(wintype);

    /* reset FFT object (also reset FFTW plan) */
    delete d_fft;
#if GNURADIO_VERSION < 0x030900
    d_fft = new gr::fft::fft_complex(d_fftsize, true);
#else
    d_fft = new gr::fft::fft_complex_fwd(d_fftsize);
#endif
}

/*! \brief Set new FFT size. */
void rx_fft_c::set_fft_size(unsigned int fftsize)
{
    if (fftsize != d_fftsize)
    {
        d_fftsize = fftsize;
        set_params();
    }

}

/*! \brief Set new quadrature rate. */
void rx_fft_c::set_quad_rate(double quad_rate)
{
    if (quad_rate != d_quadrate) {
        d_quadrate = quad_rate;
        set_params();
    }
}

/*! \brief Get currently used FFT size. */
unsigned int rx_fft_c::get_fft_size() const
{
    return d_fftsize;
}

/*! \brief Set new window type. */
void rx_fft_c::set_window_type(int wintype)
{
    if (wintype == d_wintype)
    {
        /* nothing to do */
        return;
    }

    d_wintype = wintype;

    if ((d_wintype < gr::fft::window::WIN_HAMMING) || (d_wintype > gr::fft::window::WIN_FLATTOP))
    {
        d_wintype = gr::fft::window::WIN_HAMMING;
    }

    d_window.clear();
    d_window = gr::fft::window::build((gr::fft::window::win_type)d_wintype, d_fftsize, 6.76);
}

/*! \brief Get currently used window type. */
int rx_fft_c::get_window_type() const
{
    return d_wintype;
}


/**   rx_fft_f     **/

rx_fft_f_sptr make_rx_fft_f(unsigned int fftsize, double audio_rate, int wintype)
{
    return gnuradio::get_initial_sptr(new rx_fft_f (fftsize, audio_rate, wintype));
}

/*! \brief Create receiver FFT object.
 *  \param fftsize The FFT size.
 *  \param wintype The window type (see gr::fft::window::win_type).
 *
 */
rx_fft_f::rx_fft_f(unsigned int fftsize, double audio_rate, int wintype)
    : gr::sync_block ("rx_fft_f",
          gr::io_signature::make(1, 1, sizeof(float)),
          gr::io_signature::make(0, 0, 0)),
      d_fftsize(fftsize),
      d_audiorate(audio_rate),
      d_wintype(-1)
{

    /* create FFT object */
#if GNURADIO_VERSION < 0x030900
    d_fft = new gr::fft::fft_complex(d_fftsize, true);
#else
    d_fft = new gr::fft::fft_complex_fwd(d_fftsize);
#endif

    /* allocate circular buffer */
#if GNURADIO_VERSION < 0x031000
    d_writer = gr::make_buffer(AUDIO_BUFFER_SIZE, sizeof(float));
#else
    d_writer = gr::make_buffer(AUDIO_BUFFER_SIZE, sizeof(float), 1, 1);
#endif
    d_reader = gr::buffer_add_reader(d_writer, 0);

    memset(d_writer->write_pointer(), 0, sizeof(gr_complex) * d_fftsize);
    d_writer->update_write_pointer(d_fftsize);

    /* create FFT window */
    set_window_type(wintype);

    d_lasttime = std::chrono::steady_clock::now();
}

rx_fft_f::~rx_fft_f()
{
    delete d_fft;
}

/*! \brief Audio FFT work method.
 *  \param noutput_items
 *  \param input_items
 *  \param output_items
 *
 * This method does nothing except throwing the incoming samples into the
 * circular buffer.
 * FFT is only executed when the GUI asks for new FFT data via get_fft_data().
 */
int rx_fft_f::work(int noutput_items,
                   gr_vector_const_void_star &input_items,
                   gr_vector_void_star &output_items)
{
    const float *in = (const float*)input_items[0];
    (void) output_items;

    /* just throw new samples into the buffer */
    std::lock_guard<std::mutex> lock(d_mutex);

    int items_to_copy = std::min(noutput_items, (int)d_writer->bufsize());
    if (items_to_copy < noutput_items)
        in += (noutput_items - items_to_copy);

    if (d_writer->space_available() < items_to_copy)
        d_reader->update_read_pointer(items_to_copy - d_writer->space_available());
    memcpy(d_writer->write_pointer(), in, sizeof(float) * items_to_copy);
    d_writer->update_write_pointer(items_to_copy);

    return noutput_items;
}

/*! \brief Get FFT data.
 *  \param fftPoints Buffer to copy FFT data
 *  \param fftSize Current FFT size (output).
 */
void rx_fft_f::get_fft_data(std::complex<float>* fftPoints, unsigned int &fftSize)
{
    std::unique_lock<std::mutex> lock(d_mutex);

    std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = now - d_lasttime;
    diff = std::min(diff, std::chrono::duration<double>(d_writer->bufsize() / d_audiorate));
    d_lasttime = now;

    /* perform FFT */
    d_reader->update_read_pointer(std::min((unsigned int)(diff.count() * d_audiorate * 1.001), d_reader->items_available() - d_fftsize));
    apply_window(d_fftsize);
    lock.unlock();

    /* compute FFT */
    d_fft->execute();

    /* get FFT data */
    memcpy(fftPoints, d_fft->get_outbuf(), sizeof(gr_complex)*d_fftsize);
    fftSize = d_fftsize;
}

/*! \brief Compute FFT on the available input data.
 *  \param data_in The data to compute FFT on.
 *  \param size The size of data_in.
 *
 * Note that this function does not lock the mutex since the caller, get_fft_data()
 * has already locked it.
 */
void rx_fft_f::apply_window(unsigned int size)
{
    gr_complex *dst = d_fft->get_inbuf();
    float * p = (float *)d_reader->read_pointer();
    /* apply window, and convert to complex */
    if (d_window.size())
    {
        for (unsigned int i = 0; i < size; i++)
            dst[i] = p[i] * d_window[i];
    }
    else
    {
        for (unsigned int i = 0; i < size; i++)
            dst[i] = p[i];
    }
}


/*! \brief Set new FFT size. */
void rx_fft_f::set_fft_size(unsigned int fftsize)
{
    if (fftsize != d_fftsize)
    {
        d_fftsize = fftsize;

        /* reset window */
        int wintype = d_wintype; // FIXME: would be nicer with a window_reset()
        d_wintype = -1;
        set_window_type(wintype);

        /* reset FFT object (also reset FFTW plan) */
        delete d_fft;
#if GNURADIO_VERSION < 0x030900
        d_fft = new gr::fft::fft_complex(d_fftsize, true);
#else
        d_fft = new gr::fft::fft_complex_fwd(d_fftsize);
#endif
    }
}

/*! \brief Get currently used FFT size. */
unsigned int rx_fft_f::get_fft_size() const
{
    return d_fftsize;
}

/*! \brief Set new window type. */
void rx_fft_f::set_window_type(int wintype)
{
    if (wintype == d_wintype)
    {
        /* nothing to do */
        return;
    }

    d_wintype = wintype;

    if ((d_wintype < gr::fft::window::WIN_HAMMING) || (d_wintype > gr::fft::window::WIN_FLATTOP))
    {
        d_wintype = gr::fft::window::WIN_HAMMING;
    }

    d_window.clear();
    d_window = gr::fft::window::build((gr::fft::window::win_type)d_wintype, d_fftsize, 6.76);
}

/*! \brief Get currently used window type. */
int rx_fft_f::get_window_type() const
{
    return d_wintype;
}



/* fft channelizer */
fft_channelizer_cc::sptr fft_channelizer_cc::make(int nchannels, int osr, int wintype, int nthreads)
{
    return gnuradio::get_initial_sptr(new fft_channelizer_cc (nchannels, osr, wintype, nthreads));
}

fft_channelizer_cc::fft_channelizer_cc(int nchannels, int osr, int wintype, int nthreads)
    : gr::sync_decimator ("fft_chan",
          gr::io_signature::make(1, 1, sizeof(gr_complex)),
          gr::io_signature::make(0, RX_MAX, sizeof(gr_complex)),nchannels / 2),
      d_fftsize(nchannels),
      d_osr(osr),
      d_wintype(-1),
      d_remaining(0),
      d_noutputs(0),
      d_filter_param(6.5),
      d_nthreads(nthreads),
      d_active(nthreads)
{
    start_threads();
    set_relative_rate(double(d_osr) / double(d_fftsize));
    set_decimation(d_fftsize / d_osr);
    /* create FFT window */
    set_window_type(wintype);
    set_history(2048);
    d_map.resize(RX_MAX);
    set_output_multiple(8192);
}

fft_channelizer_cc::~fft_channelizer_cc()
{
    stop_threads();
}

void fft_channelizer_cc::start_threads()
{
    /* create FFT object */
    d_threads = new l_thread[d_nthreads];
    d_active = d_nthreads;
    for(int k = 0; k < d_nthreads; k++)
    {
    #if GNURADIO_VERSION < 0x030900
        d_threads[k].d_fft = new gr::fft::fft_complex(d_fftsize * d_osr, true);
    #else
        d_threads[k].d_fft = new gr::fft::fft_complex_fwd(d_fftsize * d_osr);
    #endif
        d_threads[k].finish = false;
        d_threads[k].in = nullptr;
        d_threads[k].out = nullptr;
        d_threads[k].count = 0;
        d_threads[k].offset = 0;
        if (d_nthreads > 1)
            d_threads[k].thr = new std::thread([this, k](){this->thread_func(k);});
    }
    if (d_nthreads > 1)
    {
        std::unique_lock<std::mutex> thr_lock(d_thread_mutex);
        do d_ready.wait(thr_lock); while(d_active != 0);
    }
}

void fft_channelizer_cc::stop_threads()
{
    for (int k = 0; k < d_nthreads; k++)
        d_threads[k].finish = true;
    if (d_nthreads > 1)
        d_trigger.notify_all();
    for (int k = 0; k < d_nthreads; k++)
    {
        if (d_nthreads > 1)
            d_threads[k].thr->join();
        delete d_threads[k].d_fft;
    }
    delete [] d_threads;
}

void fft_channelizer_cc::thread_func(int n)
{
    gr::thread::set_thread_name(gr::thread::get_current_thread_id(), "chanw" + std::to_string(n));
    while (1)
    {
        if (d_threads[n].finish)
            return;
        if (d_threads[n].count)
        {
            for (int k = 0; k < d_threads[n].count; k++, d_threads[n].in += d_fftsize)
            {
                if (d_window.size())
                {
                    gr_complex *dst = d_threads[n].d_fft->get_inbuf();
                    volk_32fc_32f_multiply_32fc(dst, d_threads[n].in, &d_window[0], d_fftsize * d_osr);
                }
                else
                {
                    memcpy(d_threads[n].d_fft->get_inbuf(), d_threads[n].in, sizeof(gr_complex) * d_fftsize * d_osr);
                }
                d_threads[n].d_fft->execute();
                gr_complex * ob = (gr_complex *)d_threads[n].d_fft->get_outbuf();
                for (int j = 0; j < d_noutputs ; j++)
                    ((gr_complex *)d_threads[n].out[j])[k + d_threads[n].offset] = ob[d_map[j]];
            }
        }
        if (d_nthreads > 1)
        {
            std::unique_lock<std::mutex> guard(d_thread_mutex);
            d_active --;
            if (d_active == 0)
                d_ready.notify_one();
            d_trigger.wait(guard);
            d_active ++;
        }
        else
            return;
    }
}

bool fft_channelizer_cc::start()
{
    d_remaining = 0;
    return sync_block::start();
}

bool fft_channelizer_cc::check_topology(int ninputs, int noutputs)
{
    d_noutputs = noutputs;
    bool ret = sync_decimator::check_topology(ninputs, noutputs);
    return ret;
}

int fft_channelizer_cc::work(int noutput_items,
            gr_vector_const_void_star &input_items,
            gr_vector_void_star &output_items)
{
    const gr_complex *in = (const gr_complex*)input_items[0];
    std::lock_guard<std::mutex> lock(d_mutex);
    in += history() - d_fftsize * (d_osr - 1);
    int nblocks = noutput_items;
    int count_one = noutput_items / d_nthreads;
    if (d_nthreads == 1)
    {
        d_threads[0].in = in;
        d_threads[0].out = (gr_complex **) &output_items[0];
        d_threads[0].count = count_one;
        d_threads[0].offset = 0;
        thread_func(0);
    }
    else
    {
        for (int k = 0; k < d_nthreads; k++, in += d_fftsize * count_one)
        {
            d_threads[k].in = in;
            d_threads[k].out = (gr_complex **) &output_items[0];
            d_threads[k].count = count_one;
            d_threads[k].offset = k * count_one;
        }
        d_trigger.notify_all();
        {
            std::unique_lock<std::mutex> thr_lock(d_thread_mutex);
            do d_ready.wait(thr_lock); while(d_active != 0);

        }
    }
    return nblocks;
}

void fft_channelizer_cc::set_window_type(int wintype)
{
    if (wintype == d_wintype)
        /* nothing to do */
        return;

    if ((wintype < gr::fft::window::WIN_HAMMING) || (wintype > gr::fft::window::WIN_FLATTOP))
        wintype = gr::fft::window::WIN_HAMMING;
    set_params(d_fftsize, wintype, d_osr, d_filter_param, d_nthreads);
}

int  fft_channelizer_cc::get_window_type() const
{
    return d_wintype;
}

void fft_channelizer_cc::set_fft_size(int fftsize)
{
    if (fftsize != d_fftsize)
        set_params(fftsize, d_wintype, d_osr, d_filter_param, d_nthreads);
}

void fft_channelizer_cc::set_nthreads(int n)
{
    if (n != d_nthreads)
        set_params(d_fftsize, d_wintype, d_osr, d_filter_param, n);
}

int fft_channelizer_cc::nthreads()
{
    return d_nthreads;
}

int fft_channelizer_cc::get_fft_size() const
{
    return d_fftsize;
}

void fft_channelizer_cc::map_output(int output, int pb)
{
    if(output < 0)
        return;
    if(output >= int(d_map.size()))
        return;
    d_map[output] = (d_fftsize * d_osr + pb) % (d_fftsize * d_osr);
//    std::cerr<<"fft_channelizer_cc::map_output("<<output<<","<<pb<<")=>"<<d_map[output]<<"\n";
}

void fft_channelizer_cc::set_osr(int n)
{
    if (n != d_osr)
        set_params(d_fftsize, d_wintype, n, d_filter_param, d_nthreads);
}

void fft_channelizer_cc::set_decim(int n)
{
    set_fft_size(n);
}

void fft_channelizer_cc::set_filter_param(float n)
{
    if(d_filter_param != n)
        set_params(d_fftsize, d_wintype, d_osr, n, d_nthreads);
}

void fft_channelizer_cc::set_params(int fftsize, int wintype, int osr, float filter_param, int nthreads)
{
    std::lock_guard<std::mutex> lock(d_mutex);
    if((d_wintype == wintype)&&(d_fftsize == fftsize)&&(d_osr == osr)&&(d_filter_param == filter_param)&&(d_nthreads==nthreads))
        return;
    std::cerr<<"fft_channelizer_cc::set_params "<<fftsize<<" "<<wintype<<" "<<osr<<" "<<filter_param<<" "<<nthreads<<std::endl;
    d_wintype = wintype;
    d_filter_param = filter_param;
    d_osr = osr;
    d_fftsize = fftsize;
    d_window.clear();
    d_window = gr::fft::window::build((gr::fft::window::win_type)d_wintype, d_fftsize * d_osr, d_filter_param);
    for(auto &dw :d_window)
        dw /= float(d_fftsize) * 1.7;
    /* reset FFT object (also reset FFTW plan) */
    if(d_nthreads != nthreads)
    {
        stop_threads();
        d_nthreads = nthreads;
        start_threads();
    }
    else
        for(int k = 0; k < d_nthreads; k++)
        {
            delete d_threads[k].d_fft;
    #if GNURADIO_VERSION < 0x030900
            d_threads[k].d_fft = new gr::fft::fft_complex(d_fftsize * d_osr, true);
    #else
            d_threads[k].d_fft = new gr::fft::fft_complex_fwd(d_fftsize * d_osr);
    #endif
        }
    set_relative_rate(1.0 / double(d_fftsize));
    set_decimation(d_fftsize);
    for(int j = 0; j < d_noutputs ; j++)
        d_map[j] %= d_fftsize * d_osr;
}
