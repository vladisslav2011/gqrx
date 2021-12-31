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
#ifndef RECEIVER_H
#define RECEIVER_H

#if GNURADIO_VERSION < 0x030800
#include <gnuradio/blocks/multiply_const_ff.h>
#else
#include <gnuradio/blocks/multiply_const.h>
#endif

//#include <gnuradio/blocks/file_sink.h>
//#include <gnuradio/blocks/file_source.h>
#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/blocks/wavfile_sink.h>
#include <gnuradio/blocks/wavfile_source.h>
#include <gnuradio/blocks/throttle.h>
#include <gnuradio/top_block.h>
#include <osmosdr/source.h>
#include <string>

#include "dsp/correct_iq_cc.h"
#include "dsp/downconverter.h"
#include "dsp/filter/fir_decim.h"
#include "dsp/rx_noise_blanker_cc.h"
#include "dsp/rx_filter.h"
#include "dsp/rx_meter.h"
#include "dsp/rx_agc_xx.h"
#include "dsp/rx_demod_fm.h"
#include "dsp/rx_demod_am.h"
#include "dsp/rx_fft.h"
#include "dsp/sniffer_f.h"
#include "dsp/resampler_xx.h"
#include "dsp/format_converter.h"
#include "interfaces/udp_sink_f.h"
#include "interfaces/file_sink.h"
#include "interfaces/file_source.h"
#include "receivers/receiver_base.h"

#ifdef WITH_PULSEAUDIO
#include "pulseaudio/pa_sink.h"
#elif WITH_PORTAUDIO
#include "portaudio/portaudio_sink.h"
#else
#include <gnuradio/audio/sink.h>
#endif

/**
 * @defgroup DSP Digital signal processing library based on GNU Radio
 */

/**
 * @brief Top-level receiver class.
 * @ingroup DSP
 *
 * This class encapsulates the GNU Radio flow graph for the receiver.
 * Front-ends should only control the receiver through the interface provided
 * by this class.
 */
class receiver
{

public:

    /** Flag used to indicate success or failure of an operation */
    enum status {
        STATUS_OK    = 0, /*!< Operation was successful. */
        STATUS_ERROR = 1  /*!< There was an error. */
    };

    /** Available demodulators. */
    enum rx_demod {
        RX_DEMOD_OFF   = 0,  /*!< No receiver. */
        RX_DEMOD_NONE  = 1,  /*!< No demod. Raw I/Q to audio. */
        RX_DEMOD_AM    = 2,  /*!< Amplitude modulation. */
        RX_DEMOD_NFM   = 3,  /*!< Frequency modulation. */
        RX_DEMOD_WFM_M = 4,  /*!< Frequency modulation (wide, mono). */
        RX_DEMOD_WFM_S = 5,  /*!< Frequency modulation (wide, stereo). */
        RX_DEMOD_WFM_S_OIRT = 6,  /*!< Frequency modulation (wide, stereo oirt). */
        RX_DEMOD_SSB   = 7,  /*!< Single Side Band. */
        RX_DEMOD_AMSYNC = 8  /*!< Amplitude modulation (synchronous demod). */
    };

    /** Supported receiver types. */
    enum rx_chain {
        RX_CHAIN_NONE  = 0,   /*!< No receiver, just spectrum analyzer. */
        RX_CHAIN_NBRX  = 1,   /*!< Narrow band receiver (AM, FM, SSB). */
        RX_CHAIN_WFMRX = 2    /*!< Wide band FM receiver (for broadcast). */
    };

    /** Filter shape (convenience wrappers for "transition width"). */
    enum filter_shape {
        FILTER_SHAPE_SOFT = 0,   /*!< Soft: Transition band is TBD of width. */
        FILTER_SHAPE_NORMAL = 1, /*!< Normal: Transition band is TBD of width. */
        FILTER_SHAPE_SHARP = 2   /*!< Sharp: Transition band is TBD of width. */
    };

    struct iq_tool_stats
    {
        bool recording;
        bool playing;
        bool failed;
        int buffer_usage;
        size_t file_pos;
     };

    receiver(const std::string input_device="",
             const std::string audio_device="",
             unsigned int decimation=1);
    ~receiver();

    void        start();
    void        stop();
    void        set_input_device(const std::string device);
    void        set_output_device(const std::string device);
    void        set_input_file(const std::string name, const int sample_rate,
                               const file_formats fmt, int buffers_max,
                               bool repeat);

    std::vector<std::string> get_antennas(void) const;
    void        set_antenna(const std::string &antenna);

    double      set_input_rate(double rate);
    double      get_input_rate(void) const { return d_input_rate; }

    unsigned int    set_input_decim(unsigned int decim);
    unsigned int    get_input_decim(void) const { return d_decim; }

    double      get_quad_rate(void) const {
        return d_input_rate / (double)d_decim;
    }

    double      set_analog_bandwidth(double bw);
    double      get_analog_bandwidth(void) const;

    void        set_iq_swap(bool reversed);
    bool        get_iq_swap(void) const;

