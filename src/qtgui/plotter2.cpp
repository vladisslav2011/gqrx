/* -*- c++ -*- */
/* + + +   This Software is released under the "Simplified BSD License"  + + +
 * Copyright 2010 Moe Wheatley. All rights reserved.
 * Copyright 2011-2013 Alexandru Csete OZ9AEC
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY Moe Wheatley ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Moe Wheatley OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Moe Wheatley.
 */
#include <cmath>
#include <QGuiApplication>
#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QFont>
#include <QPainter>
#include <QtGlobal>
#include <QToolTip>
#include "plotter2.h"

Q_LOGGING_CATEGORY(plotter, "plotter2")

#define CUR_CUT_DELTA 5		//cursor capture delta in pixels

// Colors of type QRgb in 0xAARRGGBB format (unsigned int)
#define PLOTTER_BGD_COLOR           0xFF1F1D1D
#define PLOTTER_GRID_COLOR          0xFF444242
#define PLOTTER_TEXT_COLOR          0xFFDADADA
#define PLOTTER_CENTER_LINE_COLOR   0xFF788296
#define PLOTTER_FILTER_LINE_COLOR   0xFFFF7171
#define PLOTTER_FILTER_BOX_COLOR    0xFFA0A0A4
// FIXME: Should cache the QColors also

#define HOR_MARGIN 5
#define VER_MARGIN 5

int F2B(float f)
{
    int b = (f >= 1.0 ? 255 : (f <= 0.0 ? 0 : (int)floor(f * 256.0)));
    return b;
}

static inline bool val_is_out_of_range(float val, float min, float max)
{
    return (val < min || val > max);
}

static inline bool out_of_range(float min, float max)
{
    return (val_is_out_of_range(min, FFT_MIN_DB, FFT_MAX_DB) ||
            val_is_out_of_range(max, FFT_MIN_DB, FFT_MAX_DB) ||
            max < min + 10.f);
}

#define STATUS_TIP \
    "Click, drag or scroll on spectrum to tune. " \
    "Drag and scroll X and Y axes for pan and zoom. " \
    "Drag filter edges to adjust filter."

CPlotter2::CPlotter2(QWidget *parent) : QFrame(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_PaintOnScreen,false);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setMouseTracking(true);

    setTooltipsEnabled(false);
//    setStatusTip(tr(STATUS_TIP));
    m_2DPixmap = QPixmap(0,0);
    m_OverlayPixmap = QPixmap(0,0);
    m_Size = QSize(0,0);
}

CPlotter2::~CPlotter2()
= default;

QSize CPlotter2::minimumSizeHint() const
{
    return {50, 50};
}

QSize CPlotter2::sizeHint() const
{
    return {180, 180};
}

void CPlotter2::mouseMoveEvent(QMouseEvent* event)
{

    QPoint pt = event->pos();

    if (event->buttons() == Qt::NoButton)
    {
                if (m_TooltipsEnabled)
                    showToolTip(event, QString("Current demod %1: %2 kHz")
                                               .arg(m_currentVfo)
                                               .arg(m_DemodCenterFreq/1.e3, 0, 'f', 3));
    }
    setCursor(QCursor(Qt::ArrowCursor));
}





// Called when a mouse button is pressed
void CPlotter2::mousePressEvent(QMouseEvent * event)
{
    QPoint pt = event->pos();
    Qt::KeyboardModifiers key = QGuiApplication::keyboardModifiers();
    bool panadapterClicked = pt.y() < m_OverlayPixmap.height() / m_DPR;

}

void CPlotter2::mouseReleaseEvent(QMouseEvent * event)
{
    QPoint pt = event->pos();

}


// Called when a mouse wheel is turned
void CPlotter2::wheelEvent(QWheelEvent * event)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QPointF pt = QPointF(event->pos());
#else
    QPointF pt = event->position();
#endif
    int delta = m_InvertScrolling? -event->angleDelta().y() : event->angleDelta().y();
    int numDegrees = delta / 8;
    int numSteps = numDegrees / 15;  /** FIXME: Only used for direction **/

    updateOverlay();
}

// Called when screen size changes so must recalculate bitmaps
void CPlotter2::resizeEvent(QResizeEvent* )
{
    if (!size().isValid())
        return;

    if (m_Size != size())
    {
        // if changed, resize pixmaps to new screensize
        m_Size = size();
        m_DPR = devicePixelRatio();
        fft_plot_height = m_Percent2DScreen * m_Size.height() / 100;
        {
            m_OverlayPixmap = QPixmap(m_Size.width() * m_DPR, m_Size.height() * m_DPR);
            m_OverlayPixmap.setDevicePixelRatio(m_DPR);
            m_OverlayPixmap.fill(Qt::black);
            m_2DPixmap = QPixmap(m_Size.width() * m_DPR, m_Size.height() * m_DPR);
            m_2DPixmap.setDevicePixelRatio(m_DPR);
            m_2DPixmap.fill(QColor(0,0,0,0));

        }
    }

    drawOverlay();
    emit newSize();
}

