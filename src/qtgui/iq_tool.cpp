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
#include <QMessageBox>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QPalette>
#include <QString>
#include <QStringList>
#include <QScrollBar>
#include <QDateTime>
#include <QShortcut>

#include <math.h>

#include "iq_tool.h"
#include "ui_iq_tool.h"


CIqTool::CIqTool(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CIqTool)
{
    ui->setupUi(this);

    is_recording = false;
    is_playing = false;
    chunk_size = 8;
    samples_per_chunk = 1;
    fmt = FILE_FORMAT_CF;
    rec_fmt = FILE_FORMAT_CF;
    sample_rate = 192000;
    rec_len = 0;
    center_freq = 1e8;
    time_ms = 0;

    recdir = new QDir(QDir::homePath(), "*.raw");
    recdir->setNameFilters(recdir->nameFilters() << "*.sigmf-data");
    extractDir = new QDir(QDir::homePath(), "*.raw");

    error_palette = new QPalette();
    error_palette->setColor(QPalette::Text, Qt::red);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timeoutFunction()));
    grid_init(ui->gridLayout,ui->gridLayout->rowCount(),0/*ui->gridLayout->columnCount()*/);
    listWidget = new QListWidget(this);
    ui->gridLayout->addWidget(listWidget,1,0,1,7);
    connect(listWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(on_listWidget_currentTextChanged(const QString&)));
    set_observer(C_IQ_PLAY,&CIqTool::playObserver);
    set_observer(C_IQ_POS,&CIqTool::posObserver);
    set_observer(C_IQ_REC,&CIqTool::recObserver);
    set_observer(C_IQ_LOCATION,&CIqTool::locationObserver);
    set_observer(C_IQ_SELECT,&CIqTool::selectObserver);
    set_observer(C_IQ_SAVE_LOC,&CIqTool::extractDirObserver);
    set_observer(C_IQ_FORMAT,&CIqTool::formatObserver);
    set_observer(C_IQ_SEL_A,&CIqTool::selAObserver);
    set_observer(C_IQ_SEL_B,&CIqTool::selBObserver);
    set_observer(C_IQ_GOTO_A,&CIqTool::goAObserver);
    set_observer(C_IQ_GOTO_B,&CIqTool::goBObserver);
    set_observer(C_IQ_RESET_SEL,&CIqTool::resetObserver);
    set_observer(C_IQ_SAVE_SEL,&CIqTool::saveObserver);
    set_observer(C_IQ_FINE_STEP,&CIqTool::fineStepObserver);
}

CIqTool::~CIqTool()
{
    timer->stop();
    delete timer;
    delete ui;
    delete recdir;
    delete extractDir;
    delete error_palette;

}

void CIqTool::finalizeInner()
{
    getWidget(C_IQ_BUF_STAT)->hide();
    getWidget(C_IQ_SIZE_STAT)->hide();
    getAction(C_IQ_SAVE_SEL)->setEnabled(false);
    getAction(C_IQ_GOTO_A)->setEnabled(false);
    getAction(C_IQ_GOTO_B)->setEnabled(false);
    getAction(C_IQ_RESET_SEL)->setEnabled(false);
    getWidget(C_IQ_FORMAT)->show();
    getWidget(C_IQ_BUFFERS)->show();
    getWidget(C_IQ_BUF_STAT)->hide();
    getWidget(C_IQ_SIZE_STAT)->hide();
}

/*! \brief Set new sample rate. */
void CIqTool::setSampleRate(qint64 sr)
{
    rec_sample_rate = sr;

    if (!current_file.isEmpty())
    {
        // Get duration of selected recording and update label
        QFileInfo info(*recdir, current_file);
        rec_len = info.size() * samples_per_chunk / (sample_rate * chunk_size);
        dynamic_cast<QSlider *>(getWidget(C_IQ_POS))->setMaximum(rec_len);
        refreshTimeWidgets();
    }
}


/*! \brief Slot activated when the user selects a file. */
void CIqTool::on_listWidget_currentTextChanged(const QString &currentText)
{
    listWidgetFileSelected(currentText);
    if(is_playing && can_play)
    {
        sel_A=sel_B=-1.0;
        updateSliderStylesheet();
        switchControlsState(false, true);

        emit startPlayback(recdir->absoluteFilePath(current_file),
                            (float)sample_rate, center_freq, fmt,
                            time_ms);
    }
}

