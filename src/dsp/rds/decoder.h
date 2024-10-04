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
#ifndef INCLUDED_RDS_DECODER_H
#define INCLUDED_RDS_DECODER_H

#include "dsp/rds/api.h"
#include <gnuradio/sync_block.h>

namespace gr {
namespace rds {

class RDS_API decoder : virtual public gr::sync_block
{
public:
    enum
    {
        INTEGRATE_PI_NC=0,
        INTEGRATE_PI_COH,
        INTEGRATE_PI_NC_COH,
    };
    enum
    {
        INTEGRATE_PS_OFF=0,
        INTEGRATE_PS_2,
        INTEGRATE_PS_3,
        INTEGRATE_PS_23,
        INTEGRATE_PS_23_PLUS,
    };
public:
#if GNURADIO_VERSION < 0x030900
	typedef boost::shared_ptr<decoder> sptr;
#else
	typedef std::shared_ptr<decoder> sptr;
#endif
	static sptr make(bool log=false, bool debug=false);
	virtual void set_ecc_max(int n) = 0;
	virtual void reset() = 0;
	virtual void set_integrate_pi(unsigned) = 0;
	virtual void set_integrate_ps(unsigned) = 0;
	virtual void set_integrate_ps_dist(unsigned) = 0;
};

} // namespace rds
} // namespace gr

#endif /* INCLUDED_RDS_DECODER_H */
