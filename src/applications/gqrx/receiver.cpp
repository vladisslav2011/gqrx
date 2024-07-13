/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2014 Alexandru Csete OZ9AEC.
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
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <typeinfo>
#include <QDebug>

#include <gnuradio/prefs.h>
#include <gnuradio/top_block.h>
#include <osmosdr/source.h>
#include <osmosdr/ranges.h>

#include "applications/gqrx/receiver.h"
#include "dsp/correct_iq_cc.h"
#include "dsp/filter/fir_decim.h"
#include "dsp/rx_fft.h"
#include "receivers/demod_factory.h"

#ifdef WITH_PULSEAUDIO
#include "pulseaudio/pa_sink.h"
#elif WITH_PORTAUDIO
#include "portaudio/portaudio_sink.h"
#else
#include <gnuradio/audio/sink.h>
#endif

// change to WATERFALL_TIME_BENCHMARK to enable timing information output
#define NOWATERFALL_TIME_BENCHMARK

/**
 * @brief Public constructor.
 * @param input_device Input device specifier.
 * @param audio_device Audio output device specifier,
 *                     e.g. hw:0 when using ALSA or Portaudio.
 */
receiver::receiver(const std::string input_device,
                   const std::string audio_device,
                   unsigned int decimation)
    : d_current(-1),
      d_active(0),
      d_running(false),
      d_input_rate(96000.0),
      d_use_chan(false),
      d_enable_chan(false),
      d_audio_rate(48000),
      d_decim(decimation),
      d_rf_freq(144800000.0),
      d_recording_iq(false),
      d_sniffer_active(false),
      d_iq_rev(false),
      d_dc_cancel(false),
      d_iq_balance(false),
      d_mute(false),
      d_iq_fmt(FILE_FORMAT_NONE),
      d_last_format(FILE_FORMAT_NONE)
{

    tb = gr::make_top_block("gqrx");

    if (input_device.empty())
    {
        src = osmosdr::source::make("file="+escape_filename(get_zero_file())+",freq=428e6,rate=96000,repeat=true,throttle=true");
    }
    else
    {
        input_devstr = input_device;
        src = osmosdr::source::make(input_device);
    }

    // input decimator
    if (d_decim >= 2)
    {
        try
        {
            input_decim = make_fir_decim_cc(d_decim);
        }
        catch (std::range_error &e)
        {
            std::cout << "Error creating input decimator " << d_decim
                      << ": " << e.what() << std::endl
                      << "Using decimation 1." << std::endl;
            d_decim = 1;
        }

        d_decim_rate = d_input_rate / (double)d_decim;
    }
    else
    {
        d_decim_rate = d_input_rate;
    }

    rx.reserve(RX_MAX);
    rx.clear();
    d_current = 0;
    rx.push_back(make_nbrx(d_decim_rate, d_audio_rate));
    rx[d_current]->set_index(d_current);

    input_file = file_source::make(sizeof(gr_complex),get_zero_file().c_str(),0,0,0,1);
    input_throttle = gr::blocks::throttle::make(sizeof(gr_complex),192000.0);

    iq_swap = make_iq_swap_cc(false);
    iq_src = iq_swap;
    dc_corr = make_dc_corr_cc(d_decim_rate, 1.0);
    iq_fft = make_rx_fft_c(8192u, d_decim_rate, gr::fft::window::WIN_HANN);

    audio_fft = make_rx_fft_f(8192u, d_audio_rate, gr::fft::window::WIN_HANN);

    add0 = gr::blocks::add_ff::make(1);
    add1 = gr::blocks::add_ff::make(1);
    mc0  = gr::blocks::multiply_const_ff::make(1.0, 1);
    mc1  = gr::blocks::multiply_const_ff::make(1.0, 1);
    null_src = gr::blocks::null_source::make(sizeof(float));

    audio_snk = create_audio_sink(audio_device, d_audio_rate, "DMIX output");

    output_devstr = audio_device;

    probe_fft = make_rx_fft_c(8192u, d_decim_rate / 8, gr::fft::window::WIN_HANN);
    chan = fft_channelizer_cc::make(8*4, 4, gr::fft::window::WIN_KAISER, 1);
    chan->set_filter_param(7.5);

    /* wav sink and source is created when rec/play is started */
    audio_null_sink0 = gr::blocks::null_sink::make(sizeof(float));
    audio_null_sink1 = gr::blocks::null_sink::make(sizeof(float));
    sniffer = make_sniffer_f();
    /* sniffer_rr is created at each activation. */

    set_demod(Modulations::MODE_OFF);
    reconnect_all();

    gr::prefs pref;
    qDebug() << "Using audio backend:"
             << pref.get_string("audio", "audio_module", "N/A").c_str();

}

receiver::~receiver()
{
    tb->stop();
}


/** Start the receiver. */
void receiver::start()
{
    if (!d_running)
    {
        tb->start();
        d_running = true;
    }
}

/** Stop the receiver. */
void receiver::stop()
{
    if (d_running)
    {
        tb->stop();
        tb->wait(); // If the graph is needed to run again, wait() must be called after stop
        d_running = false;
    }
}

/**
 * @brief Select new input device.
 * @param device
 */
void receiver::set_input_device(const std::string device)
{
    qDebug() << "Set input device:";
    qDebug() << "  old:" << input_devstr.c_str();
    qDebug() << "  new:" << device.c_str();

    std::string error = "";

    if (device.empty())
        return;

    input_devstr = device;

    // tb->lock() can hang occasionally
    if (d_running)
    {
        tb->stop();
        tb->wait();
    }

    tb->disconnect_all();
    iq_src.reset();
    d_iq_ts.set_file_source(nullptr);
    for (auto& rxc : rx)
        rxc->connected(false);

#if GNURADIO_VERSION < 0x030802
    //Work around GNU Radio bug #3184
    //temporarily connect dummy source to ensure that previous device is closed
    src = osmosdr::source::make("file="+escape_filename(get_zero_file())+",freq=428e6,rate=96000,repeat=true,throttle=true");
    auto tmp_sink = gr::blocks::null_sink::make(sizeof(gr_complex));
    tb->connect(src, 0, tmp_sink, 0);
    tb->start();
    tb->stop();
    tb->wait();
    tb->disconnect_all();
#else
    src.reset();
#endif

    try
    {
        src = osmosdr::source::make(device);
    }
    catch (std::exception &x)
    {
        error = x.what();
        src = osmosdr::source::make("file=" + escape_filename(get_zero_file()) + ",freq=428e6,rate=96000,repeat=true,throttle=true");
    }
    reconnect_all(FILE_FORMAT_NONE, true);
    if (src->get_sample_rate() != 0)
        set_input_rate(src->get_sample_rate());

    if (d_running)
        tb->start();

    if (error != "")
    {
        throw std::runtime_error(error);
    }
}

/**
 * @brief Select a file as an input device.
 * @param name
 * @param sample_rate
 * @param fmt
 */
void receiver::set_input_file(const std::string name, const int sample_rate,
                              const file_formats fmt, uint64_t time_ms)
{
    std::string error = "";

    d_iq_filename = name;
    d_iq_time_ms = time_ms;
    input_file = file_source::make(any_to_any_base::fmt[fmt].size, name.c_str(), 0, 0, sample_rate / any_to_any_base::fmt[fmt].nsamples,
                                   time_ms, d_iq_repeat, d_iq_buffers_max);

    if (d_running)
    {
        tb->stop();
        tb->wait();
    };

    tb->disconnect_all();
    for (auto& rxc : rx)
        rxc->connected(false);

    input_throttle = gr::blocks::throttle::make(sizeof(gr_complex), sample_rate);

    //set_demod(d_demod, fmt, true);
    input_file->set_save_progress_cb(d_save_progress);
    d_iq_ts.set_file_source(input_file);
    reconnect_all(fmt, true);
    set_input_rate(sample_rate);

    if (d_running)
        tb->start();

    if (error != "")
    {
        throw std::runtime_error(error);
    }
    input_devstr = "NULL";
}

/**
 * @brief Setup input part of the graph for a file ar a device
 * @param fmt
 */