void CIqTool::listWidgetFileSelected(const QString &currentText)
{
    current_file = currentText;
    QFileInfo info(*recdir, current_file);

    parseFileName(currentText);
    rec_len = info.size() * samples_per_chunk / (sample_rate * chunk_size);
    can_play = info.size() > chunk_size;

    // Get duration of selected recording and update label
    dynamic_cast<QSlider *>(getWidget(C_IQ_POS))->setMaximum(rec_len);
    refreshTimeWidgets();
}

void CIqTool::setRunningState(bool state)
{
    is_running = state;
    listWidget->setEnabled(!(is_recording || (is_playing && is_running)));
}

/*! \brief Show/hide/enable/disable GUI controls */

void CIqTool::switchControlsState(bool recording, bool playback)
{
    getWidget(C_IQ_REC)->setEnabled(!playback);

    getWidget(C_IQ_PLAY)->setEnabled(!recording);
    getWidget(C_IQ_POS)->setEnabled(!recording);

    getWidget(C_IQ_REPEAT)->setEnabled(!(recording || playback));
    listWidget->setEnabled(!(recording || (playback && is_running)));
    getWidget(C_IQ_LOCATION)->setEnabled(!(recording || playback));
    getWidget(C_IQ_SELECT)->setEnabled(!(recording || playback));
    getWidget(C_IQ_FORMAT)->setEnabled(!(recording || playback));
    getWidget(C_IQ_BUFFERS)->setEnabled(!(recording || playback));
    getAction(C_IQ_SEL_A)->setEnabled(playback);
    getAction(C_IQ_SEL_B)->setEnabled(playback);
    if (recording || playback)
    {
        qvariant_cast<QWidget *>(getWidget(C_IQ_FORMAT)->property("title"))->hide();
        qvariant_cast<QWidget *>(getWidget(C_IQ_BUFFERS)->property("title"))->hide();
        getWidget(C_IQ_FORMAT)->hide();
        getWidget(C_IQ_BUFFERS)->hide();
        getWidget(C_IQ_BUF_STAT)->show();
        getWidget(C_IQ_SIZE_STAT)->show();
    }
    else
    {
        qvariant_cast<QWidget *>(getWidget(C_IQ_FORMAT)->property("title"))->show();
        qvariant_cast<QWidget *>(getWidget(C_IQ_BUFFERS)->property("title"))->show();
        getWidget(C_IQ_FORMAT)->show();
        getWidget(C_IQ_BUFFERS)->show();
        getWidget(C_IQ_BUF_STAT)->hide();
        getWidget(C_IQ_SIZE_STAT)->hide();
    }
}

/*! \brief Start/stop playback */
void CIqTool::playObserver(c_id, const c_def::v_union &v)
{
    QMessageBox msg_box;
    msg_box.setIcon(QMessageBox::Critical);
    if(!can_play)
    {
        msg_box.setText(tr("Invalid I/Q file selected. Zero length."));
        msg_box.exec();

        set_gui(C_IQ_PLAY, false);
        return;
    }
    is_playing = v;

    if (bool(v))
    {
        if (current_file.isEmpty())
        {
            if (listWidget->count() == 0)
            {
                msg_box.setText(tr("There are no I/Q files in the current directory."));
            }
            else
            {
                msg_box.setText(tr("Please select a file to play."));
            }
            msg_box.exec();

            set_gui(C_IQ_PLAY, false);
        }
        else
        {
            sel_A=sel_B=-1.0;
            updateSliderStylesheet();
            listWidgetFileSelected(current_file);
            switchControlsState(false, true);

            emit startPlayback(recdir->absoluteFilePath(current_file),
                               (float)sample_rate, center_freq, fmt,
                               time_ms);
        }
    }
    else
    {
        emit stopPlayback();
        switchControlsState(false, false);
        set_gui(C_IQ_POS,0);
    }
}

