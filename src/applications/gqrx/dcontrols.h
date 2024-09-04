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
#include "tagged_union.h"
#include "receivers/defines.h"
#include "receivers/modulations.h"

enum c_id
{
    C_INVALID=-1,
    C_TEST=0,
    
    C_IQ_LOCATION,
    C_IQ_SELECT,
    C_IQ_REC,
    C_IQ_PLAY,
    C_IQ_REPEAT,
    C_IQ_TIME,
    C_IQ_POS,
    C_IQ_FORMAT,
    C_IQ_BUFFERS,
    C_IQ_SIZE_STAT,
    C_IQ_BUF_STAT,
    C_IQ_FINE_STEP,
    C_IQ_SEL_A,
    C_IQ_SEL_B,
    C_IQ_RESET_SEL,
    C_IQ_SAVE_SEL,
    C_IQ_GOTO_A,
    C_IQ_GOTO_B,
    C_IQ_SAVE_LOC,
    C_IQ_PROCESS,

    C_DXC_ADDRESS,
    C_DXC_PORT,
    C_DXC_TIMEOUT,
    C_DXC_FILTER,
    C_DXC_USERNAME,
    C_DXC_DISCONNECT,
    C_DXC_CONNECT,

    C_FFT_SIZE,
    C_FFT_RBW_LABEL,
    C_FFT_RATE,
    C_FFT_OVERLAP_LABEL,
    C_FFT_TIMESPAN,
    C_FFT_TIMESPAN_LABEL,
    C_FFT_WINDOW,
    C_FFT_WINDOW_CORR,
    C_FFT_AVG,
    C_FFT_AVG_LABEL,
    C_FFT_SPLIT,
    C_FFT_PEAK_DETECT,
    C_FFT_PEAK_HOLD,
    C_FFT_PAND_MIN_DB,
    C_FFT_PAND_MAX_DB,
    C_FFT_RANGE_LOCKED,
    C_FFT_WF_MIN_DB,
    C_FFT_WF_MAX_DB,
    C_PLOT_ZOOM,
    C_PLOT_ZOOM_LABEL,
    C_PLOT_RESET,
    C_PLOT_CENTER,
    C_PLOT_DEMOD,
    C_PLOT_COLOR,
    C_PLOT_FILL,
    C_WF_COLORMAP,
    C_WF_BG_THREADS,
    C_ENABLE_BANDPLAN,
    C_TUNING_STEP,

    C_LNB_LO,
    C_IQ_AGC,
    C_IQ_AGC_ACK,
    C_IQ_SWAP,
    C_IGNORE_LIMITS,
    C_IQ_DCR,
    C_IQ_BALANCE,
    C_PPM,
    C_ANTENNA,
    C_INPUTCTL_LINE,
    C_DIGITS_RESET,
    C_WHEEL_INVERT,
    C_AUTO_BOOKMARKS,
    C_CHAN_THREADS,

//    C_AUTOSTART,
    C_HW_FREQ_LABEL,
    C_VFO_FREQUENCY,
    C_FREQ_LOCK,
    C_FREQ_LOCK_ALL,
    C_FREQ_UNLOCK_ALL,
    C_MODE,
    C_MODE_CHANGED,
    C_MODE_OPT,
    C_FILTER_WIDTH,
    C_FILTER_LO,
    C_FILTER_HI,
    C_FILTER_SHAPE,
    C_AGC_PRESET,
    C_AGC_OPT,
    C_SQUELCH_LEVEL,
    C_SQUELCH_AUTO,
    C_SQUELCH_RESET,
    C_SQUELCH_AUTO_GLOBAL,
    C_SQUELCH_RESET_GLOBAL,
    C_NB3_ON,
    C_NB2_ON,
    C_NB1_ON,
    C_NB_OPT,

    C_AGC_MAN_GAIN_UP,
    C_AGC_MAN_GAIN_DOWN,
    C_AGC_MAN_GAIN,
    C_AGC_MAN_GAIN_LABEL,
    C_AGC_MUTE,
    C_GLOBAL_MUTE,
    C_AUDIO_UDP_STREAMING,
    C_AUDIO_REC,
    C_AUDIO_PLAY,
    C_AUDIO_OPTIONS,
    C_AUDIO_REC_FILENAME,

    C_AUDIO_FFT_SPLIT,
    C_AUDIO_FFT_PAND_MIN_DB,
    C_AUDIO_FFT_PAND_MAX_DB,
    C_AUDIO_FFT_WF_MIN_DB,
    C_AUDIO_FFT_WF_MAX_DB,
    C_AUDIO_FFT_RANGE_LOCKED,
    C_AUDIO_FFT_ZOOM,
    C_AUDIO_FFT_CENTER,
    C_AUDIO_REC_DIR,
    C_AUDIO_REC_DIR_BTN,
    C_AUDIO_REC_SQUELCH_TRIGGERED,
    C_AUDIO_REC_MIN_TIME,
    C_AUDIO_REC_MAX_GAP,
    C_AUDIO_REC_COPY,
    C_AUDIO_REC_FORMAT,
    C_AUDIO_REC_COMPRESSION,
    C_AUDIO_UDP_HOST,
    C_AUDIO_UDP_PORT,
    C_AUDIO_UDP_STEREO,
    C_AUDIO_DEDICATED_DEV,
    C_AUDIO_DEDICATED_ON,
    C_AUDIO_DEDICATED_ERROR,