gr::basic_block_sptr receiver::setup_source(file_formats fmt)
{
    gr::basic_block_sptr b;

    if (fmt == FILE_FORMAT_LAST)
        fmt = d_last_format;
    else
        d_last_format = fmt;
    if (fmt == FILE_FORMAT_NONE)
    {
        // Setup source
        b = src;

        // Pre-processing
        if (d_decim >= 2)
        {
            tb->connect(b, 0, input_decim, 0);
            b = input_decim;
        }

        if (d_recording_iq)
        {
            // We record IQ with minimal pre-processing
            connect_iq_recorder();
        }
    }
    if (fmt > FILE_FORMAT_NONE)
    {
        if (convert_from[fmt]) // Connect through a converter
        {
            tb->connect(input_file, 0, convert_from[fmt], 0);
            if(d_iq_process)
                return convert_from[fmt];
            tb->connect(convert_from[fmt], 0, input_throttle, 0);
            b = input_throttle;
        }else{ // No conversion
            if(d_iq_process)
                return input_file;
            tb->connect(input_file, 0 ,input_throttle, 0);
            b = input_throttle;
        }
        return b;
    }

    return b;
}

/**
 * @brief Select new audio output device.
 * @param device
 */
void receiver::set_output_device(const std::string device)
{
    qDebug() << "Set output device:";
    qDebug() << "   old:" << output_devstr.c_str();
    qDebug() << "   new:" << device.c_str();

    output_devstr = device;

    tb->lock();

    if (d_active > 0)
    {
        if(d_iq_process)
        {
            tb->disconnect(mc0, 0, audio_null_sink0, 0);
            tb->disconnect(mc1, 0, audio_null_sink1, 0);
        }else try {
            tb->disconnect(mc0, 0, audio_snk, 0);
            tb->disconnect(mc1, 0, audio_snk, 1);
        } catch(std::exception &x) {
        }
    }
    audio_snk.reset();

    try {
        audio_snk = create_audio_sink(device, d_audio_rate, "DMIX output");

        if (d_active > 0)
        {
            if(d_iq_process)
            {
                tb->connect(mc0, 0, audio_null_sink0, 0);
                tb->connect(mc1, 0, audio_null_sink1, 0);
            }else{
                tb->connect(mc0, 0, audio_snk, 0);
                tb->connect(mc1, 0, audio_snk, 1);
            }
        }

        tb->unlock();

    } catch (std::exception &x) {
        tb->unlock();
        // handle problems on non-freeing devices
        throw x;
    }
}

/** Get a list of available antenna connectors. */
std::vector<std::string> receiver::get_antennas(void) const
{
    return src->get_antennas();
}

/** Select antenna connector. */
bool receiver::set_antenna(const c_def::v_union &antenna)
{
    if (!std::string(antenna).empty())
    {
        try{
            src->set_antenna(antenna);
        }catch(std::exception &e)
        {
            c_def::v_union n;
            get_antenna(n);
            changed_value(C_ANTENNA,0,n);
        }
    }
    return true;
}

bool receiver::get_antenna(c_def::v_union &v) const
{
    v=src->get_antenna();
    return true;
}


/**
 * @brief Set new input sample rate.
 * @param rate The desired input rate
 * @return The actual sample rate set or 0 if there was an error with the
 *         device.
 */
double receiver::set_input_rate(double rate)
{
    double  current_rate;
    bool    rate_has_changed;

    current_rate = src->get_sample_rate();
    rate_has_changed = !(rate == current_rate ||
            std::abs(rate - current_rate) < std::abs(std::min(rate, current_rate))
            * std::numeric_limits<double>::epsilon());

    tb->lock();
    try
    {
        d_input_rate = src->set_sample_rate(rate);
    }
    catch (std::runtime_error &e)
    {
        d_input_rate = 0;
    }

    if (d_input_rate == 0)
    {
        // This can be the case when no device is attached and gr-osmosdr
        // puts in a null_source with rate 100 ksps or if the rate has not
        // changed
        if (rate_has_changed)
        {
            std::cerr << std::endl;
            std::cerr << "Failed to set RX input rate to " << rate << std::endl;
            std::cerr << "Your device may not be working properly." << std::endl;
            std::cerr << std::endl;
        }
        d_input_rate = rate;
    }

    if(d_last_format == FILE_FORMAT_NONE)
        d_decim_rate = d_input_rate / (double)d_decim;
    else
        d_decim_rate = d_input_rate;
    dc_corr->set_sample_rate(d_decim_rate);
    configure_channelizer(false);
    iq_fft->set_quad_rate(d_decim_rate);
    probe_fft->set_quad_rate(d_decim_rate / chan->decim());
    tb->unlock();

    return d_input_rate;
}

/** Set input decimation */
unsigned int receiver::set_input_decim(unsigned int decim)
{
    if (decim == d_decim)
        return d_decim;

    if (d_running)
    {
        tb->stop();
        tb->wait();
    }

    input_decim.reset();
    d_decim = decim;
    if (d_decim >= 2)
    {
        try
        {
            input_decim = make_fir_decim_cc(d_decim);
        }
        catch (std::range_error &e)
        {
            std::cout << "Error opening creating input decimator " << d_decim
                      << ": " << e.what() << std::endl
                      << "Using decimation 1." << std::endl;
            d_decim = 1;
        }

        d_decim_rate = d_input_rate / (double)d_decim;
    }
    else
    {
        d_decim_rate = d_input_rate;
    }

    // update quadrature rate
    dc_corr->set_sample_rate(d_decim_rate);
    iq_fft->set_quad_rate(d_decim_rate);
    probe_fft->set_quad_rate(d_decim_rate / chan->decim());
    configure_channelizer(true);

#ifdef CUSTOM_AIRSPY_KERNELS
    if (input_devstr.find("airspy") != std::string::npos)
        src->set_bandwidth(d_decim_rate);
#endif

    if (d_running)
        tb->start();

    return d_decim;
}

/**
 * @brief Set new analog bandwidth.
 * @param bw The new bandwidth.
 * @return The actual bandwidth.
 */
double receiver::set_analog_bandwidth(double bw)
{
    return src->set_bandwidth(bw);
}

/** Get current analog bandwidth. */
double receiver::get_analog_bandwidth(void) const
{
    return src->get_bandwidth();
}

/** Set I/Q reversed. */
bool receiver::set_iq_swap(const c_def::v_union & v)
{
    if (bool(v) == d_iq_rev)
        return true;

    d_iq_rev = v;
    // until we have a way to bypass a hier_block2 without overhead
    // we do a reconf
    reconnect_all(FILE_FORMAT_LAST, true);
    iq_swap->set_enabled(d_iq_rev);
    return true;
}

/**
 * @brief Get current I/Q reversed setting.
 * @retval true I/Q swappign is enabled.
 * @retval false I/Q swapping is disabled.
 */
bool receiver::get_iq_swap(c_def::v_union & v) const
{
    v=d_iq_rev;
    return true;
}

/**
 * @brief Enable/disable automatic DC removal in the I/Q stream.
 * @param enable Whether DC removal should enabled or not.
 */
bool receiver::set_dc_cancel(const c_def::v_union & v)
{
    if (bool(v) == d_dc_cancel)
        return true;

    d_dc_cancel = v;

    // until we have a way to switch on/off
    // inside the dc_corr_cc we do a reconf
    reconnect_all(FILE_FORMAT_LAST, true);
//    set_demod(d_demod, FILE_FORMAT_LAST, true);
    return true;
}

/**
 * @brief Get auto DC cancel status.
 * @retval true  Automatic DC removal is enabled.
 * @retval false Automatic DC removal is disabled.
 */
bool receiver::get_dc_cancel(c_def::v_union & v) const
{
    v=d_dc_cancel;
    return true;
}

/**
 * @brief Enable/disable automatic I/Q balance.
 * @param enable Whether automatic I/Q balance should be enabled.
 */
bool receiver::set_iq_balance(const c_def::v_union & v)
{
    if (bool(v) == d_iq_balance)
        return true;

    d_iq_balance = v;

    try
    {
        src->set_iq_balance_mode(d_iq_balance ? 2 : 0);
    }catch(std::exception & e)
    {
        changed_value(C_IQ_BALANCE,0,0);
        // TODO: emit a message here
/*        if (enabled)
        {
            QMessageBox::warning(this, tr("Gqrx error"),
                                 tr("Failed to set IQ balance.\n"
                                    "IQ balance setting in Input Control disabled."),
                                 QMessageBox::Ok, QMessageBox::Ok);
        }*/
    }
    return true;
}

/**
 * @brief Get auto I/Q balance status.
 * @retval true  Automatic I/Q balance is enabled.
 * @retval false Automatic I/Q balance is disabled.
 */
bool receiver::get_iq_balance(c_def::v_union & v) const
{
    v=d_iq_balance;
    return true;
}

/**
 * @brief Set RF frequency.
 * @param freq_hz The desired frequency in Hz.
 * @return RX_STATUS_ERROR if an error occurs, e.g. the frequency is out of range.
 * @sa get_rf_freq()
 */
