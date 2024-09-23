/*
 * Copyright (C) 2014 Bastian Bloessl <bloessl@ccs-labs.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef INCLUDED_RDS_DECODER_IMPL_H
#define INCLUDED_RDS_DECODER_IMPL_H

#include "dsp/rds/decoder.h"
#include <atomic>
#include <vector>
#include <array>
#include <map>

#define PS_SEARCH_MAX 256

namespace gr {
namespace rds {

class decoder_impl : public decoder
{
public:
	decoder_impl(bool log, bool debug);
	void set_ecc_max(int n) {d_ecc_max = n;}
	void reset();
	void set_integrate_pi(unsigned mode) {d_integrate_pi = mode;}
	void set_integrate_ps(unsigned mode) {d_integrate_ps = mode;}
	void set_integrate_ps_dist(unsigned dist) {d_integrate_ps_dist = dist;}

private:
    constexpr static int BLOCK_SIZE{26};
    constexpr static int GROUP_SIZE{26*4};
    struct bit_locator
    {
        uint16_t l;
        uint8_t w;
    };
    struct pi_stats
    {
        uint8_t count;
        uint8_t weight;
        unsigned lastseen;
    };
    struct grp_array
    {
        unsigned int data[4];
        grp_array(const unsigned int * from)
        {
            std::memcpy(data,from,sizeof(data));
        }
    };
	~decoder_impl();

	int work(int noutput_items,
			gr_vector_const_void_star &input_items,
			gr_vector_void_star &output_items);
    bool start() override;

	void enter_no_sync();
	void enter_sync(unsigned int);
	static unsigned int calc_syndrome(unsigned long, unsigned char);
	void decode_group(unsigned *, int);
	static std::array<bit_locator,1024> build_locator();
    int process_group(unsigned * grp, int thr=0, unsigned char * offs_chars=nullptr, uint16_t * loc=nullptr, int * good_grp=nullptr);
    int pi_detect(uint32_t * p_grp, bool corr);
	void reset_corr();

    static constexpr int n_group{4*1};
	int            bit_counter;
	unsigned long  lastseen_offset_counter, reg;
	unsigned int   block_bit_counter;
	unsigned int   wrong_blocks_counter;
	unsigned int   blocks_counter;
	unsigned int   group_good_blocks_counter;
	unsigned int   groups[n_group*3];
	unsigned int  *prev_grp;
	unsigned int  *group;
	unsigned int  *next_grp;
	unsigned char  offset_chars[4];  // [ABCcDEx] (x=error)
	bool           log;
	bool           debug;
	bool           presync;
	bool           good_block;
	bool           group_assembly_started;
	unsigned char  lastseen_offset;
	unsigned char  block_number;
	enum { NO_SYNC, SYNC, FORCE_SYNC } d_state;
	static const std::array<bit_locator,1024> locator;
	uint16_t       d_prev_pi{0};
	int            d_pi_cnt{0};
	int            d_counter{0};
	std::atomic<int> d_ecc_max{0};
	pi_stats       d_pi_stats[65536*2]{};
	pi_stats       *d_pi_a{&d_pi_stats[0]};
	pi_stats       *d_pi_b{&d_pi_stats[65536]};
	int            d_bit_counter;
	int            d_pi_bitcnt[2]{0};
	char           d_max_weight[2]{0};
    int            d_prev_errs{0};
    int            d_curr_errs{0};
    int            d_next_errs{0};
    int            d_block0errs{0};
    std::map<uint16_t,std::vector<grp_array>> d_matches{};
    unsigned       d_valid_bits{0};
    uint8_t        d_used_list[PS_SEARCH_MAX]{0};
    int            d_best_pi{-1};
    float          d_weight{0.f};
    std::array<float,GROUP_SIZE*2> d_accum{0.f};
    unsigned int   d_acc_groups[4];
    int            d_acc_p{0};
    float          d_acc_alfa{0.1};
    int            d_acc_cnt{0};
    static constexpr int d_acc_lim{104*4};
    unsigned       d_integrate_pi{INTEGRATE_PI_NC_COH};
    unsigned       d_integrate_ps{INTEGRATE_PS_23};
    unsigned       d_integrate_ps_dist{PS_SEARCH_MAX};
    std::array<float, GROUP_SIZE> accum{0};

};

} /* namespace rds */
} /* namespace gr */

#endif /* INCLUDED_RDS_DECODER_IMPL_H */
