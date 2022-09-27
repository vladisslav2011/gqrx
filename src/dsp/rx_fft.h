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
#ifndef RX_FFT_H
#define RX_FFT_H

#include <mutex>
#include <gnuradio/sync_block.h>
#include <gnuradio/sync_decimator.h>
#include <gnuradio/fft/fft.h>
#include <gnuradio/filter/firdes.h>       /* contains enum win_type */
#include <gnuradio/gr_complex.h>
#include <gnuradio/buffer.h>
#if GNURADIO_VERSION >= 0x031000
#include <gnuradio/buffer_reader.h>
#endif
#include <chrono>
#include <thread>
#include <condition_variable>


#define MAX_FFT_SIZE 1048576*4
#define AUDIO_BUFFER_SIZE 65536

class rx_fft_c;
class rx_fft_f;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<rx_fft_c> rx_fft_c_sptr;
typedef boost::shared_ptr<rx_fft_f> rx_fft_f_sptr;
#else
typedef std::shared_ptr<rx_fft_c> rx_fft_c_sptr;
typedef std::shared_ptr<rx_fft_f> rx_fft_f_sptr;
#endif


class fft_c_basic
{
public:
    fft_c_basic(unsigned int fftsize = 16384, int wintype = -1);
    virtual ~fft_c_basic();
    virtual void get_fft_data(std::complex<float>* fftPoints, unsigned int &fftSize, gr_complex * data);
    virtual void set_window_type(int wintype);
    virtual int  get_window_type() const;

    virtual void set_fft_size(unsigned int fftsize);
    virtual unsigned int get_fft_size() const;
    void copy_params(fft_c_basic & from);
protected:
    unsigned int d_fftsize;   /*! Current FFT size. */
    int          d_wintype;   /*! Current window type. */

#if GNURADIO_VERSION < 0x030900
    gr::fft::fft_complex    *d_fft;    /*! FFT object. */
#else
    gr::fft::fft_complex_fwd *d_fft;   /*! FFT object. */
#endif
    std::vector<float>  d_window; /*! FFT window taps. */

    virtual void apply_window(unsigned int size, gr_complex * p);
    virtual void set_params();
};

/*! \brief Return a shared_ptr to a new instance of rx_fft_c.
 *  \param fftsize The FFT size
 *  \param winttype The window type (see gnuradio/filter/firdes.h)
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, the rx_fft_c constructor is private.
 * make_rx_fft_c is the public interface for creating new instances.
 */
rx_fft_c_sptr make_rx_fft_c(unsigned int fftsize=4096, double quad_rate=0, int wintype=gr::fft::window::WIN_HAMMING);


/*! \brief Block for computing complex FFT.
 *  \ingroup DSP
 *
 * This block is used to compute the FFT of the received spectrum.
 *
 * The samples are collected in a circular buffer with size FFT_SIZE.
 * When the GUI asks for a new set of FFT data via get_fft_data() an FFT
 * will be performed on the data stored in the circular buffer - assuming
 * of course that the buffer contains at least fftsize samples.
 *
 * \note Uses code from qtgui_sink_c
 */
class rx_fft_c : public gr::sync_block, public fft_c_basic
{
    friend rx_fft_c_sptr make_rx_fft_c(unsigned int fftsize, double quad_rate, int wintype);

protected:
    rx_fft_c(unsigned int fftsize=4096, double quad_rate=0, int wintype=gr::fft::window::WIN_HAMMING);

public:
    ~rx_fft_c();

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items) override;

    bool start() override;
    void get_fft_data(std::complex<float>* fftPoints, unsigned int &fftSize);
    using fft_c_basic::get_fft_data;

    void set_quad_rate(double quad_rate);
    void set_enabled(bool enabled) { d_enabled=enabled; };

private:
    double       d_quadrate;
    bool         d_enabled;

    std::mutex   d_mutex;  /*! Used to lock FFT output buffer. */

    gr::buffer_sptr d_writer;
    gr::buffer_reader_sptr d_reader;
    std::chrono::time_point<std::chrono::steady_clock> d_lasttime;

};



/*! \brief Return a shared_ptr to a new instance of rx_fft_f.
 *  \param fftsize The FFT size
 *  \param winttype The window type (see gnuradio/filter/firdes.h)
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, the rx_fft_f constructor is private.
 * make_rx_fft_f is the public interface for creating new instances.
 */
rx_fft_f_sptr make_rx_fft_f(unsigned int fftsize=1024, double audio_rate=48000, int wintype=gr::fft::window::WIN_HAMMING);