receiver::status receiver::set_rf_freq(double freq_hz)
{
    d_rf_freq = freq_hz;

    src->set_center_freq(d_rf_freq);
    for (auto& rxc : rx)
        rxc->set_center_freq(d_rf_freq);//to generate audio filename
    // FIXME: read back frequency?
    changed_value(C_HW_FREQ_LABEL, d_current, d_rf_freq * 1e-6);
    return STATUS_OK;
}

/**
 * @brief Get RF frequency.
 * @return The current RF frequency.
 * @sa set_rf_freq()
 */
double receiver::get_rf_freq(void)
{
    d_rf_freq = src->get_center_freq();

    return d_rf_freq;
}

/**
 * @brief Get the RF frequency range of the current input device.
 * @param start The lower limit of the range in Hz.
 * @param stop  The upper limit of the range in Hz.
 * @param step  The frequency step in Hz.
 * @returns STATUS_OK if the range could be retrieved, STATUS_ERROR if an error has occurred.
 */
receiver::status receiver::get_rf_range(double *start, double *stop, double *step)
{
    osmosdr::freq_range_t range;

    range = src->get_freq_range();

    // currently range is empty for all but E4000
    if (!range.empty())
    {
        if (range.start() < range.stop())
        {
            *start = range.start();
            *stop  = range.stop();
            *step  = range.step();  /** FIXME: got 0 for rtl-sdr? **/

            return STATUS_OK;
        }
    }

    return STATUS_ERROR;
}

bool receiver::get_hw_freq_label(c_def::v_union & v) const
{
    v = d_rf_freq * 1e-6;
    return true;
}

/** Get the names of available gain stages. */
std::vector<std::string> receiver::get_gain_names()
{
    return src->get_gain_names();
}

/**
 * @brief Get gain range for a specific stage.
 * @param[in]  name The name of the gain stage.
 * @param[out] start Lower limit for this gain setting.
 * @param[out] stop  Upper limit for this gain setting.
 * @param[out] step  The resolution for this gain setting.
 *
 * This function returns the range for the requested gain stage.
 */
receiver::status receiver::get_gain_range(std::string &name, double *start,
                                          double *stop, double *step) const
{
    osmosdr::gain_range_t range;

    range = src->get_gain_range(name);
    *start = range.start();
    *stop  = range.stop();
    *step  = range.step();

    return STATUS_OK;
}

receiver::status receiver::set_gain(std::string name, double value)
{
    src->set_gain(value, name);

    return STATUS_OK;
}

double receiver::get_gain(std::string name) const
{
    return src->get_gain(name);
}

/**
 * @brief Set RF gain.
 * @param gain_rel The desired relative gain between 0.0 and 1.0 (use -1 for
 *                 AGC where supported).
 * @return RX_STATUS_ERROR if an error occurs, e.g. the gain is out of valid range.
 */
bool receiver::set_auto_gain(const c_def::v_union & v)
{
    src->set_gain_mode(v);
    //may fail silently
    c_def::v_union tmp=src->get_gain_mode();
    if(!(tmp == v))
        changed_value(C_IQ_AGC,0,tmp);
    changed_value(C_IQ_AGC_ACK,0,tmp);
    return true;
}

bool receiver::get_auto_gain(c_def::v_union & v) const
{
    v=src->get_gain_mode();
    return true;
}

/**
 * @brief Add new demodulator and select it
 * @return current rx index or -1 if an error occurs.
 */
int receiver::add_rx()
{
    if (rx.size() == RX_MAX)
        return -1;
    //tb->lock(); does not allow block input count changing
    if (d_running)
    {
        tb->stop();
        tb->wait();
    }
    if (d_current >= 0)
        background_rx();
    //TODO: implement virtual receiver_base::sptr receiver_base::copy(int new_index)
    rx.push_back(make_nbrx(d_decim_rate / (d_use_chan ? chan->decim() : 1.0), d_audio_rate));
    int old = d_current;
    d_current = rx.size() - 1;
    rx[d_current]->set_index(d_current);
    set_demod_locked(rx[old]->get_demod(), old);
    if (d_running)
        tb->start();
    return d_current;
}

/**
 * @brief Get demodulator count
 * @return demodulator count.
 */
int receiver::get_rx_count()
{
    return rx.size();
}

/**
 * @brief Delete selected demodulator, select nearest remaining demodulator.
 * @return selected rx index or -1 if there are 0 demodulators remaining.
 */
int receiver::delete_rx()
{
    if (d_current == -1)
        return -1;
    if (rx.size() <= 1)
        return 0;
    //tb->lock(); does not allow block input count changing
    if (d_running)
    {
        tb->stop();
        tb->wait();
    }
    background_rx();
    disconnect_rx();
    rx[d_current].reset();
    if (rx.size() > 1)
    {
        if (d_current != int(rx.size()) - 1)
        {
            rx[d_current] = rx.back();
            rx[d_current]->set_index(d_current);
            rx.back().reset();
        }
        else
            d_current--;
    }
    else
    {
        d_current = -1;
    }
    rx.pop_back();
    foreground_rx();
    if (d_running)
        tb->start();
    return d_current;
}

/**
 * @brief Selects a demodulator.
 * @return STATUS_OK or STATUS_ERROR (if requested demodulator does not exist).
 */
receiver::status receiver::select_rx(int no)
{
    if (no == d_current)
        return STATUS_OK;
    if (no < int(rx.size()))
    {
        tb->lock();
        if (d_current >= 0)
            background_rx();
        d_current = no;
        foreground_rx();
        tb->unlock();
        return STATUS_OK;
    }
    return STATUS_ERROR;
}

receiver::status receiver::fake_select_rx(int no)
{
    if (no == d_current)
        return STATUS_OK;
    if (no < int(rx.size()))
    {
        d_current = no;
        return STATUS_OK;
    }
    return STATUS_ERROR;
}

int receiver::get_current()
{
    return d_current;
}

vfo::sptr receiver::get_current_vfo()
{
    return get_vfo(d_current);
}

vfo::sptr receiver::find_vfo(int64_t freq)
{
    vfo::sptr notfound;
    int64_t offset = freq - d_rf_freq;
    //FIXME: speedup with index???
    for (auto& rxc : rx)
        if (rxc->get_offset() == offset)
            return rxc;
    return notfound;
}

vfo::sptr receiver::get_vfo(int n)
{
    return rx[n];
}

std::vector<vfo::sptr> receiver::get_vfos()
{
    std::vector<vfo::sptr> vfos;
    vfos.reserve(rx.size());
    for (auto& rxc : rx)
        vfos.push_back(rxc);
    return vfos;
}

/**
 * @brief Set filter offset.
 * @param offset_hz The desired filter offset in Hz.
 * @return RX_STATUS_ERROR if the tuning offset is out of range.
 *
 * This method sets a new tuning offset for the receiver. The tuning offset is used
 * to tune within the passband, i.e. select a specific channel within the received
 * spectrum.
 *
 * The valid range for the tuning is +/- 0.5 * the bandwidth although this is just a
 * logical limit.
 *
 * @sa get_filter_offset()
 */
receiver::status receiver::set_filter_offset(double offset_hz)
{
    set_filter_offset(d_current, offset_hz);
    return STATUS_OK;
}

receiver::status receiver::set_filter_offset(int rx_index, double offset_hz)
{
    if(d_use_chan)
    {
        int channel = std::roundf(offset_hz * double(chan->decim() * chan->osr()) / double(d_decim_rate));
        chan->map_output(rx[rx_index]->get_port(), channel);
    }
    rx[rx_index]->set_offset(offset_hz);
    update_agc_panning_auto(rx_index);
    return STATUS_OK;
}

bool receiver::set_agc_panning_auto(const c_def::v_union & v)
{
    rx[d_current]->set_value(C_AGC_PANNING_AUTO, v);
    if(v)
        update_agc_panning_auto(d_current);
    return true;
}

void receiver::update_agc_panning_auto(int rx_index)
{
    c_def::v_union v;
    rx[rx_index]->get_value(C_AGC_PANNING_AUTO,v);
    if(v)
    {
        int panning = rx[rx_index]->get_offset() * 200.0 / d_decim_rate;
        set_value(C_AGC_PANNING, panning);
        changed_value(C_AGC_PANNING, rx_index, panning);
    }
/*
    if (rx[rx_index]->get_agc_panning_auto())
        rx[rx_index]->set_agc_panning(offset_hz * 200.0 / d_decim_rate);
*/
}

/**
 * @brief Get filter offset.
 * @return The current filter offset.
 * @sa set_filter_offset()
 */
double receiver::get_filter_offset(void) const
{
    return rx[d_current]->get_offset();
}

