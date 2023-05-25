/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2023 vladisslav2011@gmail.com.
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
#ifndef INCLUDED_DCONTROLS_H
#define INCLUDED_DCONTROLS_H
#include <functional>
#include <array>
#include <string>
#include <map>
#include <vector>
#include "receivers/defines.h"
#include "receivers/modulations.h"

enum c_id
{
    C_INVALID=-1,
    C_TEST=0,
    
    C_NFM_MAXDEV,
    C_NFMPLL_MAXDEV,
    C_NFM_DEEMPH,
    C_NFM_SUBTONE_FILTER,
    C_NFMPLL_DAMPING_FACTOR,
    C_NFMPLL_PLLBW,
    C_NFMPLL_SUBTONE_FILTER,
    C_AMSYNC_PLLBW,
    C_AMSYNC_DCR,
    C_AM_DCR,
    C_CW_OFFSET,
    C_DEMOD_OFF_DUMMY,
    C_RAWIQ_DUMMY,
    C_SSB_DUMMY,
    C_WFM_DEEMPH,
    C_WFM_STEREO_DEEMPH,
    C_WFM_OIRT_DEEMPH,
    C_RDS_ON,
    /* type 0 = PI
     * type 1 = PS
     * type 2 = PTY
     * type 3 = flagstring: TP, TA, MuSp, MoSt, AH, CMP, stPTY
     * type 4 = RadioText
     * type 5 = ClockTime
     * type 6 = Alternative Frequencies
     */
    C_RDS_PS,
    C_RDS_PTY,
    C_RDS_PI,
    C_RDS_RADIOTEXT,
    C_RDS_CLOCKTIME,
    C_RDS_ALTFREQ,
    C_RDS_FLAGS,
    
    C_COUNT
};

enum gui_type
{
    G_NONE=0,
    G_TEXT,
    G_SPINBOX,
    G_DOUBLESPINBOX,
    G_CHECKBOX,
    G_COMBO,
    G_STRINGCOMBO,
    G_SLIDER,
    G_LOGSLIDER, // log-scaled slider
    G_RANGESLIDER,// declare without base to get minimum value and with base to get maximum value and construct the widget
    G_MENUACTION,
    G_MENUCHECKBOX,
    G_BUTTON,
    G_TOGGLEBUTTON,
    G_LABEL,
    G_LINE,
    G_COLORPICKER,
    
    G_GUI_TYPE_COUNT
};

enum gui_dock
{
    D_INPUTCTL=0,
    D_RXOPT,
    D_AUDIO,
    D_FFT,
    D_RDS,
    D_IQTOOL,
    D_BOOKMARKS,
    D_PROBE,
    
    D_COUNT
};

enum gui_window
{
    W_BASE=0,
    W_CHILD, //"Options" extra window
    W_DEMOD_OPT,
    
    W_COUNT
};

enum value_type
{
    V_INT=0,
    V_DOUBLE,
    V_STRING,
    V_BOOLEAN
};

enum convenient_placement
{
    PLACE_NONE=-3,
    PLACE_SAME=-2,
    PLACE_NEXT=-1,
};

enum value_scope
{
    S_GUI=0,
    S_RX,
    S_VFO,
};

enum alignments
{
    ALIGN_DEFAULT=0,
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT,
    ALIGN_MASK=0x0f,
    SIZE_MINIMUM=0x10,
    SIZE_MAXIMUM=0x20,
    SIZE_PREFERRED=0x30,
    SIZE_FIXED=0x40,
    SIZE_MINIMUMEXPANDING=0x50,
    SIZE_EXPANDING=0x60,
    SIZE_IGNORED=0x70,
    SIZE_MASK=0xf0,
};