/*! \brief Block for computing real FFT.
 *  \ingroup DSP
 *
 * This block is used to compute the FFT of the audio spectrum or anything
 * else where real FFT is useful.
 *
 * The samples are collected in a circular buffer with size FFT_SIZE.
 * When the GUI asks for a new set of FFT data using get_fft_data() an FFT
 * will be performed on the data stored in the circular buffer - assuming
 * that the buffer contains at least fftsize samples.
 *
 * \note Uses code from qtgui_sink_f
 */
class rx_fft_f : public gr::sync_block
{
    friend rx_fft_f_sptr make_rx_fft_f(unsigned int fftsize, double audio_rate, int wintype);

protected:
    rx_fft_f(unsigned int fftsize=1024, double audio_rate=48000, int wintype=gr::fft::window::WIN_HAMMING);

public:
    ~rx_fft_f();

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items) override;

    bool start() override;
    void get_fft_data(std::complex<float>* fftPoints, unsigned int &fftSize);

    void set_window_type(int wintype);
    int  get_window_type() const;

    void set_fft_size(unsigned int fftsize);
    unsigned int get_fft_size() const;
    void set_enabled(bool enabled) { d_enabled=enabled; };

private:
    unsigned int d_fftsize;   /*! Current FFT size. */
    double       d_audiorate;
    int          d_wintype;   /*! Current window type. */
    bool         d_enabled;

    std::mutex   d_mutex;  /*! Used to lock FFT output buffer. */

#if GNURADIO_VERSION < 0x030900
    gr::fft::fft_complex    *d_fft;    /*! FFT object. */
#else
    gr::fft::fft_complex_fwd *d_fft;   /*! FFT object. */
#endif
    std::vector<float>  d_window; /*! FFT window taps. */

    gr::buffer_sptr d_writer;
    gr::buffer_reader_sptr d_reader;
    std::chrono::time_point<std::chrono::steady_clock> d_lasttime;

    void apply_window(unsigned int size);

};



/*! \brief Block for computing complex FFT.
 *  \ingroup DSP
 *
 * This block is used to compute the FFT of the received spectrum.
 *
 * The samples are collected in a circular buffer with size FFT_SIZE.
 * When the GUI asks for a new set of FFT data via get_fft_data() an FFT
 * will be performed on the data stored in the circular buffer - assuming
 * of course that the buffer contains at least fftsize samples.
 *
 */
class fft_channelizer_cc : public gr::sync_decimator
{
public:
#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<fft_channelizer_cc> sptr;
#else
typedef std::shared_ptr<fft_channelizer_cc> sptr;
#endif

    static sptr make(int nchannels, int osr, int wintype=gr::fft::window::WIN_HAMMING, int nthreads=1);

protected:
    fft_channelizer_cc(int nchannels, int osr, int wintype=gr::fft::window::WIN_HAMMING, int nthreads=1);

public:
    ~fft_channelizer_cc();

    bool start() override;
    bool check_topology(int ninputs, int noutputs) override;
    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items) override;

    void set_window_type(int wintype);
    int  get_window_type() const;

    void set_fft_size(int fftsize);
    int get_fft_size() const;
    void map_output(int output, int pb);
    int get_map(int output) const { return d_map[output]; }
    int channel_count() const { return d_fftsize * d_osr; }
    int decim() const { return d_fftsize; }
    int osr() const { return d_osr; }
    int filter_param() const { return d_filter_param; }
    int nthreads();
    void set_osr(int n);
    void set_decim(int n);
    void set_filter_param(float n);
    void set_nthreads(int n);

private:
    typedef struct {
    #if GNURADIO_VERSION < 0x030900
        gr::fft::fft_complex    *d_fft;    /*! FFT object. */
    #else
        gr::fft::fft_complex_fwd *d_fft;   /*! FFT object. */
    #endif
        std::thread * thr;
        const gr_complex *in;
        gr_complex **out;
        int count;
        int offset;
        bool finish;
    } l_thread;

    int          d_fftsize;   /*! Current FFT size. */
    int          d_osr;
    int          d_wintype;   /*! Current window type. */
    int          d_remaining;
    int          d_noutputs;
    float        d_filter_param;
    std::vector<int> d_map;

    std::mutex   d_mutex;  /*! Used to lock FFT output buffer. */
    std::mutex   d_thread_mutex;  /*! Thread triggering. */
    std::condition_variable d_ready;
    std::condition_variable d_trigger;
    std::vector<float>  d_window; /*! FFT window taps. */
    l_thread *   d_threads;
    int          d_nthreads;
    int          d_active;

    void set_params(int fftsize, int wintype, int osr, float filter_param, int nthreads);
    void thread_func(int n);
    void stop_threads();
    void start_threads();
};



#endif /* RX_FFT_H */