bool receiver::set_freq_lock_all(const c_def::v_union & v)
{
    for (auto& rxc : rx)
        rxc->set_freq_lock(v);
    return true;
}

receiver::status receiver::set_filter(int low, int high, filter_shape shape)
{
    if ((low >= high) || (std::abs(high-low) < RX_FILTER_MIN_WIDTH))
        return STATUS_ERROR;

    rx[d_current]->set_filter(low, high, shape);
    return STATUS_OK;
}

receiver::status receiver::get_filter(int &low, int &high, filter_shape &shape)
{
    rx[d_current]->get_filter(low, high, shape);
    return STATUS_OK;
}

bool receiver::set_freq_corr(const c_def::v_union &ppm)
{
    src->set_freq_corr(ppm);
    return true;
}

bool receiver::get_freq_corr(c_def::v_union &ppm) const
{
    ppm=src->get_freq_corr();
    return true;
}

/**
 * @brief Get current signal power.
 * @param dbfs Whether to use dbfs or absolute power.
 * @return The current signal power.
 *
 * This method returns the current signal power detected by the receiver. The detector
 * is located after the band pass filter. The full scale is 1.0
 */
float receiver::get_signal_pwr() const
{
    return rx[d_current]->get_signal_level();
}

/** Set new FFT size. */
void receiver::set_iq_fft_size(int newsize)
{
    iq_fft->set_fft_size(newsize);
}

void receiver::set_iq_fft_window(int window_type, int correction)
{
    iq_fft->set_window_type(window_type, correction);
}

/** Get latest baseband FFT data. */
void receiver::get_iq_fft_data(std::complex<float>* fftPoints, unsigned int &fftsize)
{
    iq_fft->get_fft_data(fftPoints, fftsize);
}

void receiver::set_iq_fft_enabled(bool enabled)
{
    iq_fft->set_enabled(enabled);
}

/** Get latest audio FFT data. */
void receiver::get_audio_fft_data(std::complex<float>* fftPoints, unsigned int &fftsize)
{
    audio_fft->get_fft_data(fftPoints, fftsize);
}

void receiver::set_audio_fft_enabled(bool enabled)
{
    audio_fft->set_enabled(enabled);
}

void receiver::get_probe_fft_data(std::complex<float>* fftPoints,
                                unsigned int &fftsize)
{
    probe_fft->get_fft_data(fftPoints, fftsize);
}

void receiver::set_probe_channel(int c)
{
    chan->map_output(0, c);
}

int  receiver::get_probe_channel()
{
    return chan->get_map(0);
}

int  receiver::get_probe_channel_count()
{
    return chan->get_fft_size();
}

void receiver::set_chan_decim(int n)
{
    chan->set_decim(n);
    probe_fft->set_quad_rate(d_decim_rate / chan->decim());
}

void receiver::set_chan_osr(int n)
{
    chan->set_osr(n);
}

void receiver::set_chan_filter_param(float n)
{
    chan->set_filter_param(n);
}

bool receiver::get_channelizer(c_def::v_union & v) const
{
    if(!d_enable_chan)
        v=0;
    else
        v=chan->nthreads();
    return true;
}

bool receiver::set_channelizer(const c_def::v_union & v)
{
    const int n=v;
    if (d_enable_chan && n)
    {
        if (chan->nthreads() != n)
            chan->set_nthreads(n);
        return true;
    }
    if (!d_enable_chan && !n)
        return true;

    if (chan->nthreads() != n)
        chan->set_nthreads(n);

    d_enable_chan = (n != 0);
    bool use_chan = d_enable_chan;

    if (d_decim_rate < TARGET_CHAN_RATE * 2)
        use_chan = false;
    if (use_chan == d_use_chan)
        return true;
    if (d_running)
    {
        tb->stop();
        tb->wait();
    }
    std::cerr<<"set_channelizer: stopped\n";
    set_channelizer_int(use_chan);
    if (d_running)
        tb->start();
    std::cerr<<"set_channelizer: started\n";
    return true;
}

void receiver::set_channelizer_int(bool use_chan)
{
    tb->disconnect_all();
    std::cerr<<"set_channelizer: disconnect_all\n";
    for (auto& rxc : rx)
    {
        rxc->connected(false);
        rxc->set_port(-1);
    }
    std::cerr<<"set_channelizer: reset_port\n";
    d_use_chan = use_chan;
    connect_all(FILE_FORMAT_LAST);
    std::cerr<<"set_channelizer: connect_all\n";
    for (auto& rxc : rx)
        set_filter_offset(rxc->get_index(), rxc->get_offset());
    std::cerr<<"set_channelizer: set_filter_offset\n";
    for (auto& rxc : rx)
        rxc->set_quad_rate(d_decim_rate / (use_chan ? chan->decim() : 1.0));
    std::cerr<<"set_channelizer: set_quad_rate\n";
}

void receiver::configure_channelizer(bool reconnect)
{
    int chan_decim = 2;
    while(chan_decim < d_decim_rate / TARGET_CHAN_RATE)
        chan_decim *= 2;
    bool use_chan = d_use_chan;
    if (d_decim_rate < TARGET_CHAN_RATE * 2)
        use_chan = false;
    else
    {
        use_chan = d_enable_chan;
        chan->set_decim(chan_decim);
    }
    if (use_chan == d_use_chan)
    {
        if (d_use_chan)
            for (auto& rxc : rx)
                rxc->set_quad_rate(d_decim_rate / chan->decim());
        else
            for (auto& rxc : rx)
                rxc->set_quad_rate(d_decim_rate);
        if (reconnect)
        {
            tb->disconnect_all();
            for (auto& rxc : rx)
            {
                rxc->connected(false);
                rxc->set_port(-1);
            }
            connect_all(FILE_FORMAT_LAST);
        }
    }
    else
        set_channelizer_int(use_chan);
}

bool receiver::set_sql_auto(const c_def::v_union & level_offset)
{
    const c_def::v_union new_level(rx[d_current]->get_signal_level() + float(level_offset));
    rx[d_current]->set_value(C_SQUELCH_LEVEL, new_level);
    changed_value(C_SQUELCH_LEVEL, d_current, new_level);
    return true;
}

bool receiver::set_sql_auto_global(const c_def::v_union & level_offset)
{
    for (auto& rxc: rx)
        rxc->set_sql_level(rxc->get_signal_level() + float(level_offset));
    c_def::v_union new_level;
    rx[d_current]->get_value(C_SQUELCH_LEVEL, new_level);
    changed_value(C_SQUELCH_LEVEL, d_current, new_level);
    return true;
}

bool receiver::reset_sql_global(const c_def::v_union &)
{
    c_def::v_union new_level = c_def::all()[C_SQUELCH_LEVEL].min();
    for (auto& rxc: rx)
        rxc->set_sql_level(new_level);
    changed_value(C_SQUELCH_LEVEL, d_current, new_level);
    return true;
}

/** Set AGC panning auto mode. */
/*
receiver::status receiver::set_agc_panning_auto(bool mode)
{
    rx[d_current]->set_agc_panning_auto(mode);
    if (mode)
        rx[d_current]->set_agc_panning(rx[d_current]->get_offset() * 200.0 / d_decim_rate);

    return STATUS_OK; // FIXME
}

bool receiver::get_agc_panning_auto()
{
    return rx[d_current]->get_agc_panning_auto();
}
*/
/** Get AGC current gain. */
float receiver::get_agc_gain()
{
    return rx[d_current]->get_agc_gain();
}

/** Set audio mute. */
bool receiver::set_global_mute(const c_def::v_union & v)
{
    if (d_mute == bool(v))
        return true;
    d_mute = v;
    if (d_mute)
    {
        mc0->set_k(0);
        mc1->set_k(0);
    }
    else
        updateAudioVolume();
    return true;
}

void receiver::updateAudioVolume()
{
    float mul_k = get_rx_count() ? 1.f / float(get_rx_count()) : 1.f;
    mc0->set_k(mul_k);
    mc1->set_k(mul_k);
}

/** Get audio mute. */
bool receiver::get_global_mute(c_def::v_union & v) const
{
    v=d_mute;
    return true;
}