/*! \brief Cancel playback.
 *
 * This slot can be activated to cancel an ongoing playback.
 *
 * This slot should be used to signal that a playback could not be started.
 */
void CIqTool::cancelPlayback()
{
    switchControlsState(false, false);
    is_playing = false;
}


/*! \brief Slider value (seek position) has changed. */
void CIqTool::posObserver(c_id, const c_def::v_union &v)
{
    qint64 seek_pos = qint64(v) * sample_rate / samples_per_chunk;
    emit seek(seek_pos);
    refreshTimeWidgets();
    updateStats(false, o_buffersUsed, seek_pos);
}

/*! \brief Start/stop recording */
void CIqTool::recObserver(c_id, const c_def::v_union &v)
{
    is_recording = v;

    if (is_recording)
    {
        switchControlsState(true, false);
        c_def::v_union buffers;//TODO: remove this
        get_gui(C_IQ_BUFFERS, buffers);
        emit startRecording(recdir->path(), rec_fmt);

        refreshDir();
//        listWidget->setCurrentRow(listWidget->count()-1);
        listWidget->setCurrentRow(-1);
        c_def::v_union fmt;
        get_gui(C_IQ_FORMAT, fmt);
        samples_per_chunk = any_to_any_base::fmt[int(fmt)].nsamples;
        chunk_size = any_to_any_base::fmt[int(fmt)].size;
    }
    else
    {
        switchControlsState(false, false);
        emit stopRecording();
    }
}

/*! \brief Cancel a recording.
 *
 * This slot can be activated to cancel an ongoing recording. Cancelling an
 * ongoing recording will stop the recording and delete the recorded file, if
 * any.
 *
 * This slot should be used to signal that a recording could not be started.
 */
void CIqTool::cancelRecording()
{
    set_gui(C_IQ_REC, false, true);
}

/*! \brief Update GUI ta match current recorder state.
 *
 * This slot can be periodically activated to show recording progress
 */
void CIqTool::updateStats(bool hasFailed, int buffersUsed, size_t fileSize)
{
    if(is_recording)
    {
        if (hasFailed)
        {
            QMessageBox msg_box;
            msg_box.setIcon(QMessageBox::Critical);
            msg_box.setText(tr("IQ recording failed."));
            msg_box.exec();
        }
        else
        {
            set_gui(C_IQ_BUF_STAT,buffersUsed);
            if(o_fileSize != fileSize)
            {
                rec_len = (int)(fileSize * samples_per_chunk / rec_sample_rate);
                set_gui(C_IQ_SIZE_STAT,QString("Size: %1 bytes").arg(fileSize * chunk_size).toStdString());
                refreshTimeWidgets();
                dynamic_cast<QSlider *>(getWidget(C_IQ_POS))->setMaximum(rec_len);
            }
            o_buffersUsed = buffersUsed;
            o_fileSize = fileSize;
        }
    }else{
        if(is_playing)
            set_gui(C_IQ_BUF_STAT,buffersUsed);
        if(o_fileSize != fileSize)
            set_gui(C_IQ_SIZE_STAT,QString("Pos: %1 bytes").arg(fileSize * chunk_size).toStdString());
        o_buffersUsed = buffersUsed;
        o_fileSize = fileSize;
        int val = fileSize * samples_per_chunk / sample_rate ;
        if (val < dynamic_cast<QSlider *>(getWidget(C_IQ_POS))->maximum())
            set_gui(C_IQ_POS,val);
        refreshTimeWidgets();
    }
 }

void CIqTool::updateSaveProgress(const qint64 save_progress)
{
    if(save_progress<0)
    {
        sel_A=sel_B=-1.0;
        is_saving = false;
        updateSliderStylesheet();
    }else
        updateSliderStylesheet(save_progress);
}


/*! \brief Catch window close events.
 *
 * This method is called when the user closes the audio options dialog
 * window using the window close icon. We catch the event and hide the
 * dialog but keep it around for later use.
 */
void CIqTool::closeEvent(QCloseEvent *event)
{
    timer->stop();
    hide();
    event->ignore();
}