// Called by QT when screen needs to be redrawn
void CPlotter2::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.drawPixmap(0, 0, m_OverlayPixmap);
    painter.drawPixmap(0, 0, m_2DPixmap);
}

// Called to update spectrum data for displaying on the screen
void CPlotter2::draw()
{
    int     i, n;
    int     w;
    int     h;
    int     xmin, xmax;

    if (m_DrawOverlay)
    {
        drawOverlay();
        m_DrawOverlay = false;
    }


    if (!m_Running)
        return;


    // get/draw the 2D spectrum
    w = m_2DPixmap.width() / m_DPR;
    h = m_2DPixmap.height() / m_DPR;

    if (w != 0 && h != 0)
    {
        m_2DPixmap.fill(QColor(0,0,0,0));

        QPainter painter2(&m_2DPixmap);

        // draw the plot
        if(m_polar)
        {
            n = xmax - xmin;
            for (i = 0; i < n; i++)
            {
                LineBuf[i].setX(i + xmin + 0.5);
                LineBuf[i].setY(m_fftbuf[i + xmin] + 0.5);
            }
            painter2.setPen(m_PlotColor);
            painter2.drawPolyline(LineBuf, n);
        }else{//cartesian
            painter2.setPen(m_PlotColor);
            painter2.drawPolyline(LineBuf, n);
        }

        painter2.end();

    }
    // trigger a new paintEvent
    update();
}


/**
 * Set new FFT data.
 * @param data Pointer to the new data used on the pandapter.
 * @param size The data size.
 *
 */

void CPlotter2::setNewData(float *data, int size)
{
    m_data = data;
    m_dataSize = size;
    //TODO: find min/max and scale?
    draw();
}


// Called to draw an overlay bitmap containing grid and text that
// does not need to be recreated every fft data update.
void CPlotter2::drawOverlay()
{
    if (!m_OverlayPixmap.isNull())
    {

        int     w = m_OverlayPixmap.width() / m_DPR;
        int     h = m_OverlayPixmap.height() / m_DPR;
        int     x,y;
        float   pixperdiv;
        float   adjoffset;
        float   dbstepsize;
        float   mindbadj;
        QRect   rect;
        QFontMetrics    metrics(m_Font);
        QPainter        painter(&m_OverlayPixmap);

        painter.setFont(m_Font);

        // solid background
        painter.setBrush(Qt::SolidPattern);
        painter.fillRect(0, 0, w, h, QColor(PLOTTER_BGD_COLOR));

        // X and Y axis areas
        m_YAxisWidth = metrics.boundingRect("-120").width() + 2 * HOR_MARGIN;
        m_XAxisYCenter = h - metrics.height()/2;
        int xAxisHeight = metrics.height() + 2 * VER_MARGIN;
        int xAxisTop = h - xAxisHeight;
        int fLabelTop = xAxisTop + VER_MARGIN;


        if (m_CenterLineEnabled)
        {
            x = xFromFreq(m_CenterFreq);
            if (x > 0 && x < w)
            {
                painter.setPen(QColor(PLOTTER_CENTER_LINE_COLOR));
                painter.drawLine(x, 0, x, xAxisTop);
            }
        }

        // Frequency grid
        qint64  StartFreq = m_CenterFreq + m_FftCenter - m_Span / 2;
        QString label;
        label.setNum(float((StartFreq + m_Span) / m_FreqUnits), 'f', m_FreqDigits);
        calcDivSize(StartFreq, StartFreq + m_Span,
                    qMin(w/(metrics.boundingRect(label).width() + metrics.boundingRect("O").width()), HORZ_DIVS_MAX),
                    m_StartFreqAdj, m_FreqPerDiv, m_HorDivs);
        pixperdiv = (float)w * (float) m_FreqPerDiv / (float) m_Span;
        adjoffset = pixperdiv * float (m_StartFreqAdj - StartFreq) / (float) m_FreqPerDiv;

        painter.setPen(QPen(QColor(PLOTTER_GRID_COLOR), 1, Qt::DotLine));
        for (int i = 0; i <= m_HorDivs; i++)
        {
            x = (int)((float)i * pixperdiv + adjoffset);
            if (x > m_YAxisWidth)
                painter.drawLine(x, 0, x, xAxisTop);
        }

        // draw frequency values (x axis)
        makeFrequencyStrs();
        painter.setPen(QColor(PLOTTER_TEXT_COLOR));
        for (int i = 0; i <= m_HorDivs; i++)
        {
            int tw = w;
            x = (int)((float)i*pixperdiv + adjoffset);
            if (x > m_YAxisWidth)
            {
                rect.setRect(x - tw/2, fLabelTop, tw, metrics.height());
                painter.drawText(rect, Qt::AlignHCenter|Qt::AlignBottom, m_HDivText[i]);
            }
        }

        // Level grid
        qint64 mindBAdj64 = 0;
        qint64 dbDivSize = 0;

        calcDivSize((qint64) m_PandMindB, (qint64) m_PandMaxdB,
                    qMax(h/m_VdivDelta, VERT_DIVS_MIN), mindBAdj64, dbDivSize,
                    m_VerDivs);

        dbstepsize = (float) dbDivSize;
        mindbadj = mindBAdj64;

        pixperdiv = (float) h * (float) dbstepsize / (m_PandMaxdB - m_PandMindB);
        adjoffset = (float) h * (mindbadj - m_PandMindB) / (m_PandMaxdB - m_PandMindB);

        qCDebug(plotter) << "minDb =" << m_PandMindB << "maxDb =" << m_PandMaxdB
                        << "mindbadj =" << mindbadj << "dbstepsize =" << dbstepsize
                        << "pixperdiv =" << pixperdiv << "adjoffset =" << adjoffset;

        painter.setPen(QPen(QColor(PLOTTER_GRID_COLOR), 1, Qt::DotLine));
        for (int i = 0; i <= m_VerDivs; i++)
        {
            y = h - (int)((float) i * pixperdiv + adjoffset);
            if (y < h - xAxisHeight)
                painter.drawLine(m_YAxisWidth, y, w, y);
        }

        // draw amplitude values (y axis)
        painter.setPen(QColor(PLOTTER_TEXT_COLOR));
        for (int i = 0; i < m_VerDivs; i++)
        {
            y = h - (int)((float) i * pixperdiv + adjoffset);
            int th = metrics.height();
            if (y < h -xAxisHeight)
            {
                int dB = mindbadj + dbstepsize * i;
                rect.setRect(HOR_MARGIN, y - th / 2, m_YAxisWidth - 2 * HOR_MARGIN, th);
                painter.drawText(rect, Qt::AlignRight|Qt::AlignVCenter, QString::number(dB));
            }
        }


        painter.end();
    }
}