receiver::status receiver::set_demod_locked(Modulations::idx demod, int old_idx)
{
    status ret = STATUS_OK;
    Modulations::rx_chain rxc = Modulations::get_rxc(demod);
    Modulations::idx old_demod = get_demod();
    if (rxc != Modulations::rx_chain(STATUS_ERROR))
    {
        receiver_base_cf_sptr old_rx = rx[(old_idx == -1) ? d_current : old_idx];
        Modulations::rx_chain old_rxc = Modulations::get_rxc(old_demod);
        // RX demod chain
        if (old_idx == -1)
        {
            if (old_rxc != rxc)
                if (old_rxc == Modulations::RX_CHAIN_WFMRX)
                    set_value(C_RDS_ON, false);
            background_rx();
            disconnect_rx();
        }
        if (old_rxc != rxc)
        {
            double rx_rate = d_use_chan ? d_decim_rate / chan->decim() : d_decim_rate;
            rx[d_current].reset();
            rx[d_current] = Demod_Factory::make(demod, rx_rate, d_audio_rate);
            rx[d_current]->set_index(d_current);
        }
        //Temporary workaround for https://github.com/gnuradio/gnuradio/issues/5436
        tb->connect(iq_src, 0, rx[d_current], 0);
        // End temporary workaronud
        if (old_rx.get() != rx[d_current].get())
        {
            rx[d_current]->restore_settings(*old_rx.get());
            rx[d_current]->set_offset(old_rx->get_offset());
            // Recorders
            if (old_idx == -1)
            {
                c_def::v_union tmp;
                old_rx->get_audio_rec(tmp);
                if(bool(tmp))
                    rx[d_current]->continue_audio_recording(old_rx);
            }
            if (demod == Modulations::MODE_OFF)
            {
                c_def::v_union tmp;
                rx[d_current]->get_audio_rec(tmp);
                if(bool(tmp))
                {
                    rx[d_current]->set_audio_rec(false);
                }
            }
        }
        set_demod_and_update_filter(old_rx, rx[d_current], demod);
        //Temporary workaround for https://github.com/gnuradio/gnuradio/issues/5436
        tb->disconnect(iq_src, 0, rx[d_current], 0);
        // End temporary workaronud
        connect_rx();
        foreground_rx();
        changed_value(C_MODE_CHANGED, d_current, int(demod));
    }else
        return STATUS_ERROR;
    return ret;
}

void receiver::set_demod_and_update_filter(receiver_base_cf_sptr old_rx, receiver_base_cf_sptr new_rx, Modulations::idx demod)
{
    Modulations::idx old_demod = old_rx->get_demod();
    int     flo=0, fhi=0, new_flo=0, new_fhi=0;
    Modulations::filter_shape filter_shape=Modulations::FILTER_SHAPE_COUNT, new_filter_shape=Modulations::FILTER_SHAPE_COUNT;
    old_rx->get_filter(flo, fhi, filter_shape);
    new_rx->get_filter(new_flo, new_fhi, new_filter_shape);
    int     filter_preset=Modulations::FindFilterPreset(old_demod,flo,fhi);

    if (filter_preset == FILTER_PRESET_USER)
    {
        if (((old_demod == Modulations::MODE_USB) &&
            (demod == Modulations::MODE_LSB))
            ||
        ((old_demod == Modulations::MODE_LSB) &&
            (demod == Modulations::MODE_USB)))
        {
            std::swap(flo, fhi);
            flo = -flo;
            fhi = -fhi;
        }
        Modulations::UpdateFilterRange(demod, flo, fhi);
    }else
        Modulations::GetFilterPreset(demod, filter_preset, flo, fhi);
    new_rx->set_demod(demod);
    if( (flo!=new_flo)||(fhi!=new_fhi)||(filter_shape!=new_filter_shape))
        new_rx->set_filter(flo, fhi, filter_shape);
}

receiver::status receiver::set_demod(Modulations::idx demod, int old_idx)
{
    status ret = STATUS_OK;
    if(old_idx == -1)
    {
        Modulations::idx old_demod = rx[d_current]->get_demod();
        if (old_demod == demod)
            return ret;
        if(Modulations::get_rxc(old_demod) == Modulations::get_rxc(demod))
        {
            rx[d_current]->lock();
            set_demod_and_update_filter(rx[d_current], rx[d_current], demod);
            rx[d_current]->unlock();
            changed_value(C_MODE_CHANGED, d_current, int(demod));
            return ret;
        }
    }
    if (d_running)
    {
        tb->stop();
        tb->wait();
    }
    ret = set_demod_locked(demod, old_idx);
    if (d_running)
        tb->start();

    return ret;
}

receiver::status receiver::reconnect_all(file_formats fmt, bool force)
{
    status ret = STATUS_OK;
    // tb->lock() seems to hang occasionally
    if (d_running)
    {
        tb->stop();
        tb->wait();
    }
    if (force)
    {
        tb->disconnect_all();
        for (auto& rxc : rx)
            rxc->connected(false);
    }
    connect_all(fmt);

    if (d_running)
        tb->start();

    return ret;
}

bool receiver::set_mode(const c_def::v_union & v)
{
    set_demod(Modulations::idx(int(v)));
    return true;
}

bool receiver::get_mode(c_def::v_union & v) const
{
    v=rx[d_current]->get_demod();
    return true;
}

receiver::status receiver::set_audio_rate(int rate)
{
    if(d_audio_rate != rate)
    {
        d_audio_rate = rate;
        audio_fft->set_quad_rate(rate);
    }
    return STATUS_OK;
}

receiver::status receiver::commit_audio_rate()
{
    for(auto & rxc : rx)
        rxc->set_audio_rate(d_audio_rate);
    return STATUS_OK;
}

int receiver::get_audio_rate()
{
    return d_audio_rate;
}

/** Start/stop audio playback. */
bool receiver::set_audio_play(const c_def::v_union & v)
{
    bool cmd = v;
    if(cmd)
    {
        if (!d_running)
        {
            /* receiver is not running */
            std::cout << "Can not start audio playback (receiver not running)" << std::endl;
            changed_value(C_AUDIO_PLAY, 0, false);
            return true;
        }
        c_def::v_union filename;
        get_value(C_AUDIO_REC_FILENAME, filename);
        try {
            // output ports set automatically from file
            wav_src = gr::blocks::wavfile_source::make(std::string(filename).c_str(), false);
        }
        catch (std::runtime_error &e) {
            std::cout << "Error loading " << std::string(filename) << ": " << e.what() << std::endl;
            changed_value(C_AUDIO_PLAY, 0, false);
            return true;
        }

        /** FIXME: We can only handle native rate (should maybe use the audio_rr)? */
        unsigned int audio_rate = (unsigned int) d_audio_rate;
        if (wav_src->sample_rate() != audio_rate)
        {
            std::cout << "BUG: Can not handle sample rate " << wav_src->sample_rate() << std::endl;
            wav_src.reset();

            changed_value(C_AUDIO_PLAY, 0, false);
            return true;
        }

        /** FIXME: We can only handle stereo files */
        if (wav_src->channels() != 2)
        {
            std::cout << "BUG: Can not handle other than 2 channels. File has " << wav_src->channels() << std::endl;
            wav_src.reset();
            changed_value(C_AUDIO_PLAY, 0, false);
            return true;
        }

        stop();
        /* route demodulator output to null sink */
        if (d_active > 0)
        {
            tb->disconnect(mc0, 0, audio_snk, 0);
            tb->disconnect(mc1, 0, audio_snk, 1);
            tb->connect(mc0, 0, audio_null_sink0, 0); /** FIXME: other channel? */
            tb->connect(mc1, 0, audio_null_sink1, 0); /** FIXME: other channel? */
        }
        tb->disconnect(rx[d_current], 0, audio_fft, 0);
        tb->connect(wav_src, 0, audio_snk, 0);
        tb->connect(wav_src, 1, audio_snk, 1);
        tb->connect(wav_src, 0, audio_fft, 0);
        start();
        std::cout << "Playing audio from " << std::string(filename) << std::endl;
    }else{
        /* disconnect wav source and reconnect receiver */
        stop();
        tb->disconnect(wav_src, 0, audio_snk, 0);
        tb->disconnect(wav_src, 1, audio_snk, 1);
        tb->disconnect(wav_src, 0, audio_fft, 0);
        if (d_active > 0)
        {
            tb->disconnect(mc0, 0, audio_null_sink0, 0);
            tb->disconnect(mc1, 0, audio_null_sink1, 0);
            tb->connect(mc0, 0, audio_snk, 0);
            tb->connect(mc1, 0, audio_snk, 1);
        }
        tb->connect(rx[d_current], 0, audio_fft, 0);  /** FIXME: other channel? */
        start();

        /* delete wav_src since we can not change file name */
        wav_src.reset();
    }
    return true;
}

bool receiver::get_audio_play(c_def::v_union & v) const
{
    v=!!wav_src;
    return true;
}


/**
 * @brief Connect I/Q data recorder blocks.
 */