/*! \brief Catch window show events. */
void CIqTool::showEvent(QShowEvent * event)
{
    Q_UNUSED(event);
    refreshDir();
    dynamic_cast<QSlider *>(getWidget(C_IQ_POS))->setMaximum(rec_len);
    refreshTimeWidgets();
    timer->start(1000);
}


/*! \brief Slot called when the recordings directory has changed either
 *         because of user input or programmatically.
 */
void CIqTool::locationObserver(c_id, const c_def::v_union &v)
{
    auto dir = QString::fromStdString(v);
    if (dir == "")
        dir = QDir::homePath();
    if (recdir->exists(dir))
    {
        getWidget(C_IQ_LOCATION)->setPalette(QPalette());  // Clear custom color
        recdir->setPath(dir);
        recdir->cd(dir);
        extractDir->setPath(dir);
        extractDir->cd(dir);
        //emit newRecDirSelected(dir);
    }
    else
    {
        getWidget(C_IQ_LOCATION)->setPalette(*error_palette);  // indicate error
    }
}

/*! \brief Slot called when the user clicks on the "Select" button. */
void CIqTool::selectObserver(c_id, const c_def::v_union &)
{
    c_def::v_union v;
    get_gui(C_IQ_LOCATION,v);
    auto dir = QString::fromStdString(v);
    if (dir == "")
        dir = QDir::homePath();
    QString newdir = QFileDialog::getExistingDirectory(this, tr("Select a directory"),
                                                    dir,
                                                    QFileDialog::ShowDirsOnly |
                                                    QFileDialog::DontResolveSymlinks);

    if (!newdir.isNull())
        set_gui(C_IQ_LOCATION, newdir.toStdString(),true);
}

void CIqTool::extractDirObserver(c_id, const c_def::v_union &)
{
    c_def::v_union v;
    get_gui(C_IQ_LOCATION,v);
    auto dir = QString::fromStdString(v);
    if (dir == "")
        dir = QDir::homePath();
    QString newdir = QFileDialog::getExistingDirectory(this, tr("Select a directory to place extracted files"),
                                                    dir,
                                                    QFileDialog::ShowDirsOnly |
                                                    QFileDialog::DontResolveSymlinks);

    if (!newdir.isNull())
    {
        extractDir->setPath(newdir);
        extractDir->cd(newdir);
        getAction(C_IQ_SAVE_LOC)->setText("Save to "+extractDir->path());
    }
}

void CIqTool::timeoutFunction(void)
{
    refreshDir();
}

void CIqTool::formatObserver(c_id, const c_def::v_union &v)
{
    rec_fmt = file_formats(int(v));
}

