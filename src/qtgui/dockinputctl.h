/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2013 Alexandru Csete OZ9AEC.
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
#ifndef DOCKINPUTCTL_H
#define DOCKINPUTCTL_H

#include <vector>
#include <string>

#include <QDockWidget>
#include <QGridLayout>
#include <QLabel>
#include <QList>
#include <QSettings>
#include <QSlider>
#include <QMap>
#include <QString>
#include <QVariant>
#include "applications/gqrx/dcontrols_ui.h"


/*! \brief Structure describing a gain parameter with its range. */
typedef struct
{
    std::string name;   /*!< The name of this gain stage. */
    double      value;  /*!< Initial value. */
    double      start;  /*!< The lower limit. */
    double      stop;   /*!< The uppewr limit. */
    double      step;   /*!< The resolution/step. */
} gain_t;

/*! \brief A vector with gain parameters.
 *
 * This data structure is used for transferring
 * information about available gain stages.
 */
typedef std::vector<gain_t> gain_list_t;


namespace Ui {
    class DockInputCtl;
}

class DockInputCtl : public QDockWidget, public dcontrols_ui
{
    Q_OBJECT

public:
    explicit DockInputCtl(QWidget * parent = 0);
    ~DockInputCtl();

    void    readSettings(QSettings * settings);
    void    saveSettings(QSettings * settings);

    double  gain(QString &name);

    void    setAntennas(std::vector<std::string> &antennas);

    void    setGainStages(gain_list_t &gain_list);
    void    restoreManualGains(void);

public slots:
    bool    setGain(QString name, double value);

signals:
    void gainChanged(QString name, double value);

private slots:

    void sliderValueChanged(int value);

private:
    void clearWidgets();
    void updateLabel(int idx, double value);
    void getGains(QMap<QString, QVariant> * gains);
    void setGains(QMap<QString, QVariant> * gains);
    void hwagcObserver(c_id, const c_def::v_union & v);

private:
    QList<QSlider *>  gain_sliders; /*!< A list containing the gain sliders. */
    QList<QLabel *>   gain_labels;  /*!< A list containing the gain labels. */
    QList<QLabel *>   value_labels; /*!< A list containing labels showing the current gain value. */
    QGridLayout * gainLayout;

    Ui::DockInputCtl *ui;           /*!< User interface. */
};

#endif // DOCKINPUTCTL_H