receiver::status receiver::connect_iq_recorder()
{
    gr::basic_block_sptr b;

    if (d_decim >= 2)
        b = input_decim;
    else
        b = src;

    if (convert_to[d_iq_fmt])
    {
            tb->lock();
            tb->connect(b, 0, convert_to[d_iq_fmt], 0);
            tb->connect(convert_to[d_iq_fmt], 0, iq_sink, 0);
            d_recording_iq = true;
            tb->unlock();
    }else{
        tb->lock();
        tb->connect(b, 0, iq_sink, 0);
        d_recording_iq = true;
        tb->unlock();
    }
    return STATUS_OK;
}

/* IQ tool Setters */
bool receiver::set_buffers_max(const c_def::v_union &v)
{
    d_iq_buffers_max = v;
    return true;
}

bool receiver::set_iq_process(const c_def::v_union &v)
{
    if(d_iq_process == bool(v))
        return true;
    d_iq_process = v;
    reconnect_all(FILE_FORMAT_LAST, true);
    return true;
}

bool receiver::set_iq_repeat(const c_def::v_union &v)
{
    d_iq_repeat = v;
    return true;
}

/**
 * @brief Start I/Q data recorder.
 * @param filename The filename where to record.
+ * @param bytes_per_sample A hint to choose correct sample format.
 */
receiver::status receiver::start_iq_recording(const std::string filename, const file_formats fmt)
{
    int sink_bytes_per_chunk = any_to_any_base::fmt[fmt].size;

    if (d_recording_iq) {
        std::cout << __func__ << ": already recording" << std::endl;
        return STATUS_ERROR;
    }

    try
    {
        iq_sink = file_sink::make(sink_bytes_per_chunk, filename.c_str(),
            d_input_rate / any_to_any_base::fmt[fmt].nsamples, true, d_iq_buffers_max);
    }
    catch (std::runtime_error &e)
    {
        std::cout << __func__ << ": couldn't open I/Q file" << std::endl;
        return STATUS_ERROR;
    }

    d_iq_fmt = fmt;
    return connect_iq_recorder();
 }

/** Stop I/Q data recorder. */
receiver::status receiver::stop_iq_recording()
{
    if (!d_recording_iq){
        /* error: we are not recording */
        return STATUS_ERROR;
    }

    tb->lock();
    iq_sink->close();
    if (d_iq_fmt >= FILE_FORMAT_CF)
        tb->disconnect(iq_sink);
    if (convert_to[d_iq_fmt])
        tb->disconnect(convert_to[d_iq_fmt]);
    tb->unlock();
    iq_sink.reset();
    d_recording_iq = false;

    return STATUS_OK;
}

/**
 * @brief Seek to position in IQ file source.
 * @param pos Byte offset from the beginning of the file.
 */
receiver::status receiver::seek_iq_file(long pos)
{
    receiver::status status = STATUS_OK;

    if (input_file->seek(pos, SEEK_SET))
    {
        status = STATUS_OK;
    }
    else
    {
        status = STATUS_ERROR;
    }
    if (input_file->get_items_remaining() == 0 && d_running)
    {
        tb->stop();
        tb->wait();
        tb->start();
    }
    return status;
}

/**
 * @brief Seek to position in IQ file source.
 * @param ts Absolute time in ms since epoch.
 */
receiver::status receiver::seek_iq_file_ts(uint64_t ts, uint64_t &res_point)
{
    receiver::status status = STATUS_OK;

    if (input_file->seek_ts(ts, res_point))
    {
        status = STATUS_OK;
    }
    else
    {
        status = STATUS_ERROR;
    }
    if (input_file->get_items_remaining() == 0 && d_running)
    {
        tb->stop();
        tb->wait();
        tb->start();
    }

    return status;
}

receiver::status receiver::save_file_range_ts(const uint64_t from_ms, const uint64_t len_ms, const std::string name)
{
    if (input_file->save_ts(from_ms, len_ms, name))
        return STATUS_OK;
    return STATUS_ERROR;
}

void receiver::get_iq_tool_stats(struct iq_tool_stats &stats)
{
    stats.recording = d_recording_iq;
    stats.playing = (d_last_format != FILE_FORMAT_NONE);
    if (d_recording_iq && iq_sink)
    {
        stats.failed = iq_sink->get_failed();
        stats.buffer_usage = iq_sink->get_buffer_usage();
        stats.file_pos = iq_sink->get_written();
        stats.sample_pos = stats.file_pos * any_to_any_base::fmt[d_last_format].nsamples;
    }
    if(stats.playing)
    {
        stats.failed = input_file->get_failed();
        stats.buffer_usage = input_file->get_buffer_usage();
        stats.file_pos = input_file->tell();
        stats.sample_pos = stats.file_pos * any_to_any_base::fmt[d_last_format].nsamples;
    }
}

/**
 * @brief Start data sniffer.
 * @param buffsize The buffer that should be used in the sniffer.
 * @return STATUS_OK if the sniffer was started, STATUS_ERROR if the sniffer is already in use.
 */
receiver::status receiver::start_sniffer(unsigned int samprate, int buffsize)
{
    if (d_sniffer_active) {
        /* sniffer already in use */
        return STATUS_ERROR;
    }

    sniffer->set_buffer_size(buffsize);
    sniffer_rr = make_resampler_ff((float)samprate/(float)d_audio_rate);
    tb->lock();
    tb->connect(rx[d_current], 0, sniffer_rr, 0);
    tb->connect(sniffer_rr, 0, sniffer, 0);
    tb->unlock();
    d_sniffer_active = true;

    return STATUS_OK;
}

/**
 * @brief Stop data sniffer.
 * @return STATUS_ERROR i the sniffer is not currently active.
 */
receiver::status receiver::stop_sniffer()
{
    if (!d_sniffer_active) {
        return STATUS_ERROR;
    }

    tb->lock();
    tb->disconnect(rx[d_current], 0, sniffer_rr, 0);

    // Temporary workaround for https://github.com/gnuradio/gnuradio/issues/5436
    tb->disconnect(rx[d_current], 0, audio_fft, 0);
    tb->connect(rx[d_current], 0, audio_fft, 0);
    // End temporary workaronud

    tb->disconnect(sniffer_rr, 0, sniffer, 0);
    tb->unlock();
    d_sniffer_active = false;

    /* delete resampler */
    sniffer_rr.reset();

    return STATUS_OK;
}

/** Get sniffer data. */
void receiver::get_sniffer_data(float * outbuff, unsigned int &num)
{
    sniffer->get_samples(outbuff, num);
}

/** Convenience function to connect all blocks. */
void receiver::connect_all(file_formats fmt)
{
    gr::basic_block_sptr b;

    // Setup source
    b = setup_source(fmt);

    if(d_iq_rev)
    {
        tb->connect(b, 0, iq_swap, 0);
        b = iq_swap;
    }

    if (d_dc_cancel)
    {
        tb->connect(b, 0, dc_corr, 0);
        b = dc_corr;
    }

    // Visualization
    tb->connect(b, 0, iq_fft, 0);
    if(d_use_chan)
    {
        tb->connect(b, 0, chan, 0);
        tb->connect(chan, 0, probe_fft, 0);
    }
    iq_src = b;

    // Audio path (if there is a receiver)
    d_active = 0;
    std::cerr<<"connect_all "<<get_rx_count()<<" "<<fmt<<std::endl;
    for (auto& rxc : rx)
        connect_rx(rxc->get_index());
    foreground_rx();
}

void receiver::connect_rx()
{
    connect_rx(d_current);
}

void receiver::connect_rx(int n)
{
    if (!rx[n])
        return;
    if (rx[n]->connected())
        return;
    std::cerr<<"connect_rx "<<n<<" active "<<d_active<<" demod "<<rx[n]->get_demod()<<std::endl;
    rx[n]->set_timestamp_source(&d_iq_ts);
    if (rx[n]->get_demod() != Modulations::MODE_OFF)
    {
        if (d_active == 0)
        {
           std::cerr<<"connect_rx d_active == 0"<<std::endl;
            std::cerr<<"connect_rx connect add"<<std::endl;
            tb->connect(add0, 0, mc0, 0);
            tb->connect(add1, 0, mc1, 0);
            std::cerr<<"connect audio_snk "<<d_active<<std::endl;
            if(d_iq_process)
            {
                tb->connect(mc0, 0, audio_null_sink0, 0);
                tb->connect(mc1, 0, audio_null_sink1, 0);
            }else{
                tb->connect(mc0, 0, audio_snk, 0);
                tb->connect(mc1, 0, audio_snk, 1);
            }
        }
        std::cerr<<"connect_rx d_active > 0 rx="<<n<<" port="<<d_active<<std::endl;
        if(d_use_chan)
            tb->connect(chan, d_active, rx[n], 0);
        else
            tb->connect(iq_src, 0, rx[n], 0);
        tb->connect(rx[n], 0, add0, d_active);
        tb->connect(rx[n], 1, add1, d_active);
        rx[n]->connected(true);
        rx[n]->set_port(d_active);
        if(d_use_chan)
            set_filter_offset(n, rx[n]->get_offset());
        d_active++;
    }
    else
    {
        if (d_active > 0)
        {
            std::cerr<<"connect_rx MODE_OFF d_active > 0"<<std::endl;
            rx[n]->connected(true);
        }
    }
}

