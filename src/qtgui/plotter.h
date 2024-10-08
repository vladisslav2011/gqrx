/* -*- c++ -*- */
#ifndef PLOTTER_H
#define PLOTTER_H

#include <QtGui>
#include <QFont>
#include <QFrame>
#include <QImage>
#include <vector>
#include <set>
#include <mutex>
#include <QMap>
#include "bookmarks.h"
#include "receivers/defines.h"
#include "receivers/vfo.h"

#define HORZ_DIVS_MAX 12    //50
#define VERT_DIVS_MIN 5
#define MAX_SCREENSIZE 16384

#define PEAK_CLICK_MAX_H_DISTANCE 10 //Maximum horizontal distance of clicked point from peak
#define PEAK_CLICK_MAX_V_DISTANCE 20 //Maximum vertical distance of clicked point from peak
#define PEAK_H_TOLERANCE 2


class CPlotter : public QFrame
{
    Q_OBJECT

public:
    explicit CPlotter(QWidget *parent = nullptr);
    ~CPlotter() override;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    //void SetSdrInterface(CSdrInterface* ptr){m_pSdrInterface = ptr;}
    void draw(bool timed=true); //call to draw new fft data onto screen plot
    void setRunningState(bool running) { m_Running = running; }
    void setClickResolution(int clickres) { m_ClickResolution = clickres; }
    void setFilterClickResolution(int clickres) { m_FilterClickResolution = clickres; }
    void setFilterBoxEnabled(bool enabled) { m_FilterBoxEnabled = enabled; }
    void setCenterLineEnabled(bool enabled) { m_CenterLineEnabled = enabled; }
    void setTooltipsEnabled(bool enabled) { m_TooltipsEnabled = enabled; }
    void setBookmarksEnabled(bool enabled) { m_BookmarksEnabled = enabled; }
    void setInvertScrolling(bool enabled) { m_InvertScrolling = enabled; }
    void setBandPlanEnabled(bool enabled) { m_BandPlanEnabled = enabled; }
    void setDXCSpotsEnabled(bool enabled) { m_DXCSpotsEnabled = enabled; }

    void setNewFftData(float *fftData, int size);
    void setNewFftData(float *fftData, float *wfData, int size, qint64 ts);
    void drawOneWaterfallLine(int line, float *fftData, int size, qint64 ts);
    void drawBlackWaterfallLine(int line);
    void scrollWaterfall(int dy);
    void getWaterfallMetrics(int &lines, double &ms_per_line);

    void setCenterFreq(quint64 f);
    void setFreqUnits(qint32 unit) { m_FreqUnits = unit; }

    void setDemodCenterFreq(quint64 f) { m_DemodCenterFreq = f; }

    /*! \brief Move the filter to freq_hz from center. */
    void setFilterOffset(qint64 freq_hz)
    {
        m_DemodCenterFreq = m_CenterFreq + freq_hz;
        drawOverlay();
    }
    qint64 getFilterOffset() const
    {
        return m_DemodCenterFreq - m_CenterFreq;
    }

    int getFilterBw() const
    {
        return m_DemodHiCutFreq - m_DemodLowCutFreq;
    }

    void setHiLowCutFrequencies(int LowCut, int HiCut)
    {
        m_DemodLowCutFreq = LowCut;
        m_DemodHiCutFreq = HiCut;
        drawOverlay();
    }

    void getHiLowCutFrequencies(int *LowCut, int *HiCut) const
    {
        *LowCut = m_DemodLowCutFreq;
        *HiCut = m_DemodHiCutFreq;
    }

    void setDemodRanges(int FLowCmin, int FLowCmax, int FHiCmin, int FHiCmax, bool symetric);

    /* Shown bandwidth around SetCenterFreq() */
    void setSpanFreq(quint32 s)
    {
        if (s > 0 && s < INT_MAX) {
            m_Span = (qint32)s;
            setFftCenterFreq(m_FftCenter);
        }
        if(!m_PlayingIQ || m_Running)
            updateOverlay();
        else
            drawOverlay();
    }

    void setHdivDelta(int delta) { m_HdivDelta = delta; }
    void setVdivDelta(int delta) { m_VdivDelta = delta; }

    void setFreqDigits(int digits) { m_FreqDigits = digits>=0 ? digits : 0; }

    /* Determines full bandwidth. */
    void setSampleRate(float rate)
    {
        if (rate > 0.0f)
        {
            m_SampleFreq = rate;
            drawOverlay();
        }
    }

    float getSampleRate() const
    {
        return m_SampleFreq;
    }

    void setFftCenterFreq(qint64 f) {
        qint64 limit = ((qint64)m_SampleFreq + m_Span) / 2 - 1;
        m_FftCenter = qBound(-limit, f, limit);
    }

