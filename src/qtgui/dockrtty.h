/*
 * rtty gui widget header
 *
 * Copyright 2022 Marc CAPDEVILLE F4JMZ
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#ifndef DOCKRTTY_H
#define DOCKRTTY_H
#include <QDockWidget>
#include <QSettings>

namespace Ui {
    class DockRTTY;
}


class DockRTTY : public QDockWidget
{
    Q_OBJECT

public:
    explicit DockRTTY(QWidget *parent = 0);
    ~DockRTTY();

    float get_baud_rate();
    float get_mark_freq();
    float get_space_freq();
    int   get_mode();
    int   get_parity();

public slots:
    void update_text(QString text);
    QString get_text();
    void show_Enabled();
    void show_Disabled();
    void set_Enabled();
    void set_Disabled();
    void set_baud_rate(float);
    void set_mark_freq(float);
    void set_space_freq(float);
    void ClearText();

private:
    float d_baud_rate;
    float d_mark_freq;
    float d_space_freq;
    int d_mode;
    int d_parity;

signals:
    void rtty_start_decoder();
    void rtty_stop_decoder();
    void rtty_reset_clicked();
    void rtty_baud_rate_Changed(float);
    void rtty_mark_freq_Changed(float);
    void rtty_space_freq_Changed(float);
    void rtty_mode_Changed(int);
    void rtty_parity_Changed(int);
    void rtty_save_clicked(QString);

private slots:
    void on_rttyEnable_toggled(bool);
    void on_baud_rate_editingFinished();
    void on_mark_freq_editingFinished();
    void on_space_freq_editingFinished();
    void on_mode_currentIndexChanged(int);
    void on_parity_currentIndexChanged(int);
    void on_reset_clicked();
    void on_swap_clicked();
    void on_save_clicked();

private:
    Ui::DockRTTY *ui;        /*! The Qt designer UI file. */
};

#endif // DOCKRTTY_H