void receiver::disconnect_rx()
{
    disconnect_rx(d_current);
}

void receiver::disconnect_rx(int n)
{
    std::cerr<<"disconnect_rx "<<n<<" active "<<d_active<<" demod "<<rx[n]->get_demod()<<std::endl;
    if (rx[n]->get_demod() != Modulations::MODE_OFF)
    {
        d_active--;
        if (d_active == 0)
        {
           std::cerr<<"disconnect_rx d_active == 0"<<std::endl;
            std::cerr<<"disconnect_rx disconnect add"<<std::endl;
            tb->disconnect(add0, 0, mc0, 0);
            tb->disconnect(add1, 0, mc1, 0);
            std::cerr<<"disconnect audio_snk "<<d_active<<std::endl;
            if(d_iq_process)
            {
                tb->disconnect(mc0, 0, audio_null_sink0, 0);
                tb->disconnect(mc1, 0, audio_null_sink1, 0);
            }else{
                tb->disconnect(mc0, 0, audio_snk, 0);
                tb->disconnect(mc1, 0, audio_snk, 1);
            }
        }
        int rx_port = rx[n]->get_port();
        std::cerr<<"disconnect_rx d_active > 0 get_port="<<rx_port<<std::endl;
        if(d_use_chan)
            tb->disconnect(chan, rx_port, rx[n], 0);
        else
            tb->disconnect(iq_src, 0, rx[n], 0);
        std::cerr<<"disconnect_rx in disconnected"<<std::endl;
        tb->disconnect(rx[n], 0, add0, rx_port);
        tb->disconnect(rx[n], 1, add1, rx_port);
        std::cerr<<"disconnect_rx out disconnected"<<std::endl;
        if(rx_port != d_active)
            for(auto& rxc: rx)//FIXME: replace with index lookup
                if(rxc)
                    if(rxc->connected() &&(rxc->get_port() == d_active))
                    {
                        std::cerr<<"disconnect_rx replacing rx="<<rxc->get_index()<<" "<<"get_port="<<rxc->get_port()<<"=>"<<rx_port<<std::endl;
                        if(d_use_chan)
                        {
                            tb->disconnect(chan, d_active, rxc, 0);
                            tb->connect(chan, rx_port, rxc, 0);
                        }
                        tb->disconnect(rxc, 0, add0, d_active);
                        tb->disconnect(rxc, 1, add1, d_active);
                        tb->connect(rxc, 0, add0, rx_port);
                        tb->connect(rxc, 1, add1, rx_port);
                        rxc->set_port(rx_port);
                        if(d_use_chan)
                            set_filter_offset(rxc->get_index(), rxc->get_offset());
                        break;
                    }
    }
    else
    {
        if (d_active > 0)
        {
            std::cerr<<"disconnect_rx MODE_OFF d_active > 0"<<std::endl;
        }
    }
    rx[n]->set_port(-1);
    rx[n]->connected(false);
}

void receiver::background_rx()
{
    std::cerr<<"background_rx "<<d_current<<" "<<rx[d_current]->get_demod()<<std::endl;
    if (rx[d_current]->get_demod() != Modulations::MODE_OFF)
    {
        tb->disconnect(rx[d_current], 0, audio_fft, 0);
        if (d_sniffer_active)
        {
            tb->disconnect(rx[d_current], 0, sniffer_rr, 0);
            tb->disconnect(sniffer_rr, 0, sniffer, 0);
        }
    }
}

void receiver::foreground_rx()
{
    std::cerr<<"foreground_rx "<<d_current<<" "<<rx[d_current]->get_demod()<<std::endl;
    if (rx[d_current]->get_demod() != Modulations::MODE_OFF)
    {
        tb->connect(rx[d_current], 0, audio_fft, 0);
        if (d_sniffer_active)
        {
            tb->connect(rx[d_current], 0, sniffer_rr, 0);
            tb->connect(sniffer_rr, 0, sniffer, 0);
        }
    }
    else
    {

        if (d_sniffer_active)
        {
            d_sniffer_active = false;

            /* delete resampler */
            sniffer_rr.reset();
        }
    }
    if(!d_mute)
        updateAudioVolume();
}

uint64_t receiver::get_filesource_timestamp_ms()
{
    return input_file->get_timestamp_ms();
}

receiver::fft_reader_sptr receiver::get_fft_reader(uint64_t offset, receiver::fft_reader::fft_data_ready cb, int nthreads)
{
    if( d_fft_reader)
    {
        d_fft_reader->reconfigure(d_iq_filename, any_to_any_base::fmt[d_last_format].size,
            any_to_any_base::fmt[d_last_format].nsamples, d_input_rate, d_iq_time_ms, offset,
            convert_from[d_last_format], iq_fft, cb, nthreads);
        return d_fft_reader;
    }else
        return d_fft_reader = std::make_shared<receiver::fft_reader>(d_iq_filename, any_to_any_base::fmt[d_last_format].size,
            any_to_any_base::fmt[d_last_format].nsamples, d_input_rate, d_iq_time_ms, offset, convert_from[d_last_format],
            iq_fft, cb, nthreads);
}

std::string receiver::escape_filename(std::string filename)
{
    std::stringstream ss1;
    std::stringstream ss2;

    ss1 << std::quoted(filename, '\'', '\\');
    ss2 << std::quoted(ss1.str(), '\'', '\\');
    return ss2.str();
}

// receiver::fft_reader

#ifdef _MSC_VER
#define GR_FSEEK _fseeki64
#define GR_FTELL _ftelli64
#define GR_FSTAT _fstat
#define GR_FILENO _fileno
#define GR_STAT _stat
#define S_ISREG(m) (((m)&S_IFMT) == S_IFREG)
#else
#define GR_FSEEK fseeko
#define GR_FTELL ftello
#define GR_FSTAT fstat
#define GR_FILENO fileno
#define GR_STAT stat
#endif

receiver::fft_reader::fft_reader(std::string filename, int chunk_size, int samples_per_chunk, int sample_rate, uint64_t base_ts, uint64_t offset, any_to_any_base::sptr conv, rx_fft_c_sptr fft, receiver::fft_reader::fft_data_ready handler, int nthreads)
{
    d_filename = filename;
    d_chunk_size = chunk_size;
    d_samples_per_chunk = samples_per_chunk;
    d_sample_rate = sample_rate;
    d_base_ts = base_ts;
    d_offset = offset * d_samples_per_chunk;
    d_offset_ms = offset * d_samples_per_chunk * 1000llu / d_sample_rate;
    d_conv = conv;
    if(nthreads == 0)
        nthreads=std::max(1u,std::thread::hardware_concurrency() / 2);
    {
        std::unique_lock<std::mutex> lock(mutex);
        start_threads(nthreads, fft);
        lock.unlock();
    }
    data_ready = handler;
    d_fd = fopen(filename.c_str(), "rb");
    if(d_fd)
    {
        GR_FSEEK(d_fd, 0, SEEK_END);
        d_file_size = GR_FTELL(d_fd);
    }
    d_lasttime = std::chrono::steady_clock::now();
}

receiver::fft_reader::~fft_reader()
{
    stop_threads();
    if(d_fd)
        fclose(d_fd);
    d_fd = nullptr;
}

void receiver::fft_reader::start_threads(int nthreads, rx_fft_c_sptr fft)
{
    busy = nthreads;
    threads.resize(nthreads);
    for(int k=0;k<nthreads;k++)
    {
        task & t=threads[k];
        t.owner=this;
        if(k==0)
            t.d_fft.copy_params(*fft);
        else
            t.d_fft.copy_params(threads[0].d_fft);
        t.samples = t.d_fft.get_fft_size();
        t.d_buf.resize(d_chunk_size * (t.samples / d_samples_per_chunk));
        t.d_fftbuf.resize(t.samples);
        t.index = k;
        t.ready = false;
        t.exit_request = false;
        t.line = 0;
        t.thread = new std::thread(&receiver::fft_reader::task::thread_func,&t);
    }
}