    void        set_dc_cancel(bool enable);
    bool        get_dc_cancel(void) const;

    void        set_iq_balance(bool enable);
    bool        get_iq_balance(void) const;

    status      set_rf_freq(double freq_hz);
    double      get_rf_freq(void);
    status      get_rf_range(double *start, double *stop, double *step);

    std::vector<std::string>    get_gain_names();
    status      get_gain_range(std::string &name, double *start, double *stop,
                               double *step) const;
    status      set_auto_gain(bool automatic);
    status      set_gain(std::string name, double value);
    double      get_gain(std::string name) const;

    status      set_filter_offset(double offset_hz);
    double      get_filter_offset(void) const;
    status      set_cw_offset(double offset_hz);
    double      get_cw_offset(void) const;
    status      set_filter(double low, double high, filter_shape shape);
    status      set_freq_corr(double ppm);
    float       get_signal_pwr() const;
    void        set_iq_fft_size(int newsize);
    void        set_iq_fft_window(int window_type);
    void        get_iq_fft_data(std::complex<float>* fftPoints,
                                unsigned int &fftsize);
    void        get_audio_fft_data(std::complex<float>* fftPoints,
                                   unsigned int &fftsize);

    /* Noise blanker */
    status      set_nb_on(int nbid, bool on);
    status      set_nb_threshold(int nbid, float threshold);

    /* Squelch parameter */
    status      set_sql_level(double level_db);
    status      set_sql_alpha(double alpha);

    /* AGC */
    status      set_agc_on(bool agc_on);
    status      set_agc_hang(bool use_hang);
    status      set_agc_threshold(int threshold);
    status      set_agc_slope(int slope);
    status      set_agc_decay(int decay_ms);
    status      set_agc_manual_gain(int gain);

    status      set_demod(rx_demod demod,
                          file_formats fmt = FILE_FORMAT_LAST,
                          bool force = false);

    /* FM parameters */
    status      set_fm_maxdev(float maxdev_hz);
    status      set_fm_deemph(double tau);

    /* AM parameters */
    status      set_am_dcr(bool enabled);

    /* AM-Sync parameters */
    status      set_amsync_dcr(bool enabled);
    status      set_amsync_pll_bw(float pll_bw);

    /* Audio parameters */
    status      set_af_gain(float gain_db);
    status      start_audio_recording(const std::string filename);
    status      stop_audio_recording();
    status      start_audio_playback(const std::string filename);
    status      stop_audio_playback();

    status      start_udp_streaming(const std::string host, int port, bool stereo);
    status      stop_udp_streaming();

    /* I/Q recording and playback */
    status      start_iq_recording(const std::string filename, const file_formats fmt, int buffers_max);
    status      stop_iq_recording();
    status      seek_iq_file(long pos);
    void        get_iq_tool_stats(struct iq_tool_stats &stats);
    bool        is_playing_iq() { return d_iq_fmt != FILE_FORMAT_NONE; }

    /* sample sniffer */
    status      start_sniffer(unsigned int samplrate, int buffsize);
    status      stop_sniffer();
    void        get_sniffer_data(float * outbuff, unsigned int &num);

    bool        is_recording_audio(void) const { return d_recording_wav; }
    bool        is_snifffer_active(void) const { return d_sniffer_active; }

    /* rds functions */
    void        get_rds_data(std::string &outbuff, int &num);
    void        start_rds_decoder(void);
    void        stop_rds_decoder();
    bool        is_rds_decoder_active(void) const;
    void        reset_rds_parser(void);

    /* utility functions */
    static std::string escape_filename(std::string filename);

private:
    void        connect_all(rx_chain type, file_formats fmt);
    void        setup_source(file_formats fmt);
    status      connect_iq_recorder();

private:
    bool        d_running;          /*!< Whether receiver is running or not. */
    double      d_input_rate;       /*!< Input sample rate. */
    double      d_decim_rate;       /*!< Rate after decimation (input_rate / decim) */
    double      d_quad_rate;        /*!< Quadrature rate (after down-conversion) */
    double      d_audio_rate;       /*!< Audio output rate. */
    unsigned int    d_decim;        /*!< input decimation. */
    unsigned int    d_ddc_decim;    /*!< Down-conversion decimation. */
    double      d_rf_freq;          /*!< Current RF frequency. */
    double      d_filter_offset;    /*!< Current filter offset */
    double      d_cw_offset;        /*!< CW offset */
    bool        d_recording_iq;     /*!< Whether we are recording I/Q file. */
    bool        d_recording_wav;    /*!< Whether we are recording WAV file. */
    bool        d_sniffer_active;   /*!< Only one data decoder allowed. */
    bool        d_iq_rev;           /*!< Whether I/Q is reversed or not. */
    bool        d_dc_cancel;        /*!< Enable automatic DC removal. */
    bool        d_iq_balance;       /*!< Enable automatic IQ balance. */
    file_formats d_iq_fmt;
    file_formats d_last_format;

