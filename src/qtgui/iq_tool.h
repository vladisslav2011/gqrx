/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2014 Alexandru Csete OZ9AEC.
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
#ifndef IQ_TOOL_H
#define IQ_TOOL_H

#include <QCloseEvent>
#include <QDialog>
#include <QDir>
#include <QPalette>
#include <QSettings>
#include <QShowEvent>
#include <QString>
#include <QTimer>
#include <QMenu>
#include "applications/gqrx/receiver.h"


namespace Ui {
    class CIqTool;
}


struct iqt_cplx
{
    float re;
    float im;
};


/*! \brief User interface for I/Q recording and playback. */
class CIqTool : public QDialog
{
    Q_OBJECT

public:
    explicit CIqTool(QWidget *parent = 0);
    ~CIqTool();

    void setSampleRate(qint64 sr);

    void closeEvent(QCloseEvent *event);
    void showEvent(QShowEvent * event);

    void saveSettings(QSettings *settings);
    void readSettings(QSettings *settings);
    qint64 selectionLength();

signals:
    void startRecording(const QString recdir, enum receiver::file_formats fmt,
                        int buffers_max);
    void stopRecording();
    void startPlayback(const QString filename, float samprate,
                       qint64 center_freq, enum receiver::file_formats fmt,
                       qint64 time_ms, int buffers_max, bool repeat);
    void stopPlayback();
    void seek(qint64 seek_pos);
    void saveFileRange(const QString &, enum receiver::file_formats, quint64,quint64);

public slots:
    void cancelRecording();
    void cancelPlayback();
    void updateStats(bool hasFailed, int buffersUsed, size_t fileSize);
    void updateSaveProgress(const qint64 save_progress);

private slots:
    void on_recDirEdit_textChanged(const QString &text);
    void on_recDirButton_clicked();
    void on_recButton_clicked(bool checked);
    void on_playButton_clicked(bool checked);
    void on_slider_valueChanged(int value);
    void on_listWidget_currentTextChanged(const QString &currentText);
    void timeoutFunction(void);
    void on_formatCombo_currentIndexChanged(int index);
    void on_slider_customContextMenuRequested(const QPoint& pos);
    void sliderA();
    void sliderB();
    void sliderReset();
    void sliderSave();
    void sliderGoA();
    void sliderGoB();

private:
    void refreshDir(void);
    void refreshTimeWidgets(void);
    void parseFileName(const QString &filename);
    void switchControlsState(bool recording, bool playback);
    void updateSliderStylesheet(qint64 save_progress = -2);

private:
    Ui::CIqTool *ui;

    QDir        *recdir;
    QTimer      *timer;
    QPalette    *error_palette; /*!< Palette used to indicate an error. */

    QString current_file;      /*!< Selected file in file browser. */
    QMenu       *sliderMenu;
    QAction     *setA;
    QAction     *setB;
    QAction     *selSave;
    QAction     *selReset;
    QAction     *goA;
    QAction     *goB;

    double  sel_A{-1.0};
    double  sel_B{-1.0};
    bool    is_recording;
    bool    is_playing;
    bool    is_saving{false};
    int     chunk_size;
    qint64  samples_per_chunk;
    enum receiver::file_formats fmt;
    enum receiver::file_formats rec_fmt;
    quint64 time_ms;
    qint64  sample_rate;       /*!< Current sample rate. */
    qint64  center_freq;       /*!< Center frequency. */
    qint64  rec_len;           /*!< Length of a recording in seconds */
    int     o_buffersUsed{0};
    size_t  o_fileSize{0};
};

#endif // IQ_TOOL_H