    qint64 getFftCenterFreq() const {
        return m_FftCenter;
    }

    void setPlayingIQ(bool state)
    {
        m_PlayingIQ = state;
    }

    int     getNearestPeak(QPoint pt);
    void    setWaterfallSpan(quint64 span_ms);
    quint64 getWfTimeRes() const;
    void    setFftRate(int rate_hz);
    void    clearWaterfall();
    void    setCurrentVfo(int current);
    void    addVfo(vfo::sptr n_vfo);
    void    removeVfo(vfo::sptr n_vfo);
    void    clearVfos();
    void    getLockedVfos(std::vector<vfo::sptr> &to);
    void    blockUpdates(bool);

signals:
    void newDemodFreq(qint64 freq, qint64 delta); /* delta is the offset from the center */
    void newDemodFreqLoad(qint64 freq, qint64 delta);/* tune and load demodulator settings */
    void newDemodFreqAdd(qint64 freq, qint64 delta);/* new demodulator here */
    void newLowCutFreq(int f);
    void newHighCutFreq(int f);
    void newFilterFreq(int low, int high);  /* substitute for NewLow / NewHigh */
    void pandapterRangeChanged(float min, float max);
    void newZoomLevel(float level);
    void newFftCenterFreq(qint64 f);
    void newSize();
    void selectVfo(int);
    void setPlaying(bool);
    void seekIQ(qint64);

public slots:
    // zoom functions
    void resetHorizontalZoom();
    void moveToCenterFreq();
    void moveToDemodFreq();
    void zoomOnXAxis(float level);

    // other FFT slots
    void setFftPlotColor(const QColor& color);
    void setFftFill(bool enabled);
    void setPeakHold(bool enabled);
    void setFftRange(float min, float max);
    void setWfColormap(const QString &cmap);
    void setPandapterRange(float min, float max);
    void setWaterfallRange(float min, float max);
    void setPeakDetection(bool enabled, float c);
    void toggleBandPlan(bool state);
    void updateOverlay();

    void setPercent2DScreen(int percent)
    {
        m_Percent2DScreen = percent;
        m_Size = QSize(0,0);
        resizeEvent(nullptr);
    }
    void setFrequencyRange(qint64 min, qint64 max)
    {
        m_MinFreq = min;
        m_MaxFreq = max;
    }

protected:
    //re-implemented widget event handlers
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent * event) override;
    void mousePressEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;
    void wheelEvent( QWheelEvent * event ) override;