    C_AGC_MAX_GAIN,
    C_AGC_MAX_GAIN_LABEL,
    C_AGC_TARGET,
    C_AGC_TARGET_LABEL,
    C_AGC_ATTACK,
    C_AGC_ATTACK_LABEL,
    C_AGC_DECAY,
    C_AGC_DECAY_LABEL,
    C_AGC_HANG,
    C_AGC_HANG_LABEL,
    C_AGC_PANNING,
    C_AGC_PANNING_LABEL,
    C_AGC_PANNING_AUTO,
    C_AGC_ON,

    C_NB1_THR,
    C_NB2_THR,
    C_NB3_GAIN,

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
    C_RAWIQ_RATE,
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
    D_DXC,
    D_PROBE,
    
    D_COUNT
};

enum gui_window
{
    W_BASE=0,
    W_CHILD, //"Options" extra window
    W_DEMOD_OPT,
    W_NB_OPT,
    W_AGC_OPT,
    
    W_COUNT
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
    constexpr inline const T & N () const {return d_##N;}\
    constexpr inline c_def & N (const T & n_##N){d_##N=n_##N; return *this;}
#define DS(N,I) \
    const char * d_##N{I};\
    constexpr inline const char * N () const {return d_##N;}\
    constexpr inline c_def & N (const char * n_##N){d_##N=n_##N; return *this;}

struct c_def;

struct c_def
{
    using v_union = ::tag_union;
    struct v_preset
    {
        const char * key;
        const char * display;//FIXME: implement translation
        tag_union_const value;
        const char * shortcut;
        constexpr v_preset()
            :key(nullptr),display(nullptr),value(0),shortcut(nullptr)
            {}
        constexpr v_preset(const char * n_key, const char * n_display, const tag_union_const &n_value, const char * n_shortcut=nullptr)
            :key(n_key),display(n_display),value(n_value),shortcut(n_shortcut)
            {}
    };
    struct preset_list
    {
        typedef const v_preset * iterator;
        std::size_t n_items{0};
        const v_preset * items{nullptr};
        constexpr std::size_t size() const {return n_items;}
        constexpr iterator begin() const {return items;}
        constexpr iterator end() const {return &items[n_items];}
        template<std::size_t N>
        constexpr preset_list & from(const v_preset (&init) [N])
        {
            n_items = N;
            items = init;
            return *this;
        }
        template<std::size_t N>
        constexpr preset_list & from(v_preset (&init) [N])
        {
            n_items = N;
            items = init;
            return *this;
        }
        constexpr const v_preset & operator [] (const std::size_t what)
        {
            return items[what];
        }
        iterator find(const std::string & what) const;
        iterator find(const v_union & what) const;
        template<std::size_t N>
        constexpr static preset_list make(v_preset (&init) [N])
        {
            return preset_list{N, init};
        };

        template<std::size_t N>
        constexpr static preset_list make(const v_preset (&init) [N])
        {
            return preset_list{N, init};
        };
    };
    struct grid_placement
    {
        constexpr grid_placement(int n_row=0,int n_column=0, int n_rowspan=1,
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
    DS(name,"Unnamed")
    DS(title,"Default title") //FIXME: implement translation
    DD(grid_placement,title_placement,grid_placement(PLACE_NEXT,0))
    DS(next_title,"Default next title") //FIXME: implement translation
    DD(grid_placement,next_placement,grid_placement(PLACE_NONE,PLACE_NONE))
    DD(grid_placement,placement,grid_placement(PLACE_SAME,1))
    DS(hint,"Default tooltip") //FIXME: implement translation
    DS(icon,nullptr)
    DS(shortcut,nullptr)
    DS(tab,nullptr) //FIXME: implement translation
    DD(gui_type,g_type,G_TEXT)
    DD(gui_dock,dock,D_INPUTCTL)
    DD(gui_window,window,W_BASE)
    DD(value_scope,scope,S_RX)
    DD(bool,demod_specific,false) //should be hidden if current demod does not match demod member
    DD(Modulations::grp,demodgroup,Modulations::GRP_COUNT)
    DS(v3_config_group,"V3_DEFAULT")
    DS(v4_config_group,"V4_DEFAULT")
    DS(config_key,nullptr)
    DD(int,bookmarks_column,-1)
    DS(bookmarks_key,nullptr)
    DD(value_type, v_type,V_INT)
    DS(suffix,"")
    DS(prefix,"")
    DD(tag_union_const,def,0)
    DD(tag_union_const,min,0)
    DD(tag_union_const,max,0)
    DD(tag_union_const,step,0) //for slider or double spinbox
    DD(int,frac_digits,0)//for double spinbox
    DD(bool,readable,true)
    DD(bool,writable,true)
    DD(bool,event,false)
    //using preset_list=std::vector<v_preset>;
    //DD(preset_map,presets,{})
    preset_list d_presets{};
    constexpr const preset_list& presets() const {return d_presets;}
    template<std::size_t N> constexpr c_def & presets(v_preset (&init) [N])
    {
        d_presets = preset_list::make<N>(init);
        return *this;
    }
    template<std::size_t N> constexpr c_def & presets(const v_preset (&init) [N])
    {
        d_presets = preset_list::make<N>(init);
        return *this;
    }
public:

    bool clip(v_union & v) const;
    static const std::array<c_def,C_COUNT> & all();
};

#undef DD
#undef DS

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