// Create frequency division strings based on start frequency, span frequency,
// and frequency units.
// Places in QString array m_HDivText
// Keeps all strings the same fractional length
void CPlotter2::makeFrequencyStrs()
{
    qint64  StartFreq = m_StartFreqAdj;
    double  freq;
    int     i,j;

    if ((1 == m_FreqUnits) || (m_FreqDigits == 0))
    {
        // if units is Hz then just output integer freq
        for (i = 0; i <= m_HorDivs; i++)
        {
            freq = (double)StartFreq/(double)m_FreqUnits;
            m_HDivText[i].setNum((int)freq);
            StartFreq += m_FreqPerDiv;
        }
        return;
    }
    // here if is fractional frequency values
    // so create max sized text based on frequency units
    for (i = 0; i <= m_HorDivs; i++)
    {
        freq = (double)StartFreq / (double)m_FreqUnits;
        m_HDivText[i].setNum(freq,'f', m_FreqDigits);
        StartFreq += m_FreqPerDiv;
    }
    // now find the division text with the longest non-zero digit
    // to the right of the decimal point.
    int max = 0;
    for (i = 0; i <= m_HorDivs; i++)
    {
        int dp = m_HDivText[i].indexOf('.');
        int l = m_HDivText[i].length()-1;
        for (j = l; j > dp; j--)
        {
            if (m_HDivText[i][j] != '0')
                break;
        }
        if ((j - dp) > max)
            max = j - dp;
    }
    // truncate all strings to maximum fractional length
    StartFreq = m_StartFreqAdj;
    for (i = 0; i <= m_HorDivs; i++)
    {
        freq = (double)StartFreq/(double)m_FreqUnits;
        m_HDivText[i].setNum(freq,'f', max);
        StartFreq += m_FreqPerDiv;
    }
}


// Ensure overlay is updated by either scheduling or forcing a redraw
void CPlotter2::updateOverlay()
{
    if (m_Running)
        m_DrawOverlay = true;
    else
        drawOverlay();
}


void CPlotter2::calcDivSize (qint64 low, qint64 high, int divswanted, qint64 &adjlow, qint64 &step, int& divs)
{
    qCDebug(plotter) << "low:" << low;
    qCDebug(plotter) << "high:" << high;
    qCDebug(plotter) << "divswanted:" << divswanted;

    if (divswanted == 0)
        return;

    static const qint64 stepTable[] = { 1, 2, 5 };
    static const int stepTableSize = sizeof (stepTable) / sizeof (stepTable[0]);
    qint64 multiplier = 1;
    step = 1;
    divs = high - low;
    int index = 0;
    adjlow = (low / step) * step;

    while (divs > divswanted)
    {
        step = stepTable[index] * multiplier;
        divs = int ((high - low) / step);
        adjlow = (low / step) * step;
        index = index + 1;
        if (index == stepTableSize)
        {
            index = 0;
            multiplier = multiplier * 10;
        }
    }
    if (adjlow < low)
        adjlow += step;

    qCDebug(plotter) << "adjlow:" << adjlow;
    qCDebug(plotter) << "step:" << step;
    qCDebug(plotter) << "divs:" << divs;
}

void CPlotter2::showToolTip(QMouseEvent* event, QString toolTipText)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QToolTip::showText(event->globalPos(), toolTipText, this);
#else
    QToolTip::showText(event->globalPosition().toPoint(), toolTipText, this);
#endif
}

