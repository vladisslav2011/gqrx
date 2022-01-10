//////////////////////////////////////////////////////////////////////
// agc_impl.h: interface for the CAgc class.
//
//  This class implements an automatic gain function.
//
// History:
//  2010-09-15  Initial creation MSW
//  2011-03-27  Initial release
//  2011-09-24  Adapted for gqrx
//////////////////////////////////////////////////////////////////////
#ifndef AGC_IMPL_H
#define AGC_IMPL_H

#include <complex>
#include <vector>

#define MAX_DELAY_BUF 2048

/*
typedef struct _dCplx
{
    double re;
    double im;
} tDComplex;

#define TYPECPX tDComplex
*/

#define TYPECPX std::complex<float>

class CAgc
{
public:
    CAgc();
    virtual ~CAgc();
    void SetParameters(double sample_rate, bool agc_on, int target_level,
                       int manual_gain, int max_gain, int attack, int decay,
                       int hang, bool force = false);
    void ProcessData(float * pOutData, const float * pInData, int Length);
    void ProcessData(TYPECPX * pOutData, const TYPECPX * pInData, int Length);

private:
    float get_peak();
    void update_buffer(int p);
    
    float d_sample_rate;
    bool d_agc_on;
    int d_target_level;
    int d_manual_gain;
    int d_max_gain;
    int d_attack;
    int d_decay;
    int d_hang;
    
    float d_target_mag;
    int d_hang_samp;
    int d_buf_samples;
    int d_buf_size;
    int d_max_idx;
    int d_buf_p;
    int d_hang_counter;
    float d_current_gain;
    float d_target_gain;
    float d_decay_step;
    float d_attack_step;
    float d_floor;

    std::vector<TYPECPX> d_sample_buf;
    std::vector<float>   d_mag_buf;
    
    float d_prev_dbg;
};

# if 0
class CAgc
{
public:
    CAgc();
    virtual ~CAgc();
    void SetParameters(bool AgcOn, bool UseHang, int Threshold, int ManualGain, int Slope, int Decay, double SampleRate);
    void ProcessData(int Length, const TYPECPX * pInData, TYPECPX * pOutData);
    void ProcessData(int Length, const float * pInData, float * pOutData);

private:
    bool        m_AgcOn;
    bool        m_UseHang;
    int         m_Threshold;
    int         m_ManualGain;
    int         m_Decay;

    float       m_SampleRate;

    float       m_SlopeFactor;
    float       m_ManualAgcGain;

    float       m_DecayAve;
    float       m_AttackAve;

    float       m_AttackRiseAlpha;
    float       m_AttackFallAlpha;
    float       m_DecayRiseAlpha;
    float       m_DecayFallAlpha;

    float       m_FixedGain;
    float       m_Knee;
    float       m_GainSlope;
    float       m_Peak;

    int         m_SigDelayPtr;
    int         m_MagBufPos;
    int         m_DelaySamples;
    int         m_WindowSamples;
    int         m_HangTime;
    int         m_HangTimer;

    TYPECPX     m_SigDelayBuf[MAX_DELAY_BUF];
    float*      m_SigDelayBuf_r;

    float       m_MagBuf[MAX_DELAY_BUF];
};
#endif

#endif //  AGC_IMPL_H