void CIqTool::updateSliderStylesheet(qint64 save_progress)
{
    if((sel_A<0.0)&&(sel_B<0.0))
    {
        getWidget(C_IQ_POS)->setStyleSheet("");
        getAction(C_IQ_SEL_A)->setText("A");
        getAction(C_IQ_SEL_B)->setText("B");
        getAction(C_IQ_SAVE_SEL)->setText("Save");
        getAction(C_IQ_SAVE_SEL)->setEnabled(false);
        getAction(C_IQ_GOTO_A)->setEnabled(false);
        getAction(C_IQ_GOTO_B)->setEnabled(false);
        getAction(C_IQ_SEL_A)->setEnabled(true);
        getAction(C_IQ_SEL_B)->setEnabled(true);
        getAction(C_IQ_RESET_SEL)->setEnabled(true);
        return;
    }
    getAction(C_IQ_GOTO_A)->setEnabled(true);
    getAction(C_IQ_GOTO_B)->setEnabled(true);
    if((sel_A>=0.0) && (sel_B<0.0))
    {
        sel_B=1.0;
    }
    if((sel_B>=0.0) && (sel_A<0.0))
    {
        sel_A=0.0;
    }
    if(sel_A>sel_B)
        std::swap(sel_A,sel_B);
    getAction(C_IQ_SEL_A)->setText(QString("A: %1 s").arg(quint64(sel_A * double(rec_len))));
    getAction(C_IQ_SEL_B)->setText(QString("B: %1 s").arg(quint64(sel_B * double(rec_len))));
    getAction(C_IQ_SAVE_SEL)->setText(QString("Save: %1 s %2 mb")
        .arg(quint64((sel_B-sel_A) * double(rec_len)))
        .arg((sel_B-sel_A) * double(rec_len) * double(sample_rate) * double(chunk_size)/double(samples_per_chunk)*1e-6,2,'f',2,' ')
    );
    double selLen=sel_B-sel_A;
    if(selLen<0.00001)
        selLen=0.00001;
    selLen+=sel_A;
    if(selLen>1.0)
        selLen=1.0;
    if(save_progress == -1)
    {
        getAction(C_IQ_SEL_A)->setEnabled(true);
        getAction(C_IQ_SEL_B)->setEnabled(true);
        if(sel_A * double(rec_len) >= 1.0)
            getAction(C_IQ_SAVE_SEL)->setEnabled(true);
        getAction(C_IQ_RESET_SEL)->setEnabled(true);
    }
    if(save_progress>0)
    {
        double prgLen = sel_A + double(save_progress)/(double(rec_len)*1000.0);
        if(prgLen>1.0)
            prgLen=1.0;
        if(prgLen<0.000007)
            prgLen=0.000007;
        getWidget(C_IQ_POS)->setStyleSheet(QString(
        "background-color: qlineargradient("
            "spread:repeat,"
            "x1:0,"
            "y1:0,"
            "x2:1,"
            "y2:0,"
            "stop:0 rgba(255, 255, 255, 0),"
            "stop:%1 rgba(255, 255, 255, 0),"
            "stop:%2 rgba(0, 255, 0, 255),"
            "stop:%3 rgba(0, 255, 0, 255),"
            "stop:%4 rgba(255, 0, 0, 255),"
            "stop:%5 rgba(255, 0, 0, 255),"
            "stop:%6 rgba(255, 255, 255, 0),"
            "stop:1 rgba(255, 255, 255, 0)"
        ");").arg(sel_A+0.000001)
            .arg(sel_A+0.000002)
            .arg(prgLen-0.000004)
            .arg(prgLen-0.000003)
            .arg(selLen-0.000002)
            .arg(selLen-0.000001)
            );
    }
    else
    {
        getWidget(C_IQ_POS)->setStyleSheet(QString(
        "background-color: qlineargradient("
            "spread:repeat,"
            "x1:0,"
            "y1:0,"
            "x2:1,"
            "y2:0,"
            "stop:0 rgba(255, 255, 255, 0),"
            "stop:%1 rgba(255, 255, 255, 0),"
            "stop:%2 rgba(255, 0, 0, 255),"
            "stop:%3 rgba(255, 0, 0, 255),"
            "stop:%4 rgba(255, 255, 255, 0),"
            "stop:1 rgba(255, 255, 255, 0)"
        ");").arg(sel_A+0.000001)
            .arg(sel_A+0.000002)
            .arg(selLen-0.000002)
            .arg(selLen-0.000001)
            );
        if(sel_A * double(rec_len) >= 1.0)
            getAction(C_IQ_SAVE_SEL)->setEnabled(true);
    }
}

void CIqTool::fineStepObserver(c_id, const c_def::v_union &v)
{
    const c_def & def = c_def::all()[C_IQ_POS];
    const int step = bool(v)?1:int(def.step());
    dynamic_cast<QSlider *>(getWidget(C_IQ_POS))->setSingleStep(step);
}

void CIqTool::selAObserver(c_id, const c_def::v_union &v)
{
    if(is_saving)
        return;
    sel_A=double(o_fileSize * samples_per_chunk)/double(rec_len * sample_rate);
    updateSliderStylesheet();
}

void CIqTool::selBObserver(c_id, const c_def::v_union &v)
{
    if(is_saving)
        return;
    sel_B=double(o_fileSize * samples_per_chunk)/double(rec_len * sample_rate);
    updateSliderStylesheet();
}

void CIqTool::goAObserver(c_id, const c_def::v_union &v)
{
    set_gui(C_IQ_POS, qint64(sel_A*dynamic_cast<QSlider *>(getWidget(C_IQ_POS))->maximum()), true);
}

