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
#include <gnuradio/blocks/add_ff.h>
#else
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/add_blk.h>
#endif

//#include <gnuradio/blocks/file_sink.h>
//#include <gnuradio/blocks/file_source.h>
#include <gnuradio/blocks/null_sink.h>
//temporary workaround to make add_ff happy
#include <gnuradio/blocks/null_source.h>
//emd workaround
#include <gnuradio/blocks/wavfile_source.h>
#include <gnuradio/blocks/throttle.h>
#include <gnuradio/top_block.h>
#include <osmosdr/source.h>
#include <string>
#include <memory>
#include <atomic>

#include "dsp/correct_iq_cc.h"
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
#include "interfaces/audio_sink.h"
#include "applications/gqrx/dcontrols.h"

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
class receiver: public conf_dispatchers<receiver>
{

public:

    /** Flag used to indicate success or failure of an operation */
    enum status {
        STATUS_OK    = 0, /*!< Operation was successful. */
        STATUS_ERROR = -1  /*!< There was an error. */
    };

    enum dcr_mode {
        DCR_OFF = 0,
        DCR_MANUAL,
        DCR_AUTO,
        DCR_OFF_SW,
        DCR_MANUAL_SW,
    };

    /** Filter shape (convenience wrappers for "transition width"). */
    typedef Modulations::filter_shape filter_shape;

    struct iq_tool_stats
    {
        bool recording;
        bool playing;
        bool failed;
        int buffer_usage;
        size_t file_pos;
        size_t sample_pos;
     };

    typedef std::function<void(int64_t)> iq_save_progress_t;

    struct fft_reader
    {
        typedef std::function<void(int, gr_complex*, float*, unsigned, uint64_t)> fft_data_ready;
        fft_reader(std::string filename, int chunk_size, int samples_per_chunk, int sample_rate, uint64_t base_ts,uint64_t offset, any_to_any_base::sptr conv, rx_fft_c_sptr fft, fft_data_ready handler, int nthreads=0);
        ~fft_reader();
        void start_threads(int nthreads, rx_fft_c_sptr fft);
        void stop_threads();
        void reconfigure(std::string filename, int chunk_size, int samples_per_chunk, int sample_rate, uint64_t base_ts, uint64_t offset, any_to_any_base::sptr conv, rx_fft_c_sptr fft, receiver::fft_reader::fft_data_ready handler, int nthreads);
        uint64_t ms_available();
        bool get_iq_fft_data(uint64_t ms, int n);
        void wait();
        private:
        struct task
        {
            task(){thread = nullptr;};
            task(task &from){thread = nullptr;};
            task(task &&from){thread = nullptr;};
            ~task(){if(thread)delete thread;};
            void thread_func();
            struct fft_reader * owner;
            int index;
            bool ready;
            bool exit_request;
            int line;
            unsigned samples;
            uint64_t ts;
            fft_c_basic d_fft;
            std::vector<uint8_t> d_buf;
            std::vector<gr_complex> d_fftbuf;
            std::thread *thread;
            std::condition_variable start{};
        };
        std::string d_filename;
        FILE * d_fd;
        int d_chunk_size;
        int d_samples_per_chunk;
        int d_sample_rate;
        uint64_t d_base_ts;
        uint64_t d_offset;
        uint64_t d_offset_ms;
        uint64_t d_file_size;
        any_to_any_base::sptr d_conv;
        fft_data_ready data_ready;
        std::vector<task> threads;
        std::mutex mutex;
        std::condition_variable finished;
        std::atomic<unsigned> busy;
        std::chrono::time_point<std::chrono::steady_clock> d_lasttime;
    };
    typedef std::shared_ptr<fft_reader> fft_reader_sptr;

