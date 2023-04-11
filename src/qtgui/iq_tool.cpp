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
#include <QTime>
#include <QScrollBar>
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
    fmt = receiver::FILE_FORMAT_CF;
    rec_fmt = receiver::FILE_FORMAT_CF;
    sample_rate = 192000;
    rec_len = 0;
    center_freq = 1e8;
    time_ms = 0;

    recdir = new QDir(QDir::homePath(), "*.raw");

    error_palette = new QPalette();
    error_palette->setColor(QPalette::Text, Qt::red);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timeoutFunction()));
    connect(ui->formatCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(on_formatCombo_currentIndexChanged(int)));
    ui->formatCombo->addItem("gr_complex cf", receiver::FILE_FORMAT_CF);
    ui->formatCombo->addItem("int 32", receiver::FILE_FORMAT_CS32L);
    ui->formatCombo->addItem("short 16", receiver::FILE_FORMAT_CS16L);
    ui->formatCombo->addItem("char 8", receiver::FILE_FORMAT_CS8);
    ui->formatCombo->addItem("uint 32", receiver::FILE_FORMAT_CS32LU);
    ui->formatCombo->addItem("ushort 16", receiver::FILE_FORMAT_CS16LU);
    ui->formatCombo->addItem("uchar 8", receiver::FILE_FORMAT_CS8U);
    ui->formatCombo->addItem("10 bit", receiver::FILE_FORMAT_CS10L);
    ui->formatCombo->addItem("12 bit", receiver::FILE_FORMAT_CS12L);
    ui->formatCombo->addItem("14 bit", receiver::FILE_FORMAT_CS14L);
    ui->bufferStats->hide();
    ui->sizeStats->hide();
    sliderMenu = new QMenu(this);
    // marker A
    {
        QAction* action = new QAction("A", this);
        sliderMenu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(sliderA()));
        QKeySequence seq(Qt::Key_BracketLeft);
        auto *shortcut = new QShortcut(seq, this);
        connect(shortcut, SIGNAL(activated()), this, SLOT(sliderA()));
        action->setShortcut(seq);
        setA=action;
        setA->setEnabled(false);
    }
    // marker B
    {
        QAction* action = new QAction("B", this);
        sliderMenu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(sliderB()));
        QKeySequence seq(Qt::Key_BracketRight);
        auto *shortcut = new QShortcut(seq, this);
        connect(shortcut, SIGNAL(activated()), this, SLOT(sliderB()));
        action->setShortcut(seq);
        setB=action;
        setB->setEnabled(false);
    }
    // Reset selection
    {
        QAction* action = new QAction("Reset", this);
        sliderMenu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(sliderReset()));
        selReset=action;
    }
    // Save selection
    {
        QAction* action = new QAction("Save", this);
        sliderMenu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(sliderSave()));
        QKeySequence seq(Qt::CTRL+Qt::Key_S);
        auto *shortcut = new QShortcut(seq, this);
        connect(shortcut, SIGNAL(activated()), this, SLOT(sliderSave()));
        action->setShortcut(seq);
        selSave=action;
        selSave->setEnabled(false);
    }
    // Go to marker A
    {
        QAction* action = new QAction("Go to A", this);
        sliderMenu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(sliderGoA()));
        QKeySequence seq(Qt::CTRL+Qt::Key_BracketLeft);
        auto *shortcut = new QShortcut(seq, this);
        connect(shortcut, SIGNAL(activated()), this, SLOT(sliderGoA()));
        action->setShortcut(seq);
        goA=action;
        goA->setEnabled(false);
    }
    // Go to marker B
    {
        QAction* action = new QAction("Go to B", this);
        sliderMenu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(sliderGoB()));
        QKeySequence seq(Qt::CTRL+Qt::Key_BracketRight);
        auto *shortcut = new QShortcut(seq, this);
        connect(shortcut, SIGNAL(activated()), this, SLOT(sliderGoB()));
        action->setShortcut(seq);
        goB=action;
        goB->setEnabled(false);
    }
    ui->slider->setContextMenuPolicy(Qt::CustomContextMenu);
}

CIqTool::~CIqTool()
{
    timer->stop();
    delete timer;
    delete ui;
    delete recdir;
    delete error_palette;

}

/*! \brief Set new sample rate. */
void CIqTool::setSampleRate(qint64 sr)
{
    sample_rate = sr;

    if (!current_file.isEmpty())
    {
        // Get duration of selected recording and update label
        QFileInfo info(*recdir, current_file);
        rec_len = (int)(info.size() * samples_per_chunk / (sample_rate * chunk_size));
        refreshTimeWidgets();
    }
}