void receiver::fft_reader::stop_threads()
{
    std::unique_lock<std::mutex> lock(mutex);
    for(unsigned k=0;k<threads.size();k++)
    {
        threads[k].exit_request=true;
        threads[k].start.notify_one();
    }
    lock.unlock();
    for(unsigned k=0;k<threads.size();k++)
        threads[k].thread->join();
}

void receiver::fft_reader::reconfigure(std::string filename, int chunk_size, int samples_per_chunk, int sample_rate, uint64_t base_ts, uint64_t offset, any_to_any_base::sptr conv, rx_fft_c_sptr fft, receiver::fft_reader::fft_data_ready handler, int nthreads)
{
    std::unique_lock<std::mutex> lock(mutex);
    while(busy)
        finished.wait(lock);
    bool sample_size_changed = (d_chunk_size != chunk_size) || (d_samples_per_chunk != samples_per_chunk);
    d_chunk_size = chunk_size;
    d_samples_per_chunk = samples_per_chunk;
    d_sample_rate = sample_rate;
    d_base_ts = base_ts;
    d_offset = offset * d_samples_per_chunk;
    d_offset_ms = offset * d_samples_per_chunk * 1000llu / d_sample_rate;
    d_conv = conv;
    if(nthreads == 0)
        nthreads=std::max(1u,std::thread::hardware_concurrency() / 2);
    if(threads.size() != unsigned(nthreads))
    {
        lock.unlock();
        stop_threads();
        lock.lock();
        start_threads(nthreads, fft);
    }else{
        bool update_fft = (fft->get_fft_size() != threads[0].d_fft.get_fft_size());
        update_fft |= (fft->get_window_type() != threads[0].d_fft.get_window_type());
        update_fft |= (fft->get_window_correction() != threads[0].d_fft.get_window_correction());
        for(int k=0;k<nthreads;k++)
        {
            task & t=threads[k];
            if(update_fft)
            {
                if(k==0)
                    t.d_fft.copy_params(*fft);
                else
                    t.d_fft.copy_params(threads[0].d_fft);
                t.samples = t.d_fft.get_fft_size();
                t.d_fftbuf.resize(t.samples);
            }
            if(update_fft || sample_size_changed)
                t.d_buf.resize(d_chunk_size * (t.samples / d_samples_per_chunk));
        }
    }
    data_ready = handler;
    if(d_filename != filename)
    {
        d_filename = filename;
        if(d_fd)
            fclose(d_fd);
        lock.unlock();
        d_fd = fopen(filename.c_str(), "rb");
        if(d_fd)
        {
            GR_FSEEK(d_fd, 0, SEEK_END);
            d_file_size = GR_FTELL(d_fd);
        }
    }
    d_lasttime = std::chrono::steady_clock::now();
}

uint64_t receiver::fft_reader::ms_available()
{
    return d_offset * 1000llu / uint64_t(d_sample_rate);
}

void receiver::fft_reader::wait()
{
    std::unique_lock<std::mutex> lock(mutex);
    while(busy)
        finished.wait(lock);
#ifdef WATERFALL_TIME_BENCHMARK
    std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = now - d_lasttime;
    std::cerr<<"time "<<diff.count()<<"\n";
#endif
}

bool receiver::fft_reader::get_iq_fft_data(uint64_t ms, int n)
{
    uint64_t samp = ms * d_sample_rate / 1000llu;
    int read_ofs = 0;
    unsigned k;
    if(!d_fd)
    {
        return false;
    }
    if(samp > d_offset)
        samp = 0;
    else
        samp = d_offset - samp;
    if(samp>=threads[0].samples)
        GR_FSEEK(d_fd, ((samp - threads[0].samples) / d_samples_per_chunk) * d_chunk_size, SEEK_SET);
    else{
        GR_FSEEK(d_fd, 0, SEEK_SET);
        read_ofs = threads[0].samples - samp;
    }
    std::unique_lock<std::mutex> lock(mutex);
    if(busy == threads.size())
        finished.wait(lock);
    for(k = 0; k < threads.size(); k++)
        if(threads[k].ready)
            break;
    if(k >= threads.size())
        return false;
    busy++;
    lock.unlock();
    if(read_ofs > 0)
        std::memset(threads[k].d_buf.data(), 0, (read_ofs / d_samples_per_chunk) * d_chunk_size);
    size_t nread = fread(&threads[k].d_buf[(read_ofs / d_samples_per_chunk) * d_chunk_size], d_chunk_size, (threads[k].samples - read_ofs) / d_samples_per_chunk, d_fd);
    if(nread != (threads[k].samples - read_ofs) / d_samples_per_chunk)
    {
        //FIXME: Handle error?
    }
    threads[k].line = n;
    threads[k].ts = d_base_ts + d_offset_ms - ms;
    threads[k].ready = false;
    threads[k].start.notify_one();
    return true;
}

void receiver::fft_reader::task::thread_func()
{
    gr_complex * buf;
    unsigned fftsize;
    std::unique_lock<std::mutex> lock(owner->mutex);
    while(1)
    {
        if(owner->busy)
            owner->busy--;
        ready = true;
        owner->finished.notify_one();
        if(exit_request)
            return;
        start.wait(lock);
        if(exit_request)
            return;
        ready = false;
        lock.unlock();

        if(owner->d_conv)
        {
            owner->d_conv->convert(d_buf.data(), d_fftbuf.data(), d_fft.get_fft_size());
            d_fft.get_fft_data(buf, fftsize, d_fftbuf.data());
        }else
            d_fft.get_fft_data(buf, fftsize, (gr_complex*)d_buf.data());
        owner->data_ready(line, buf, (float*)d_fftbuf.data(), fftsize, ts);

        lock.lock();
    }
}

bool receiver::set_value(c_id optid, const c_def::v_union & value)
{
    if(setters[optid])
        return (this->*setters[optid])(value);
    return rx[d_current]->set_value(optid, value);
}

bool receiver::get_value(c_id optid, c_def::v_union & value) const
{
    if(getters[optid])
        return (this->*getters[optid])(value);
    return rx[d_current]->get_value(optid, value);
}

int receiver::conf_initializer()
{
    getters[C_IQ_BUFFERS]=&receiver::get_buffers_max;
    setters[C_IQ_BUFFERS]=&receiver::set_buffers_max;
    getters[C_IQ_PROCESS]=&receiver::get_iq_process;
    setters[C_IQ_PROCESS]=&receiver::set_iq_process;
    getters[C_IQ_REPEAT]=&receiver::get_iq_repeat;
    setters[C_IQ_REPEAT]=&receiver::set_iq_repeat;

    getters[C_IQ_AGC]=&receiver::get_auto_gain;
    setters[C_IQ_AGC]=&receiver::set_auto_gain;
    getters[C_IQ_SWAP]=&receiver::get_iq_swap;
    setters[C_IQ_SWAP]=&receiver::set_iq_swap;
    getters[C_IQ_DCR]=&receiver::get_dc_cancel;
    setters[C_IQ_DCR]=&receiver::set_dc_cancel;
    getters[C_IQ_BALANCE]=&receiver::get_iq_balance;
    setters[C_IQ_BALANCE]=&receiver::set_iq_balance;
    getters[C_PPM]=&receiver::get_freq_corr;
    setters[C_PPM]=&receiver::set_freq_corr;
    getters[C_ANTENNA]=&receiver::get_antenna;
    setters[C_ANTENNA]=&receiver::set_antenna;
    getters[C_CHAN_THREADS]=&receiver::get_channelizer;
    setters[C_CHAN_THREADS]=&receiver::set_channelizer;

    getters[C_HW_FREQ_LABEL]=&receiver::get_hw_freq_label;
    setters[C_FREQ_LOCK_ALL]=&receiver::set_freq_lock_all;
    setters[C_FREQ_UNLOCK_ALL]=&receiver::set_freq_lock_all;
    setters[C_MODE]=&receiver::set_mode;
    getters[C_MODE]=&receiver::get_mode;
    setters[C_SQUELCH_AUTO]=&receiver::set_sql_auto;
    setters[C_SQUELCH_AUTO_GLOBAL]=&receiver::set_sql_auto_global;
    setters[C_SQUELCH_RESET_GLOBAL]=&receiver::reset_sql_global;
    setters[C_AGC_PANNING_AUTO]=&receiver::set_agc_panning_auto;
    setters[C_GLOBAL_MUTE]=&receiver::set_global_mute;
    getters[C_GLOBAL_MUTE]=&receiver::get_global_mute;
    setters[C_AUDIO_PLAY]=&receiver::set_audio_play;
    getters[C_AUDIO_PLAY]=&receiver::get_audio_play;
    return 0;
}

template<> int conf_dispatchers<receiver>::conf_dispatchers_init_dummy(receiver::conf_initializer());
