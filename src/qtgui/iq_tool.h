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
#include <QListWidget>
#include "dsp/format_converter.h"
#include "applications/gqrx/dcontrols_ui.h"


namespace Ui {
    class CIqTool;
}


struct iqt_cplx
{
    float re;
    float im;
};


/*! \brief User interface for I/Q recording and playback. */
class CIqTool : public QDialog, public dcontrols_ui
{
    Q_OBJECT

public:
    explicit CIqTool(QWidget *parent = 0);
    ~CIqTool();

    void setSampleRate(qint64 sr);

    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent * event) override;

    qint64 selectionLength();

signals:
    void startRecording(const QString recdir, file_formats fmt);
    void stopRecording();
    void startPlayback(const QString filename, float samprate,
                       qint64 center_freq, file_formats fmt,
                       qint64 time_ms);
    void stopPlayback();
    void seek(qint64 seek_pos);
    void saveFileRange(const QString &, file_formats, quint64,quint64);

public slots:
    void cancelRecording();
    void cancelPlayback();
    void updateStats(bool hasFailed, int buffersUsed, size_t fileSize);
    void updateSaveProgress(const qint64 save_progress);
    void setRunningState(bool);

private slots:
    void on_listWidget_currentTextChanged(const QString &currentText);
    void timeoutFunction(void);

private:
    void playObserver(c_id, const c_def::v_union&);
    void posObserver(c_id, const c_def::v_union&);
    void recObserver(c_id, const c_def::v_union&);
    void locationObserver(c_id, const c_def::v_union&);
    void selectObserver(c_id, const c_def::v_union&);
    void extractDirObserver(c_id, const c_def::v_union&);
    void formatObserver(c_id, const c_def::v_union&);
    void fineStepObserver(c_id, const c_def::v_union&);
    void selAObserver(c_id, const c_def::v_union&);
    void selBObserver(c_id, const c_def::v_union&);
    void goAObserver(c_id, const c_def::v_union&);
    void goBObserver(c_id, const c_def::v_union&);
    void resetObserver(c_id, const c_def::v_union&);
    void saveObserver(c_id, const c_def::v_union&);
    void refreshDir(void);
    void refreshTimeWidgets(void);
    void parseFileName(const QString &filename);
    void switchControlsState(bool recording, bool playback);
    void updateSliderStylesheet(qint64 save_progress = -2);
    void listWidgetFileSelected(const QString &currentText);
    // No spacer at bottom here
    void finalizeInner() override;

private:
    Ui::CIqTool *ui;

    QDir        *recdir;
    QDir        *extractDir;
    QTimer      *timer;
    QPalette    *error_palette; /*!< Palette used to indicate an error. */
    QListWidget *listWidget;

    QString current_file;      /*!< Selected file in file browser. */

    double  sel_A{-1.0};
    double  sel_B{-1.0};
    bool    is_recording;
    bool    is_playing;
    bool    is_saving{false};
    bool    is_running{false};
    bool    can_play{false};
    qint64  chunk_size;
    qint64  samples_per_chunk;
    file_formats fmt;
    file_formats rec_fmt;
    quint64 time_ms;
    qint64  rec_sample_rate{192000}; /*!< Current recording sample rate. */
    qint64  sample_rate;             /*!< Current playback sample rate. */
    qint64  center_freq;             /*!< Center frequency. */
    qint64  rec_len;                 /*!< Length of a recording in seconds */
    int     o_buffersUsed{0};
    size_t  o_fileSize{0};
};

#endif // IQ_TOOL_H