// Macros are ugly, but convenient
#define DD(T,N,I) \
    T d_##N{I};\
    inline const T & N () const {return d_##N;}\
    inline c_def & N (const T & n_##N){d_##N=n_##N; return *this;}

struct c_def;

struct c_def
{
    class v_union
    {
        enum tag_type
        {
            TAG_INT=0,
            TAG_DOUBLE,
            TAG_STRING
        };
        tag_type tag;
        union union_type {
            int64_t i;
            double d;
            std::string s;
            union_type():i(0){}
            ~union_type(){}
        } value;
        public:
        v_union():tag(TAG_INT)
        {
            value.i=0;
        }
        v_union(const c_def::v_union& other):tag(other.tag)
        {
            switch(tag)
            {
            case TAG_STRING:
                new (&value.s) std::string(other.value.s);
                break;
            case TAG_DOUBLE:
                value.d=other.value.d;
                break;
            case TAG_INT:
                value.i=other.value.i;
                break;
            }
        }
        v_union(const char *init):tag(TAG_STRING)
        {
            new (&value.s) std::string(init);
        }
        v_union(const std::string &init):tag(TAG_STRING)
        {
            new (&value.s) std::string(init);
        }
        v_union(const float &init):tag(TAG_DOUBLE)
        {
            value.d = (double)init;
        }
        v_union(const double &init):tag(TAG_DOUBLE)
        {
            value.d = init;
        }
        v_union(const int &init):tag(TAG_INT)
        {
            value.i = init;
        }
        v_union(const long int &init):tag(TAG_INT)
        {
            value.i = init;
        }
        v_union(const long long int &init):tag(TAG_INT)
        {
            value.i = init;
        }
        v_union(const bool &init):tag(TAG_INT)
        {
            value.i = init;
        }
        v_union(const value_type vt, const std::string & from)
        {
            switch(vt)
            {
            case V_INT:
                    value.i=stoll(from);
                    tag=TAG_INT;
                break;
            case V_DOUBLE:
                    value.d=stod(from);
                    tag=TAG_DOUBLE;
                break;
            case V_STRING:
                    new (&value.s) std::string(from);
                    tag=TAG_STRING;
                break;
            case V_BOOLEAN:
                    value.i=(from=="true");
                    tag=TAG_INT;
                break;
            }
        }
        ~v_union()
        {
            if(tag==TAG_STRING)
                value.s.~basic_string();
        }
        void typecheck(const tag_type required, const char * msg) const
        {
            if(tag!=required)
            {
                dprint(msg);
                //throw does not work as expected here
                //emit a message and exit
                exit(1);
            }
        }
        operator int() const
        {
            typecheck(TAG_INT, "invalid cast to int");
            return value.i;
        }
        operator long int() const
        {
            typecheck(TAG_INT, "invalid cast to int");
            return value.i;
        }
        operator long long int() const
        {
            typecheck(TAG_INT, "invalid cast to int");
            return value.i;
        }
        operator bool() const
        {
            typecheck(TAG_INT, "invalid cast to bool");
            return value.i;
        }
        operator double() const
        {
            typecheck(TAG_DOUBLE, "invalid cast to double");
            return value.d;
        }
        operator float() const
        {
            typecheck(TAG_DOUBLE, "invalid cast to float");
            return value.d;
        }
        operator std::string() const
        {
            typecheck(TAG_STRING, "invalid cast to string");
            return value.s;
        }
        const std::string to_string() const
        {
            switch(tag)
            {
            case TAG_INT:
                return std::to_string(value.i);
            case TAG_DOUBLE:
                return std::to_string(value.d);
            case TAG_STRING:
                return value.s;
            default:
                return "";
            }
        }
        void dprint(const char * x="") const
        {
            switch(tag)
            {
            case TAG_INT:
                std::cerr<<"v_union:i="<<value.i<<" "<<x<<"\n";
                break;
            case TAG_DOUBLE:
                std::cerr<<"v_union:d="<<value.d<<" "<<x<<"\n";
                break;
            case TAG_STRING:
                std::cerr<<"v_union:s="<<value.s<<" "<<x<<"\n";
                break;
            }
        }
        bool typecheck_cmp(const v_union & other) const
        {
            if(tag!=other.tag)
            {
                dprint("invalid comparison");
                other.dprint("invalid comparison");
                return false;
            }
            return true;
        }
        bool operator < (const v_union & other) const
        {
            if(!typecheck_cmp(other))
                return false;
            switch(tag)
            {
            case TAG_INT:
                return value.i<other.value.i;
                break;
            case TAG_DOUBLE:
                return value.d<other.value.d;
                break;
            case TAG_STRING:
                return value.s<other.value.s;
                break;
            }
            return false;
        }
        bool operator > (const v_union & other) const
        {
            if(!typecheck_cmp(other))
                return false;
            switch(tag)
            {
            case TAG_INT:
                return value.i>other.value.i;
                break;
            case TAG_DOUBLE:
                return value.d>other.value.d;
                break;
            case TAG_STRING:
                return value.s>other.value.s;
                break;
            }
            return false;
        }
        bool operator == (const v_union & other) const
        {
            if(!typecheck_cmp(other))
                return false;
            switch(tag)
            {
            case TAG_INT:
                return value.i==other.value.i;
                break;
            case TAG_DOUBLE:
                return value.d==other.value.d;
                break;
            case TAG_STRING:
                return value.s==other.value.s;
                break;
            }
            return false;
        }
        v_union& operator=(const v_union& other)
        {
            if(tag == TAG_STRING)
            {
                if(other.tag != TAG_STRING)
                    value.s.~basic_string();
                else
                    value.s=other.value.s;
            }
            if(tag != TAG_STRING && other.tag == TAG_STRING)
                new (&value.s) std::string(other.value.s);
            else{
                switch(other.tag)
                {
                case TAG_INT:
                    value.i=other.value.i;
                    break;
                case TAG_DOUBLE:
                    value.d=other.value.d;
                    break;
                case TAG_STRING:
                    break;
                }
            }
            tag=other.tag;
            return *this;
        }
    };
    struct v_preset
    {
        std::string key;
        std::string display;//FIXME: implement translation
        v_union value;
        std::string shortcut;
        v_preset(const std::string &n_key, const std::string &n_display, const v_union &n_value, const std::string & n_shortcut="")
            :key(n_key),display(n_display),value(n_value),shortcut(n_shortcut)
            {}
    };
    struct grid_placement
    {
        grid_placement(int n_row=0,int n_column=0, int n_rowspan=1,
            int n_colspan=1,int alignment=ALIGN_DEFAULT)
            :column(n_column),row(n_row),colspan(n_colspan),rowspan(n_rowspan)
            ,align(alignments(alignment))
        {
        }
        int column;
        int row;
        int colspan;
        int rowspan;
        alignments align;
    };
    DD(c_id,idx,c_id(-1))
    DD(c_id,base,c_id(-1))
    DD(std::string,name,"Unnamed")
    DD(std::string,title,"Default title") //FIXME: implement translation
    DD(grid_placement,title_placement,grid_placement(PLACE_NEXT,0))
    DD(std::string,next_title,"Default next title") //FIXME: implement translation
    DD(grid_placement,next_placement,grid_placement(PLACE_NONE,PLACE_NONE))
    DD(grid_placement,placement,grid_placement(PLACE_SAME,1))
    DD(std::string,hint,"Default tooltip") //FIXME: implement translation
    DD(std::string,icon,"")
    DD(std::string,shortcut,"")
    DD(std::string,tab,"") //FIXME: implement translation
    DD(gui_type,g_type,G_TEXT)
    DD(gui_dock,dock,D_INPUTCTL)
    DD(gui_window,window,W_BASE)
    DD(value_scope,scope,S_RX)
    DD(bool,demod_specific,false) //should be hidden if current demod does not match demod member
    DD(Modulations::grp,demodgroup,Modulations::GRP_OFF)
    DD(std::string,v3_config_group,"V3_DEFAULT")
    DD(std::string,v4_config_group,"V4_DEFAULT")
    DD(std::string,config_key,"Unnamed")
    DD(int,bookmarks_column,-1)
    DD(std::string,bookmarks_key,"-1")
    DD(value_type, v_type,V_INT)
    DD(std::string,suffix,"")
    DD(std::string,prefix,"")
    DD(v_union,def,0)
    DD(v_union,min,0)
    DD(v_union,max,0)
    DD(v_union,step,0) //for slider or double spinbox
    DD(int,frac_digits,0)//for double spinbox
    DD(bool,readable,true)
    DD(bool,writable,true)
    DD(bool,event,false)
    using preset_list=std::vector<v_preset>;
    //DD(preset_map,presets,{})
    preset_list d_presets{};
    std::map<std::string, int> d_kpresets{};
    std::map<v_union, int> d_ipresets{};
    inline const preset_list& presets() const {return d_presets;}
    inline const std::map<std::string, int> & kpresets() const {return d_kpresets;}
    inline const std::map<v_union, int> & ipresets() const {return d_ipresets;}
    inline c_def & presets(const preset_list & n_presets)
    {
        d_presets = n_presets;
        std::map<std::string,int> n_kpresets{};
        std::map<v_union,int> n_ipresets{};
        for(unsigned k=0; k<n_presets.size(); k++)
        {
            n_kpresets[n_presets[k].key]=k;
            n_ipresets[n_presets[k].value]=k;
        }
        d_ipresets=n_ipresets;
        d_kpresets=n_kpresets;
        return *this;
    }
public:

    bool clip(v_union & v) const;
    static const std::array<c_def,C_COUNT> & all();
};

#undef DD

struct conf_base
{
    virtual bool set_value(c_id, const c_def::v_union &) = 0;
    virtual bool get_value(c_id, c_def::v_union &) const = 0;
    bool register_observer(c_id, std::function<void (const int, const c_def::v_union &)>);
    const std::function<void (const int, const c_def::v_union &)> & observer(c_id id) const;
    void changed_value(c_id, const int, const c_def::v_union &) const;
    private:
    static std::array<std::function<void (const int, const c_def::v_union &)>, C_COUNT> observers;
};

struct conf_notifier: public conf_base
{
    bool set_value(c_id optid, const c_def::v_union & value) override
    {
        return false;
    }
    bool get_value(c_id optid, c_def::v_union & value) const override
    {
        return false;
    }
};

template <typename T> struct conf_dispatchers: public conf_base
{
    static std::array<bool (T::*)(const c_def::v_union &), C_COUNT> setters;
    static std::array<bool (T::*)(c_def::v_union &) const, C_COUNT> getters;
    static int conf_dispatchers_init_dummy;
    bool set_value(c_id optid, const c_def::v_union & value) override
    {
        if(setters[optid])
            return (dynamic_cast<T*>(this)->*setters[optid])(value);
        return false;
    }

    bool get_value(c_id optid, c_def::v_union & value) const override
    {
        if(getters[optid])
            return (dynamic_cast<const T*>(this)->*getters[optid])(value);
        return false;
    }

};

template<typename T> std::array<bool (T::*)(const c_def::v_union &), C_COUNT> conf_dispatchers<T>::setters;
template<typename T> std::array<bool (T::*)(c_def::v_union &) const, C_COUNT> conf_dispatchers<T>::getters;

#endif //INCLUDED_DCONTROLS_H