/*! \brief Slot activated when the user selects a file. */
void CIqTool::on_listWidget_currentTextChanged(const QString &currentText)
{

    current_file = currentText;
    QFileInfo info(*recdir, current_file);

    parseFileName(currentText);
    rec_len = (int)(info.size() * samples_per_chunk / (sample_rate * chunk_size));

    // Get duration of selected recording and update label
    refreshTimeWidgets();

}

/*! \brief Show/hide/enable/disable GUI controls */

void CIqTool::switchControlsState(bool recording, bool playback)
{
    ui->recButton->setEnabled(!playback);

    ui->playButton->setEnabled(!recording);
    ui->slider->setEnabled(!recording);

    ui->repeat->setEnabled(!(recording || playback));
    ui->listWidget->setEnabled(!(recording || playback));
    ui->recDirEdit->setEnabled(!(recording || playback));
    ui->recDirButton->setEnabled(!(recording || playback));
    ui->formatCombo->setEnabled(!(recording || playback));
    ui->repeat->setEnabled(!(recording || playback));
    ui->buffersSpinBox->setEnabled(!(recording || playback));
    setA->setEnabled(playback);
    setB->setEnabled(playback);
    if (recording || playback)
    {
        ui->formatLabel->hide();
        ui->buffersLabel->hide();
        ui->formatCombo->hide();
        ui->buffersSpinBox->hide();
        ui->bufferStats->show();
        ui->sizeStats->show();
    }
    else
    {
        ui->formatLabel->show();
        ui->buffersLabel->show();
        ui->formatCombo->show();
        ui->buffersSpinBox->show();
        ui->bufferStats->hide();
        ui->sizeStats->hide();
    }
}

/*! \brief Start/stop playback */
void CIqTool::on_playButton_clicked(bool checked)
{
    is_playing = checked;

    if (checked)
    {
        if (current_file.isEmpty())
        {
            QMessageBox msg_box;
            msg_box.setIcon(QMessageBox::Critical);
            if (ui->listWidget->count() == 0)
            {
                msg_box.setText(tr("There are no I/Q files in the current directory."));
            }
            else
            {
                msg_box.setText(tr("Please select a file to play."));
            }
            msg_box.exec();

            ui->playButton->setChecked(false); // will not trig clicked()
        }
        else
        {
            sel_A=sel_B=-1.0;
            updateSliderStylesheet();
            on_listWidget_currentTextChanged(current_file);
            switchControlsState(false, true);

            emit startPlayback(recdir->absoluteFilePath(current_file),
                               (float)sample_rate, center_freq, fmt,
                               time_ms, ui->buffersSpinBox->value(),
                               ui->repeat->checkState() == Qt::Checked);
        }
    }
    else
    {
        emit stopPlayback();
        switchControlsState(false, false);
        ui->slider->setValue(0);
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
void CIqTool::on_slider_valueChanged(int value)
{
    refreshTimeWidgets();

    qint64 seek_pos = (qint64)(value) * sample_rate / samples_per_chunk;
    emit seek(seek_pos);
    o_fileSize = seek_pos;
}


/*! \brief Start/stop recording */
void CIqTool::on_recButton_clicked(bool checked)
{
    is_recording = checked;

    if (checked)
    {
        switchControlsState(true, false);
        emit startRecording(recdir->path(), rec_fmt, ui->buffersSpinBox->value());

        refreshDir();
        ui->listWidget->setCurrentRow(ui->listWidget->count()-1);
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
    ui->recButton->setChecked(false);
    on_recButton_clicked(false);
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
            if(o_buffersUsed!=buffersUsed)
                ui->bufferStats->setText(QString("Buffer: %1%").arg(buffersUsed));
            if(o_fileSize != fileSize)
                ui->sizeStats->setText(QString("Size: %1 bytes").arg(fileSize * chunk_size));
            o_buffersUsed = buffersUsed;
            o_fileSize = fileSize;
        }
    }
    if(is_playing)
    {
        if(o_buffersUsed!=buffersUsed)
            ui->bufferStats->setText(QString("Buffer: %1%").arg(buffersUsed));
        if(o_fileSize != fileSize)
            ui->sizeStats->setText(QString("Pos: %1 bytes").arg(fileSize * chunk_size));
        o_buffersUsed = buffersUsed;
        o_fileSize = fileSize;
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
    refreshTimeWidgets();
    timer->start(1000);
}


void CIqTool::saveSettings(QSettings *settings)
{
    if (!settings)
        return;

    // Location of baseband recordings
    QString dir = recdir->path();
    if (dir != QDir::homePath())
        settings->setValue("baseband/rec_dir", dir);
    else
        settings->remove("baseband/rec_dir");
    settings->setValue("baseband/rec_fmt", rec_fmt);
    settings->setValue("baseband/rec_buffers", ui->buffersSpinBox->value());
}

void CIqTool::readSettings(QSettings *settings)
{
    if (!settings)
        return;

    // Location of baseband recordings
    QString dir = settings->value("baseband/rec_dir", QDir::homePath()).toString();
    ui->recDirEdit->setText(dir);
    int found = ui->formatCombo->findData(settings->value("baseband/rec_fmt", receiver::FILE_FORMAT_CF));
    if(found == -1)
    {
        rec_fmt = receiver::FILE_FORMAT_CF;
    }
    else
    {
        ui->formatCombo->setCurrentIndex(found);
    }
    ui->buffersSpinBox->setValue(settings->value("baseband/rec_buffers", 1).toInt());
}



/*! \brief Slot called when the recordings directory has changed either
 *         because of user input or programmatically.
 */
void CIqTool::on_recDirEdit_textChanged(const QString &dir)
{
    if (recdir->exists(dir))
    {
        ui->recDirEdit->setPalette(QPalette());  // Clear custom color
        recdir->setPath(dir);
        recdir->cd(dir);
        //emit newRecDirSelected(dir);
    }
    else
    {
        ui->recDirEdit->setPalette(*error_palette);  // indicate error
    }
}

/*! \brief Slot called when the user clicks on the "Select" button. */
void CIqTool::on_recDirButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select a directory"),
                                                    ui->recDirEdit->text(),
                                                    QFileDialog::ShowDirsOnly |
                                                    QFileDialog::DontResolveSymlinks);

    if (!dir.isNull())
        ui->recDirEdit->setText(dir);
}

