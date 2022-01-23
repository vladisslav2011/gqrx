/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2021 vladisslav2011@gmail.com.
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

#ifndef GQRX_WAVFILE_SINK_C_H
#define GQRX_WAVFILE_SINK_C_H

//#include <gnuradio/blocks/wavfile.h>

//! WAV file header information.
struct wav_header_info {

    //! sample rate [S/s]
    int sample_rate;

    //! Number of channels
    int nchans;

    //! Bytes per sample
    int bytes_per_sample;

    //! Number of samples per channel
    long long samples_per_chan;

    //! sndfile format
    int format;

    //! sndfile format
    int subformat;
};

enum wavfile_format_t {
    FORMAT_WAV = 0x010000,
    FORMAT_FLAC = 0x170000,
    FORMAT_OGG = 0x200000,
    FORMAT_RF64 = 0x220000,
};

enum wavfile_subformat_t {
    FORMAT_PCM_S8 = 1,
    FORMAT_PCM_16,
    FORMAT_PCM_24,
    FORMAT_PCM_32,
    FORMAT_PCM_U8,
    FORMAT_FLOAT,
    FORMAT_DOUBLE,
    FORMAT_VORBIS = 0x0060,
};

#include <gnuradio/blocks/wavfile_sink.h>
#include <sndfile.h> // for SNDFILE
#include <thread>
#include <condition_variable>

class wavfile_sink_gqrx : public gr::blocks::wavfile_sink
{
private:
    wav_header_info d_h;
    int d_bytes_per_sample_new;
    bool d_append;

    std::vector<float> d_buffer;

    SNDFILE* d_fp;
    SNDFILE* d_new_fp;
    bool d_updated;
    std::mutex d_mutex;

    static constexpr int s_items_size = 8192;
    static constexpr int s_max_channels = 24;

    /*!
     * \brief If any file changes have occurred, update now. This is called
     * internally by work() and thus doesn't usually need to be called by
     * hand.
     */
    void do_update();

    /*!
     * \brief Implementation of set_bits_per_sample without mutex lock.
     */
    void set_bits_per_sample_unlocked(int bits_per_sample);

    /*!
     * \brief Writes information to the WAV header which is not available
     * a-priori (chunk size etc.) and closes the file. Not thread-safe and
     * assumes d_fp is a valid file pointer, should thus only be called by
     * other methods.
     */
    void close_wav();

protected:
    bool stop() override;

public:
#if GNURADIO_VERSION < 0x030900
    typedef boost::shared_ptr<wavfile_sink_gqrx> sptr;
#else
    typedef std::shared_ptr<wavfile_sink_gqrx> sptr;
#endif
static sptr make(const char* filename,
                 int n_channels,
                 unsigned int sample_rate,
                 wavfile_format_t format,
                 wavfile_subformat_t subformat,
                 bool append = false);
    wavfile_sink_gqrx(const char* filename,
                      int n_channels,
                      unsigned int sample_rate,
                      wavfile_format_t format,
                      wavfile_subformat_t subformat,
                      bool append = false);
    ~wavfile_sink_gqrx() override;

    bool open(const char* filename) override;
    void close() override;

    void set_sample_rate(unsigned int sample_rate) override;
    void set_bits_per_sample(int bits_per_sample) override;
    void set_append(bool append);

    int bits_per_sample();
    unsigned int sample_rate();

    int work(int noutput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items) override;
};

#endif /* GQRX_WAVFILE_SINK_C_H */