void CIqTool::goBObserver(c_id, const c_def::v_union &v)
{
    set_gui(C_IQ_POS, qint64(sel_B*dynamic_cast<QSlider *>(getWidget(C_IQ_POS))->maximum()), true);
}

void CIqTool::resetObserver(c_id, const c_def::v_union &v)
{
    if(is_saving)
        return;
    sel_A=sel_B=-1.0;
    updateSliderStylesheet();
}

void CIqTool::saveObserver(c_id, const c_def::v_union &v)
{
    if(sel_A<0.0)
        return;
    is_saving=true;
    quint64 from_ms=sel_A*double(rec_len)*1000.0+time_ms;
    quint64 len_ms=(sel_B-sel_A)*double(rec_len)*1000.0;
    if(len_ms<1.0)
        len_ms=1.0;
    emit saveFileRange(extractDir->path(), fmt, from_ms, len_ms);
    updateSliderStylesheet(0);
    getAction(C_IQ_SEL_A)->setEnabled(false);
    getAction(C_IQ_SEL_B)->setEnabled(false);
    getAction(C_IQ_SAVE_SEL)->setEnabled(false);
    getAction(C_IQ_RESET_SEL)->setEnabled(false);
}

qint64 CIqTool::selectionLength()
{
    return (sel_B-sel_A)*double(rec_len)*1000.0;
}


/*! \brief Refresh list of files in current working directory. */
void CIqTool::refreshDir()
{
    int selection = listWidget->currentRow();
    QScrollBar * sc = listWidget->verticalScrollBar();
    int lastScroll = sc->sliderPosition();

    recdir->refresh();
    QStringList files = recdir->entryList();

    listWidget->blockSignals(true);
    listWidget->clear();
    listWidget->insertItems(0, files);
    listWidget->setCurrentRow(selection);
    sc->setSliderPosition(lastScroll);
    listWidget->blockSignals(false);
}

/*! \brief Refresh time labels and slider position
 *
 * \note Safe for recordings > 24 hours
 */
void CIqTool::refreshTimeWidgets(void)
{
    // duration
    int len = rec_len;
    int lh, lm, ls;
    lh = len / 3600;
    len = len % 3600;
    lm = len / 60;
    ls = len % 60;

    // current position
    c_def::v_union v;
    get_gui(C_IQ_POS,v);
    int pos = v;
    int ph, pm, ps;
    ph = pos / 3600;
    pos = pos % 3600;
    pm = pos / 60;
    ps = pos % 60;

    set_gui(C_IQ_TIME, QString("%1:%2:%3 / %4:%5:%6")
                           .arg(ph, 2, 10, QChar('0'))
                           .arg(pm, 2, 10, QChar('0'))
                           .arg(ps, 2, 10, QChar('0'))
                           .arg(lh, 2, 10, QChar('0'))
                           .arg(lm, 2, 10, QChar('0'))
                           .arg(ls, 2, 10, QChar('0'))
                           .toStdString());
}

/*! \brief Extract sample rate and offset frequency from file name */
void CIqTool::parseFileName(const QString &filename)
{
    bool   sr_ok;
    qint64 sr;
    bool   center_ok;
    qint64 center;

    QString fmt_str = "";
    QStringList list = filename.split('_');

    if (list.size() < 5)
        return;

    // gqrx_yyyyMMdd_hhmmss_freq_samprate_fc.raw
    QDateTime ts = QDateTime::fromString(list.at(1) + list.at(2), "yyyyMMddhhmmss");
    ts.setOffsetFromUtc(0);
    time_ms = ts.toMSecsSinceEpoch();
    sr = list.at(4).toLongLong(&sr_ok);
    center = list.at(3).toLongLong(&center_ok);

    fmt_str = list.at(5);

    if (sr_ok)
        sample_rate = sr;
    if (center_ok)
        center_freq = center;
    fmt = FILE_FORMAT_CF;
    for(int k=FILE_FORMAT_CF;k<FILE_FORMAT_COUNT;k++)
        if(fmt_str.compare(any_to_any_base::fmt[k].suffix) == 0)
            fmt=file_formats(k);
    samples_per_chunk = any_to_any_base::fmt[fmt].nsamples;
    chunk_size = any_to_any_base::fmt[fmt].size;
}