    struct iqfile_timestamp_source:wavfile_sink_gqrx::timestamp_source
    {
        uint64_t get() override
        {
            if(iqfile)
                return iqfile->get_timestamp_ms();
            else
                return time(nullptr) * 1000llu;
        }
        void set_file_source(file_source::sptr value)
        {
            iqfile = value;
        }
        private:
            file_source::sptr iqfile;
    };

    receiver(const std::string input_device="",
             const std::string audio_device="",
             unsigned int decimation=1);
    virtual ~receiver();

    void        start();
    void        stop();
    bool        is_running() const {return d_running;}
    void        set_input_device(const std::string device);
    void        set_output_device(const std::string device);
    void        set_input_file(const std::string name, const int sample_rate,
                               const file_formats fmt, uint64_t time_ms);

    std::vector<std::string> get_antennas(void) const;
    bool        set_antenna(const c_def::v_union &);
    bool        get_antenna(c_def::v_union &) const;

    double      set_input_rate(double rate);
    double      get_input_rate(void) const { return d_input_rate; }

    unsigned int    set_input_decim(unsigned int decim);
    unsigned int    get_input_decim(void) const { return d_decim; }

    double      get_quad_rate(void) const {
        return d_input_rate / (double)d_decim;
    }

    double      set_analog_bandwidth(double bw);
    double      get_analog_bandwidth(void) const;

    bool        set_iq_swap(const c_def::v_union &);
    bool        get_iq_swap(c_def::v_union &) const;

    bool        set_dc_cancel(const c_def::v_union &);
    bool        get_dc_cancel(c_def::v_union &) const;

    bool        set_iq_balance(const c_def::v_union &);
    bool        get_iq_balance(c_def::v_union &) const;

    status      set_rf_freq(double freq_hz);
    double      get_rf_freq(void);
    status      get_rf_range(double *start, double *stop, double *step);
    bool        get_hw_freq_label(c_def::v_union &) const;

    std::vector<std::string>    get_gain_names();
    status      get_gain_range(std::string &name, double *start, double *stop,
                               double *step) const;
    bool        set_auto_gain(const c_def::v_union &);
    bool        get_auto_gain(c_def::v_union &) const;
    status      set_gain(std::string name, double value);
    double      get_gain(std::string name) const;

    int         add_rx();
    int         get_rx_count();
    int         delete_rx();
    status      select_rx(int no);
    status      fake_select_rx(int no);
    int         get_current();
    vfo::sptr   get_current_vfo();
    vfo::sptr   get_vfo(int n);
    vfo::sptr   find_vfo(int64_t freq);
    std::vector<vfo::sptr> get_vfos();

    status      set_filter_offset(double offset_hz, bool locked=true);
    status      set_filter_offset(int rx_index, double offset_hz, bool locked=true);
    double      get_filter_offset(void) const;
    bool        set_freq_lock_all(const c_def::v_union & v);

    status      set_filter(int low, int high, filter_shape shape);
    status      get_filter(int &low, int &high, filter_shape &shape);
    bool        set_freq_corr(const c_def::v_union &);
    bool        get_freq_corr(c_def::v_union &) const;
    float       get_signal_pwr() const;
    void        set_iq_fft_size(int newsize);
    void        set_iq_fft_window(int window_type, int correction);
    void        get_iq_fft_data(std::complex<float>* fftPoints,
                                unsigned int &fftsize);
    void        set_iq_fft_enabled(bool enabled);
    void        get_audio_fft_data(std::complex<float>* fftPoints,
                                   unsigned int &fftsize);
    void        set_audio_fft_enabled(bool enabled);
    bool        set_audio_fft_size(const c_def::v_union & v);
    bool        get_audio_fft_size(c_def::v_union & v) const;

    /* FFT Probe */
    void        get_probe_fft_data(std::complex<float>* fftPoints,
                                   unsigned int &fftsize);
    void        set_probe_channel(int c);
    int         get_probe_channel();
    int         get_probe_channel_count();
    void        set_chan_decim(int n);
    void        set_chan_osr(int n);
    int         get_chan_decim(){return chan->decim();}
    int         get_chan_osr(){return chan->osr();}
    void        set_chan_filter_param(float n);
    bool        get_channelizer(c_def::v_union &) const;
    bool        set_channelizer(const c_def::v_union &);

