/* -*- c++ -*- */
/*
 * Copyright 2012, 2018 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef GQRX_FILE_SOURCE_C_H
#define GQRX_FILE_SOURCE_C_H

#include <gnuradio/blocks/api.h>
#include <gnuradio/sync_block.h>
#include <thread>
#include <atomic>
#include <queue>
#include <condition_variable>

class BLOCKS_API file_source : public gr::sync_block
{
public:
#if GNURADIO_VERSION < 0x030900
    typedef boost::shared_ptr<file_source> sptr;
#else
    typedef std::shared_ptr<file_source> sptr;
#endif

    /*!
    * \brief Make a file sink.
    * \param itemsize size of the input data items.
    * \param filename name of the file to open and write output to.
    * \param append if true, data is appended to the file instead of
    *        overwriting the initial content.
    */
    static sptr make(size_t itemsize, const char *filename, uint64_t offset,
                     uint64_t len, int sample_rate, uint64_t time_ms,
                     bool repeat = false, int buffers_max = 1);

private:
    typedef std::function<void(int64_t)> iq_save_progress_t;
    size_t d_itemsize;
    uint64_t d_start_offset_items;
    uint64_t d_length_items;
    std::atomic<uint64_t> d_items_remaining;
    FILE* d_fp;
    FILE* d_new_fp;
    bool d_repeat;
    bool d_updated;
    bool d_file_begin;
    bool d_seekable;
    long d_repeat_cnt;
    pmt::pmt_t d_add_begin_tag;

    std::mutex d_mutex;
    std::vector<uint8_t> d_buf;
    uint8_t *    d_rp;
    uint8_t *    d_wp;
    std::condition_variable d_reader_wake;
    std::condition_variable d_reader_ready;
    bool         d_reader_finish;
    std::thread * d_reader_thread;
    bool         d_failed;
    bool         d_eof;
    bool         d_closing;
    bool         d_seek;
    int          d_sample_rate;
    bool         d_buffering;
    uint64_t     d_seek_point;
    uint64_t     d_buffer_size;
    int          d_seek_ok;
    uint64_t     d_time_ms;
    pmt::pmt_t _id;
    std::string  d_filename;
    std::mutex   d_save_mutex{};
    std::thread *d_save_thread{};
    uint64_t     d_save_from_ms;
    uint64_t     d_save_len_ms;
    std::string  d_save_to;
    iq_save_progress_t d_save_progress{};
    uint64_t     d_save_written_ms{0};
    std::atomic<bool> d_save_terminate{false};
#if 0
    void do_update();
#endif
    void reader();
    void save_thread_fn();

public:
    file_source(size_t itemsize,
                    const char* filename,
                    uint64_t offset,
                    uint64_t len,
                    int sample_rate,
                    uint64_t time_ms,
                    bool repeat = false,
                    int buffers_max = 8);
    ~file_source();

    bool seek(int64_t seek_point, int whence);
    bool seek_ts(uint64_t ts, uint64_t &res_point);
    void open(const char* filename, bool repeat, uint64_t offset, uint64_t len);
    void close();
    uint64_t tell();
    uint64_t get_size() { return d_length_items - d_start_offset_items; }
    bool get_failed() const { return d_failed;};
    int get_buffer_usage();

    int work(int noutput_items,
            gr_vector_const_void_star& input_items,
            gr_vector_void_star& output_items);

    void set_begin_tag(pmt::pmt_t val);
    uint64_t get_timestamp_ms();
    uint64_t get_items_remaining();
    bool save_ts(const uint64_t from_ms, const uint64_t len_ms, const std::string name);
    template<typename T> void set_save_progress_cb(T save_progress)
    {
        d_save_progress = save_progress;
    }
};

#endif