void CIqTool::timeoutFunction(void)
{
    refreshDir();

    if (is_playing)
    {
        int val = o_fileSize * samples_per_chunk / sample_rate ;
        if (val < ui->slider->maximum())
        {
            ui->slider->blockSignals(true);
            ui->slider->setValue(val);
            ui->slider->blockSignals(false);
            refreshTimeWidgets();
        }
    }
    if (is_recording)
        refreshTimeWidgets();
}

void CIqTool::on_slider_customContextMenuRequested(const QPoint& pos)
{
    sliderMenu->popup(ui->slider->mapToGlobal(pos));
}

void CIqTool::on_formatCombo_currentIndexChanged(int index)
{
    rec_fmt = (enum receiver::file_formats)ui->formatCombo->currentData().toInt();
}

void CIqTool::updateSliderStylesheet(qint64 save_progress)
{
    if((sel_A<0.0)&&(sel_B<0.0))
    {
        ui->slider->setStyleSheet("");
        setA->setText("A");
        setB->setText("B");
        selSave->setText("Save");
        selSave->setEnabled(false);
        goA->setEnabled(false);
        goB->setEnabled(false);
        setA->setEnabled(true);
        setB->setEnabled(true);
        selReset->setEnabled(true);
        return;
    }
    goA->setEnabled(true);
    goB->setEnabled(true);
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
    setA->setText(QString("A: %1 s").arg(quint64(sel_A * double(rec_len))));
    setB->setText(QString("B: %1 s").arg(quint64(sel_B * double(rec_len))));
    selSave->setText(QString("Save: %1 - %2 s")
        .arg(quint64(sel_A * double(rec_len)))
        .arg(quint64(sel_B * double(rec_len)))
    );
    double selLen=sel_B-sel_A;
    if(selLen<0.00001)
        selLen=0.00001;
    selLen+=sel_A;
    if(selLen>1.0)
        selLen=1.0;
    if(save_progress == -1)
    {
        setA->setEnabled(true);
        setB->setEnabled(true);
        selSave->setEnabled(true);
        selReset->setEnabled(true);
    }
    if(save_progress>0)
    {
        double prgLen = sel_A + double(save_progress)/(double(rec_len)*1000.0);
        if(prgLen>1.0)
            prgLen=1.0;
        if(prgLen<0.000007)
            prgLen=0.000007;
        ui->slider->setStyleSheet(QString(
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
        ui->slider->setStyleSheet(QString(
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
        selSave->setEnabled(true);
    }
}


void CIqTool::sliderA()
{
    if(is_saving)
        return;
    sel_A=double(o_fileSize * samples_per_chunk)/double(rec_len * sample_rate);
    updateSliderStylesheet();
}

void CIqTool::sliderB()
{
    if(is_saving)
        return;
    sel_B=double(o_fileSize * samples_per_chunk)/double(rec_len * sample_rate);
    updateSliderStylesheet();
}

void CIqTool::sliderGoA()
{
    ui->slider->setValue(sel_A*ui->slider->maximum());
}

void CIqTool::sliderGoB()
{
    ui->slider->setValue(sel_B*ui->slider->maximum());
}

void CIqTool::sliderReset()
{
    if(is_saving)
        return;
    sel_A=sel_B=-1.0;
    updateSliderStylesheet();
}

void CIqTool::sliderSave()
{
    if(sel_A<0.0)
        return;
    is_saving=true;
    quint64 from_ms=sel_A*double(rec_len)*1000.0+time_ms;
    quint64 len_ms=(sel_B-sel_A)*double(rec_len)*1000.0;
    if(len_ms<1.0)
        len_ms=1.0;
    emit saveFileRange(recdir->path(), fmt, from_ms, len_ms);
    updateSliderStylesheet(0);
    setA->setEnabled(false);
    setB->setEnabled(false);
    selSave->setEnabled(false);
    selReset->setEnabled(false);
}

qint64 CIqTool::selectionLength()
{
    return (sel_B-sel_A)*double(rec_len)*1000.0;
}


/*! \brief Refresh list of files in current working directory. */
void CIqTool::refreshDir()
{
    int selection = ui->listWidget->currentRow();

    QScrollBar * sc = ui->listWidget->verticalScrollBar();
    int lastScroll = sc->sliderPosition();
    recdir->refresh();
    QStringList files = recdir->entryList();

    ui->listWidget->blockSignals(true);
    ui->listWidget->clear();
    ui->listWidget->insertItems(0, files);
    ui->listWidget->setCurrentRow(selection);
    sc->setSliderPosition(lastScroll);
    ui->listWidget->blockSignals(false);

    if (is_recording)
    {
        // update rec_len; if the file being recorded is the one selected
        // in the list, the length will update periodically
        QFileInfo info(*recdir, current_file);
        rec_len = (int)(info.size() * samples_per_chunk / (sample_rate * chunk_size));
    }
}

/*! \brief Refresh time labels and slider position
 *
 * \note Safe for recordings > 24 hours
 */
void CIqTool::refreshTimeWidgets(void)
{
    ui->slider->setMaximum(rec_len);

    // duration
    int len = rec_len;
    int lh, lm, ls;
    lh = len / 3600;
    len = len % 3600;
    lm = len / 60;
    ls = len % 60;

    // current position
    int pos = ui->slider->value();
    int ph, pm, ps;
    ph = pos / 3600;
    pos = pos % 3600;
    pm = pos / 60;
    ps = pos % 60;

    ui->timeLabel->setText(QString("%1:%2:%3 / %4:%5:%6")
                           .arg(ph, 2, 10, QChar('0'))
                           .arg(pm, 2, 10, QChar('0'))
                           .arg(ps, 2, 10, QChar('0'))
                           .arg(lh, 2, 10, QChar('0'))
                           .arg(lm, 2, 10, QChar('0'))
                           .arg(ls, 2, 10, QChar('0')));
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
    time_ms = ts.toMSecsSinceEpoch();
    sr = list.at(4).toLongLong(&sr_ok);
    center = list.at(3).toLongLong(&center_ok);

    fmt_str = list.at(5);
    list = fmt_str.split('.');
    fmt_str = list.at(0);

    if (sr_ok)
        sample_rate = sr;
    if (center_ok)
        center_freq = center;
    if(fmt_str.compare("fc") == 0)
        fmt = receiver::FILE_FORMAT_CF;
    if(fmt_str.compare("32") == 0)
        fmt = receiver::FILE_FORMAT_CS32L;
    if(fmt_str.compare("16") == 0)
        fmt = receiver::FILE_FORMAT_CS16L;
    if(fmt_str.compare("14") == 0)
        fmt = receiver::FILE_FORMAT_CS14L;
    if(fmt_str.compare("12") == 0)
        fmt = receiver::FILE_FORMAT_CS12L;
    if(fmt_str.compare("10") == 0)
        fmt = receiver::FILE_FORMAT_CS10L;
    if(fmt_str.compare("8") == 0)
        fmt = receiver::FILE_FORMAT_CS8;
    if(fmt_str.compare("32u") == 0)
        fmt = receiver::FILE_FORMAT_CS32LU;
    if(fmt_str.compare("16u") == 0)
        fmt = receiver::FILE_FORMAT_CS16LU;
    if(fmt_str.compare("8u") == 0)
        fmt = receiver::FILE_FORMAT_CS8U;
    samples_per_chunk = receiver::samples_per_chunk[fmt];
    chunk_size = receiver::chunk_size[fmt];
}