    /* Squelch parameter */
    bool        set_sql_auto(const c_def::v_union &);
    bool        set_sql_auto_global(const c_def::v_union &);
    bool        reset_sql_global(const c_def::v_union &);

    float       get_agc_gain();

    /* Mute */
    bool        set_global_mute(const c_def::v_union &);
    bool        get_global_mute(c_def::v_union &) const;
    void        updateAudioVolume();
    /* Demod */
    status      set_demod_locked(Modulations::idx demod, int old_idx = -1);
    status      set_demod(Modulations::idx demod, int old_idx = -1);
    void        set_demod_and_update_filter(receiver_base_cf_sptr old_rx, receiver_base_cf_sptr new_rx, Modulations::idx demod);
    Modulations::idx get_demod() {return rx[d_current]->get_demod();}
    status      reconnect_all(file_formats fmt = FILE_FORMAT_LAST,
                          bool force = false);
    bool        set_mode(const c_def::v_union &);
    bool        get_mode(c_def::v_union &) const;

    /* Audio parameters */
    status      set_audio_rate(int rate);
    status      commit_audio_rate();
    int         get_audio_rate();
    bool        get_audio_play(c_def::v_union &) const;
    bool        set_audio_play(const c_def::v_union &);

    /* I/Q recording and playback */
    bool        get_buffers_max(c_def::v_union &v) const { v=d_iq_buffers_max; return true;}
    bool        set_buffers_max(const c_def::v_union &);
    bool        get_iq_process(c_def::v_union &v) const { v=d_iq_process; return true;}
    bool        set_iq_process(const c_def::v_union &);
    bool        get_iq_repeat(c_def::v_union &v) const { v=d_iq_repeat; return true;}
    bool        set_iq_repeat(const c_def::v_union &);
    status      start_iq_recording(const std::string filename, const file_formats fmt);
    status      stop_iq_recording();
    status      seek_iq_file(long pos);
    status      seek_iq_file_ts(uint64_t ts, uint64_t &res_point);
    void        get_iq_tool_stats(struct iq_tool_stats &stats);
    uint64_t    get_iq_file_size() { return input_file->get_size(); }
    bool        is_playing_iq() { return d_last_format != FILE_FORMAT_NONE; }
    bool        is_recording_iq() { return d_iq_fmt != FILE_FORMAT_NONE; }
    status      save_file_range_ts(const uint64_t from_ms, const uint64_t len_ms, const std::string name);
    template <typename T> void set_iq_save_progress_cb(T handler)
    {
        d_save_progress = handler;
    }

    /* sample sniffer */
    status      start_sniffer(unsigned int samplrate, int buffsize);
    status      stop_sniffer();
    void        get_sniffer_data(float * outbuff, unsigned int &num);

    bool        is_snifffer_active(void) const { return d_sniffer_active; }


    /* utility functions */
    static std::string escape_filename(std::string filename);
    uint64_t get_filesource_timestamp_ms();
    fft_reader_sptr get_fft_reader(uint64_t offset, receiver::fft_reader::fft_data_ready cb, int nthreads);
    file_formats get_last_format() const { return d_last_format; }

