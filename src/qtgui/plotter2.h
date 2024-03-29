/* -*- c++ -*- */
#ifndef PLOTTER2_H
#define PLOTTER2_H

#include <QtGui>
#include <QFont>
#include <QFrame>
#include <QImage>
#include <vector>
#include <mutex>
#include <QMap>
#include "receivers/defines.h"
#include "receivers/vfo.h"

#define HORZ_DIVS_MAX 12    //50
#define VERT_DIVS_MIN 5
#define MAX_SCREENSIZE 16384


class CPlotter2 : public QFrame
{
    Q_OBJECT

public:
    explicit CPlotter2(QWidget *parent = nullptr);
    ~CPlotter2() override;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    void draw(); //call to draw new fft data onto screen plot
    void setTooltipsEnabled(bool enabled) { m_TooltipsEnabled = enabled; }

    void setNewData(float *data, int size);

signals:
    void newSize();

public slots:
    void updateOverlay();

protected:
    //re-implemented widget event handlers
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent * event) override;
    void mousePressEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;
    void wheelEvent( QWheelEvent * event ) override;

private:
    void        drawOverlay();
    void        makeFrequencyStrs();
    static void calcDivSize (qint64 low, qint64 high, int divswanted, qint64 &adjlow, qint64 &step, int& divs);
    void        showToolTip(QMouseEvent* event, QString toolTipText);

    float      *m_data{};
    int         m_dataSize;
    QPixmap     m_2DPixmap;
    QPixmap     m_OverlayPixmap;
    QColor      m_PlotColor{255,255,255};
    QSize       m_Size;
    qreal       m_DPR{};
    QString     m_HDivText[HORZ_DIVS_MAX+1];
    bool        m_DrawOverlay;
    bool        m_TooltipsEnabled{};  /*!< Tooltips enabled */
    bool        m_InvertScrolling;
    bool        m_polar{false};
    int         m_CursorCaptureDelta;
    int         m_GrabPosition;

    int         m_FLowCmin;
    int         m_FLowCmax;
    int         m_FHiCmin;
    int         m_FHiCmax;
    bool        m_symetric;

    int         m_XAxisYCenter{};
    int         m_YAxisWidth{};
    int         m_HorDivs;   /*!< Current number of horizontal divisions. Calculated from width. */
    int         m_VerDivs;   /*!< Current number of vertical divisions. Calculated from height. */


    int         m_Xzero{};
    int         m_Yzero{};  /*!< Used to measure mouse drag direction. */
    int         m_FreqDigits;  /*!< Number of decimal digits in frequency strings. */

    QFont       m_Font;      /*!< Font used for plotter (system font) */
    int         m_HdivDelta; /*!< Minimum distance in pixels between two horizontal grid lines (vertical division). */
    int         m_VdivDelta; /*!< Minimum distance in pixels between two vertical grid lines (horizontal division). */
};

#endif // PLOTTER_H