    std::string input_devstr;  /*!< Current input device string. */
    std::string output_devstr; /*!< Current output device string. */

    rx_demod    d_demod;       /*!< Current demodulator. */

    gr::top_block_sptr         tb;        /*!< The GNU Radio top block. */

    osmosdr::source::sptr     src;       /*!< Real time I/Q source. */
    fir_decim_cc_sptr         input_decim;      /*!< Input decimator. */
    receiver_base_cf_sptr     rx;        /*!< receiver. */

    dc_corr_cc_sptr           dc_corr;   /*!< DC corrector block. */
    iq_swap_cc_sptr           iq_swap;   /*!< I/Q swapping block. */

    rx_fft_c_sptr             iq_fft;     /*!< Baseband FFT block. */
    rx_fft_f_sptr             audio_fft;  /*!< Audio FFT block. */

    downconverter_cc_sptr     ddc;        /*!< Digital down-converter for demod chain. */

    gr::blocks::multiply_const_ff::sptr audio_gain0; /*!< Audio gain block. */
    gr::blocks::multiply_const_ff::sptr audio_gain1; /*!< Audio gain block. */
    gr::blocks::multiply_const_ff::sptr wav_gain0; /*!< WAV file gain block. */
    gr::blocks::multiply_const_ff::sptr wav_gain1; /*!< WAV file gain block. */

    file_sink::sptr         iq_sink;     /*!< I/Q file sink. */

    //Format converters to/from different sample formats
    std::vector<any_to_any_base::sptr> convert_to
    {
        nullptr,
        nullptr,
        nullptr,
        any_to_any<gr_complex,std::complex<int8_t>>::make(),
        any_to_any<gr_complex,std::complex<int16_t>>::make(),
        any_to_any<gr_complex,std::complex<int32_t>>::make(),
        any_to_any<gr_complex,std::complex<uint8_t>>::make(),
        any_to_any<gr_complex,std::complex<uint16_t>>::make(),
        any_to_any<gr_complex,std::complex<uint32_t>>::make(),
        any_to_any<gr_complex,std::array<int8_t,40>>::make(),
        any_to_any<gr_complex,std::array<int8_t,24>>::make(),
        any_to_any<gr_complex,std::array<int8_t,56>>::make(),
        any_to_any<gr_complex,int8_t>::make(),
        any_to_any<gr_complex,int16_t>::make(),
        any_to_any<gr_complex,std::array<int16_t,20>>::make(),
        any_to_any<gr_complex,std::array<int16_t,12>>::make(),
        any_to_any<gr_complex,std::array<int16_t,28>>::make(),
        nullptr
    };
    std::vector<any_to_any_base::sptr> convert_from
    {
        nullptr,
        nullptr,
        nullptr,
        any_to_any<std::complex<int8_t>,gr_complex>::make(),
        any_to_any<std::complex<int16_t>,gr_complex>::make(),
        any_to_any<std::complex<int32_t>,gr_complex>::make(),
        any_to_any<std::complex<uint8_t>,gr_complex>::make(),
        any_to_any<std::complex<uint16_t>,gr_complex>::make(),
        any_to_any<std::complex<uint32_t>,gr_complex>::make(),
        any_to_any<std::array<int8_t,40>,gr_complex>::make(),
        any_to_any<std::array<int8_t,24>,gr_complex>::make(),
        any_to_any<std::array<int8_t,56>,gr_complex>::make(),
        any_to_any<int8_t,gr_complex>::make(),
        any_to_any<int16_t,gr_complex>::make(),
        any_to_any<std::array<int16_t,20>,gr_complex>::make(),
        any_to_any<std::array<int16_t,12>,gr_complex>::make(),
        any_to_any<std::array<int16_t,28>,gr_complex>::make(),
        nullptr
    };

    gr::blocks::throttle::sptr                     input_throttle;
    file_source::sptr                              input_file;

    gr::blocks::wavfile_sink::sptr      wav_sink;   /*!< WAV file sink for recording. */
    gr::blocks::wavfile_source::sptr    wav_src;    /*!< WAV file source for playback. */
    gr::blocks::null_sink::sptr         audio_null_sink0; /*!< Audio null sink used during playback. */
    gr::blocks::null_sink::sptr         audio_null_sink1; /*!< Audio null sink used during playback. */

    udp_sink_f_sptr   audio_udp_sink;  /*!< UDP sink to stream audio over the network. */
    sniffer_f_sptr    sniffer;    /*!< Sample sniffer for data decoders. */
    resampler_ff_sptr sniffer_rr; /*!< Sniffer resampler. */

#ifdef WITH_PULSEAUDIO
    pa_sink_sptr              audio_snk;  /*!< Pulse audio sink. */
#elif WITH_PORTAUDIO
    portaudio_sink_sptr       audio_snk;  /*!< portaudio sink */
#else
    gr::audio::sink::sptr     audio_snk;  /*!< gr audio sink */
#endif

    //! Get a path to a file containing random bytes
    static std::string get_zero_file(void);
};

#endif // RECEIVER_H