private:
    enum eCapturetype {
        NOCAP,
        LEFT,
        CENTER,
        RIGHT,
        YAXIS,
        XAXIS,
        TAG,
        WATERFALL
    };
    struct wfLineStats
    {
        qint64 ts{0};     // timestamp ms
        qint64 center{0}; // FFT center
        qint64 span{0};   // FFT span
        wfLineStats(qint64 n_ts,qint64 n_center,qint64 n_span)
        {
            ts=n_ts;
            center=n_center;
            span=n_span;
        }
    };

    void        drawOverlay();
    void        drawVfo(QPainter &painter, const int demodFreqX,
                        const int demodLowCutFreqX, const int dw, const int h,
                        const int index, const bool is_selected);
    void        makeFrequencyStrs();
    int         xFromFreq(qint64 freq);
    qint64      freqFromX(int x);
    void        tsFreqFromWfXY(int x, int y, qint64 &ts, qint64 &freq);
    void        zoomStepX(float factor, int x);
    static qint64      roundFreq(qint64 freq, int resolution);
    void        clampDemodParameters();
    static bool        isPointCloseTo(int x, int xr, int delta)
    {
        return ((x > (xr - delta)) && (x < (xr + delta)));
    }
    void getScreenIntegerFFTData(qint32 plotHeight, qint32 plotWidth,
                                 float maxdB, float mindB,
                                 qint64 startFreq, qint64 stopFreq,
                                 float *inBuf, qint32 *outBuf,
                                 qint32 *maxbin, qint32 *minbin);
    static void calcDivSize (qint64 low, qint64 high, int divswanted, qint64 &adjlow, qint64 &step, int& divs);
    void        showToolTip(QMouseEvent* event, QString toolTipText);

    bool        m_PeakHoldActive;
    bool        m_PeakHoldValid;
    qint32      m_fftbuf[MAX_SCREENSIZE]{};
    qint32      m_fftbuf2[MAX_SCREENSIZE]{};
    quint8      m_wfbuf[MAX_SCREENSIZE]{}; // used for accumulating waterfall data at high time spans
    qint32      m_fftPeakHoldBuf[MAX_SCREENSIZE]{};
    float      *m_fftData{};     /*! pointer to incoming FFT data */
    float      *m_wfData{};
    QList<wfLineStats> m_wfLineStats;
    int         m_fftDataSize{};

    int         m_XAxisYCenter{};
    int         m_YAxisWidth{};

    eCapturetype    m_CursorCaptured;
    QPixmap     m_2DPixmap;
    QPixmap     m_OverlayPixmap;
    QPixmap     m_WaterfallPixmap;
    QImage      m_WaterfallLine;
    QVector<QRgb> m_ColorTbl;
    QSize       m_Size;
    qreal       m_DPR{};
    QString     m_HDivText[HORZ_DIVS_MAX+1];
    bool        m_Running;
    bool        m_DrawOverlay;
    qint64      m_CenterFreq;       // The HW frequency
    qint64      m_FftCenter;        // Center freq in the -span ... +span range
    qint64      m_DemodCenterFreq;
    qint64      m_StartFreqAdj{};
    qint64      m_FreqPerDiv{};
    bool        m_CenterLineEnabled;  /*!< Distinguish center line. */
    bool        m_FilterBoxEnabled;   /*!< Draw filter box. */
    bool        m_TooltipsEnabled{};  /*!< Tooltips enabled */
    bool        m_BandPlanEnabled;    /*!< Show/hide band plan on spectrum */
    bool        m_BookmarksEnabled;   /*!< Show/hide bookmarks on spectrum */
    bool        m_InvertScrolling;
    bool        m_DXCSpotsEnabled;    /*!< Show/hide DXC Spots on spectrum */
    int         m_DemodHiCutFreq;
    int         m_DemodLowCutFreq;
    int         m_DemodFreqX{};       //screen coordinate x position
    int         m_DemodHiCutFreqX{};  //screen coordinate x position
    int         m_DemodLowCutFreqX{}; //screen coordinate x position
    int         m_CursorCaptureDelta;
    int         m_GrabPosition;
    int         m_Percent2DScreen;
    qint64      m_MinFreq;
    qint64      m_MaxFreq;

    int         m_FLowCmin;
    int         m_FLowCmax;
    int         m_FHiCmin;
    int         m_FHiCmax;
    bool        m_symetric;

    int         m_HorDivs;   /*!< Current number of horizontal divisions. Calculated from width. */
    int         m_VerDivs;   /*!< Current number of vertical divisions. Calculated from height. */

    float       m_PandMindB;
    float       m_PandMaxdB;
    float       m_WfMindB;
    float       m_WfMaxdB;

    qint64      m_Span;
    float       m_SampleFreq;    /*!< Sample rate. */
    qint32      m_FreqUnits;
    int         m_ClickResolution;
    int         m_FilterClickResolution;
    qint32      m_CumWheelDelta;

    int         m_Xzero{};
    int         m_Yzero{};  /*!< Used to measure mouse drag direction. */
    int         m_FreqDigits;  /*!< Number of decimal digits in frequency strings. */

    QFont       m_Font;      /*!< Font used for plotter (system font) */
    int         m_HdivDelta; /*!< Minimum distance in pixels between two horizontal grid lines (vertical division). */
    int         m_VdivDelta; /*!< Minimum distance in pixels between two vertical grid lines (horizontal division). */
    int         m_BandPlanHeight; /*!< Height in pixels of band plan (if enabled) */

    quint32     m_LastSampleRate{};

    QColor      m_FftColor, m_FftFillCol, m_PeakHoldColor;
    bool        m_FftFill{};

    float       m_PeakDetection{};
    QMap<int,int>   m_Peaks;

    QList< QPair<QRect, qint64> >     m_Taglist;
    vfo::set    m_vfos;
    vfo::set::iterator m_vfos_ub;
    vfo::set::iterator m_vfos_lb;
    vfo::sptr   m_lookup_vfo;
    int         m_currentVfo;
    int         m_capturedVfo;
    bool        m_PlayingIQ;
    qint64      m_CapturedTs;

    // Waterfall averaging
    quint64     tlast_wf_ms;        // last time waterfall has been updated
    quint64     tnow_wf_ms;        // time when new waterfall data has arrived
    quint64     msec_per_wfline;    // milliseconds between waterfall updates
    quint64     wf_span;            // waterfall span in milliseconds (0 = auto)
    int         fft_rate;           // expected FFT rate (needed when WF span is auto)
    std::mutex  m_wf_mutex;         // waterfall update mutex
    std::vector<qint32> m_pTranslateTbl;
    qint32 old_plotWidth{0};
    qint32 old_minbin{0}, old_maxbin{0};
    int    old_largeFft{0};
    qint64 old_startFreq{-1};
    qint64 old_stopFreq{-1};
    qint32 old_FFTSize{-1};
    float  old_SampleFreq{-1.0};
    bool   blockedUpdates{false};

};

#endif // PLOTTER_H