    //arbitrary option setters/getters
    bool set_value(c_id optid, const c_def::v_union & value) override;
    bool get_value(c_id optid, c_def::v_union & value) const override;
    static int conf_initializer();

private:
    void        connect_all(file_formats fmt);
    void        connect_rx();
    void        connect_rx(int n);
    void        disconnect_rx();
    void        disconnect_rx(int n);
    void        foreground_rx();
    void        background_rx();
    gr::basic_block_sptr setup_source(file_formats fmt);
    status      connect_iq_recorder();
    void        set_channelizer_int(bool use_chan);
    void        configure_channelizer(bool reconnect);
    bool        set_agc_panning_auto(const c_def::v_union &);
    void        update_agc_panning_auto(int rx_index);

private:
    int         d_current;          /*!< Current selected demodulator. */
    int         d_active;           /*!< Active demodulator count. */
    bool        d_running;          /*!< Whether receiver is running or not. */
    double      d_input_rate;       /*!< Input sample rate. */
    double      d_decim_rate;       /*!< Rate after decimation (input_rate / decim) */
    bool        d_use_chan;         /*!< Use channelizer instead of direct connection */
    bool        d_enable_chan;      /*!< Enable channelizer usage when input rate is high enough */
    double      d_audio_rate;       /*!< Audio output rate. */
    unsigned int    d_decim;        /*!< input decimation. */
    double      d_rf_freq;          /*!< Current RF frequency. */
    std::string d_iq_filename;
    uint64_t    d_iq_time_ms;
    int         d_iq_buffers_max;
    bool        d_iq_process;
    bool        d_iq_repeat;
    bool        d_sniffer_active;   /*!< Only one data decoder allowed. */
    bool        d_iq_rev;           /*!< Whether I/Q is reversed or not. */
    dcr_mode    d_dc_cancel;        /*!< Enable automatic DC removal. */
    bool        d_iq_balance;       /*!< Enable automatic IQ balance. */
    bool        d_mute;             /*!< Enable audio mute. */
    file_formats d_iq_fmt;
    file_formats d_last_format;
    iqfile_timestamp_source d_iq_ts;

    std::string input_devstr;       /*!< Current input device string. */
    std::string output_devstr;      /*!< Current output device string. */

    gr::basic_block_sptr iq_src;    /*!< Points to the block, connected to rx[]. */

    gr::top_block_sptr         tb;        /*!< The GNU Radio top block. */

    osmosdr::source::sptr     src;       /*!< Real time I/Q source. */
    fir_decim_cc_sptr         input_decim;      /*!< Input decimator. */
    std::vector<receiver_base_cf_sptr> rx;     /*!< receiver. */
    gr::blocks::add_ff::sptr           add0;   /* Audio downmix */
    gr::blocks::add_ff::sptr           add1;   /* Audio downmix */
    gr::blocks::multiply_const_ff::sptr mc0;   /* Audio downmix */
    gr::blocks::multiply_const_ff::sptr mc1;   /* Audio downmix */
    gr::blocks::null_source::sptr null_src;    /* temporary workaround */

    dc_corr_cc_sptr           dc_corr;   /*!< DC corrector block. */
    iq_swap_cc_sptr           iq_swap;   /*!< I/Q swapping block. */

    fft_channelizer_cc::sptr  chan;
    rx_fft_c_sptr             probe_fft;  /*!< Probe FFT block. */
    rx_fft_c_sptr             iq_fft;     /*!< Baseband FFT block. */
    rx_fft_f_sptr             audio_fft;  /*!< Audio FFT block. */

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

    gr::blocks::wavfile_source::sptr    wav_src;    /*!< WAV file source for playback. */
    gr::blocks::null_sink::sptr         audio_null_sink0; /*!< Audio null sink used during playback. */
    gr::blocks::null_sink::sptr         audio_null_sink1; /*!< Audio null sink used during playback. */

    sniffer_f_sptr    sniffer;    /*!< Sample sniffer for data decoders. */
    resampler_ff_sptr sniffer_rr; /*!< Sniffer resampler. */

    gr::basic_block_sptr  audio_snk;  /*!< Pulse audio sink. */
    //! Get a path to a file containing random bytes
    receiver::fft_reader_sptr d_fft_reader;
    static std::string get_zero_file(void);
     iq_save_progress_t d_save_progress{};
};

#endif // RECEIVER_H
