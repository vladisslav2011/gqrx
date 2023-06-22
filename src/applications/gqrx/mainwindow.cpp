/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2014 Alexandru Csete OZ9AEC.
 * Copyright (C) 2013 by Elias Oenal <EliasOenal@gmail.com>
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
#include <string>
#include <vector>
#include <volk/volk.h>

#include <QSettings>
#include <QByteArray>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFile>
#include <QGroupBox>
#include <QJsonDocument>
#include <QKeySequence>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QResource>
#include <QShortcut>
#include <QString>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTextStream>
#include <QtGlobal>
#include <QTimer>
#include <QVBoxLayout>
#include <QSvgWidget>
#include "qtgui/ioconfig.h"
#include "mainwindow.h"
#include "qtgui/dxc_options.h"
#include "qtgui/dxc_spots.h"
#include <chrono>
using namespace std::chrono_literals;

/* Qt Designer files */
#include "ui_mainwindow.h"

/* DSP */
#include "receiver.h"
#include "remote_control_settings.h"

#include "qtgui/bookmarkstaglist.h"
#include "qtgui/bandplan.h"

Q_DECLARE_METATYPE(c_id)
Q_DECLARE_METATYPE(c_def::v_union)
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

MainWindow::MainWindow(const QString& cfgfile, bool edit_conf, QWidget *parent) :
    QMainWindow(parent),
    configOk(true),
    ui(new Ui::MainWindow),
    d_lnb_lo(0),
    d_hw_freq(0),
    d_ignore_limits(false),
    d_auto_bookmarks(false),
    d_fftAvg(1.0),
    d_have_audio(true),
    dec_afsk1200(nullptr),
    waterfall_background_thread(&MainWindow::waterfall_background_func,this)
{
    auto t1 = high_resolution_clock::now();
    qRegisterMetaType<c_id>();
    qRegisterMetaType<c_def::v_union>();

    ui->setupUi(this);

    /* Initialise default configuration directory */
    QByteArray xdg_dir = qgetenv("XDG_CONFIG_HOME");
    if (xdg_dir.isEmpty())
    {
        // Qt takes care of conversion to native separators
        m_cfg_dir = QString("%1/.config/gqrx").arg(QDir::homePath());
    }
    else
    {
        m_cfg_dir = QString("%1/gqrx").arg(xdg_dir.data());
    }

    setWindowTitle(QString("Gqrx %1").arg(VERSION));

    /* frequency control widget */
    ui->freqCtrl->setup(0, 0, 9999e6, 1, FCTL_UNIT_NONE);
    ui->freqCtrl->setFrequency(144500000);

    /* create receiver object */
    rx = new receiver("", "", 1);
    rx->set_rf_freq(144500000.0);

    // remote controller
    remote = new RemoteControl();

    /* meter timer */
    meter_timer = new QTimer(this);
    connect(meter_timer, SIGNAL(timeout()), this, SLOT(meterTimeout()));

    /* FFT timer & data */
    iq_fft_timer = new QTimer(this);
    connect(iq_fft_timer, SIGNAL(timeout()), this, SLOT(iqFftTimeout()));

    audio_fft_timer = new QTimer(this);
    connect(audio_fft_timer, SIGNAL(timeout()), this, SLOT(audioFftTimeout()));

    d_fftData = new std::complex<float>[MAX_FFT_SIZE];
    d_realFftData = new float[MAX_FFT_SIZE];
    d_iirFftData = new float[MAX_FFT_SIZE];
    for (int i = 0; i < MAX_FFT_SIZE; i++)
        d_iirFftData[i] = -140.0;  // dBFS

    /* timer for data decoders */
    dec_timer = new QTimer(this);
    connect(dec_timer, SIGNAL(timeout()), this, SLOT(decoderTimeout()));

    // create I/Q tool widget
    iq_tool = new CIqTool(this);

    // create DXC Objects
    dxc_options = new DXCOptions(this);
    dxc_timer = new QTimer(this);
    dxc_timer->start(1000);

    /* create dock widgets */
    uiDockRxOpt = new DockRxOpt();
    uiDockRDS = new DockRDS();
    uiDockProbe = new DockProbe();
    uiDockAudio = new DockAudio();
    uiDockInputCtl = new DockInputCtl();
    uiDockFft = new DockFft();
    BandPlan::Get().setConfigDir(m_cfg_dir);
    Bookmarks::Get().setConfigDir(m_cfg_dir);
    BandPlan::Get().load();
    uiDockBookmarks = new DockBookmarks(this);

    // setup some toggle view shortcuts
    uiDockInputCtl->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_J));
    uiDockRxOpt->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    uiDockFft->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F));
    uiDockAudio->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_A));
    uiDockProbe->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    uiDockBookmarks->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));
    ui->mainToolBar->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T));

    /* frequency setting shortcut */
    auto *freq_shortcut = new QShortcut(QKeySequence(Qt::Key_F), this);
    QObject::connect(freq_shortcut, &QShortcut::activated, this, &MainWindow::frequencyFocusShortcut);

    // zero cursor (rx filter offset)
    auto *rx_offset_zero_shortcut = new QShortcut(QKeySequence(Qt::Key_Z), this);
    QObject::connect(rx_offset_zero_shortcut, &QShortcut::activated, this, &MainWindow::rxOffsetZeroShortcut);

    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    /* Add dock widgets to main window. This should be done even for
       dock widgets that are going to be hidden, otherwise they will
       end up floating in their own top-level window and can not be
       docked to the mainwindow.
    */
    addDockWidget(Qt::RightDockWidgetArea, uiDockInputCtl);
    addDockWidget(Qt::RightDockWidgetArea, uiDockRxOpt);
    addDockWidget(Qt::RightDockWidgetArea, uiDockFft);
    tabifyDockWidget(uiDockInputCtl, uiDockRxOpt);
    tabifyDockWidget(uiDockRxOpt, uiDockFft);
    uiDockRxOpt->raise();

    addDockWidget(Qt::RightDockWidgetArea, uiDockAudio);
    addDockWidget(Qt::RightDockWidgetArea, uiDockProbe);
    addDockWidget(Qt::RightDockWidgetArea, uiDockRDS);
    tabifyDockWidget(uiDockAudio, uiDockRDS);
    tabifyDockWidget(uiDockRDS, uiDockProbe);
    uiDockAudio->raise();

    addDockWidget(Qt::BottomDockWidgetArea, uiDockBookmarks);

    /* hide docks that we don't want to show initially */
    uiDockBookmarks->hide();
    uiDockRDS->hide();

    /* Add dock widget actions to View menu. By doing it this way all signal/slot
       connections will be established automagially.
    */
    ui->menu_View->addAction(uiDockInputCtl->toggleViewAction());
    ui->menu_View->addAction(uiDockRxOpt->toggleViewAction());
    ui->menu_View->addAction(uiDockRDS->toggleViewAction());
    ui->menu_View->addAction(uiDockAudio->toggleViewAction());
    ui->menu_View->addAction(uiDockFft->toggleViewAction());
    ui->menu_View->addAction(uiDockBookmarks->toggleViewAction());
    ui->menu_View->addAction(uiDockProbe->toggleViewAction());
    ui->menu_View->addSeparator();
    ui->menu_View->addAction(ui->mainToolBar->toggleViewAction());
    ui->menu_View->addSeparator();
    ui->menu_View->addAction(ui->actionFullScreen);

    docks.resize(D_COUNT);
    docks[D_INPUTCTL]=uiDockInputCtl;
    docks[D_RXOPT]=uiDockRxOpt;
    docks[D_AUDIO]=uiDockAudio;
    docks[D_FFT]=uiDockFft;
    docks[D_RDS]=uiDockRDS;
    docks[D_IQTOOL]=iq_tool;
    docks[D_DXC]=dxc_options;
    docks[D_PROBE]=uiDockProbe;
    /*
    uiDockBookmarks,//D_BOOKMARKS
    */
    try{
        for(int k=0;k<C_COUNT;k++)
        {
            auto defs=c_def::all();
            if(k != defs[k].idx())
            {
                std::cerr<<"c_def: invalid position: expected idx="<<k<<" actual idx="<<defs[k].idx()<<" name="<<defs[k].name()<<
                    " title="<<defs[k].title()<<"\n";
                exit(1);
            }
            docks[defs[k].dock()]->add_control(c_id(k));
            if(defs[k].event())
            {
                if(defs[k].scope()==S_VFO)
                    rx->register_observer(c_id(k),[=](const int vfo, const c_def::v_union & value)
                    {
                        if(rx->get_current()==vfo)
                            emit observer_signal(c_id(k),value);
                    });
                else
                    rx->register_observer(c_id(k),[=](const int vfo, const c_def::v_union & value)
                    {
                        emit observer_signal(c_id(k),value);
                    });
            }
        }
    } catch (std::runtime_error & e)
    {
        std::cerr<<"Caught runtime error during GUI initialization: "<<e.what()<<"\n";
        exit(1);
    }
    //add spacers
    for(auto it=docks.begin();it!=docks.end();++it)
        if(*it)
            (*it)->finalize();

    #ifndef ENABLE_RNNOISE
        getWidget(C_NB3_ON)->setDisabled(true);
        getWidget(C_NB3_GAIN)->setDisabled(true);
    #endif
    set_observer(C_RDS_ON, &MainWindow::rdsOnObserver);
    set_observer(C_RDS_PI, &MainWindow::rdsPIObserver);
    set_observer(C_RDS_PS, &MainWindow::rdsPSObserver);
    set_observer(C_RDS_RADIOTEXT, &MainWindow::rdsRTObserver);
    set_observer(C_AGC_ON, &MainWindow::agcOnObserver);
    set_observer(C_AGC_MAN_GAIN, &MainWindow::agcManualGainObserver);
    set_observer(C_AUDIO_REC_COPY, &MainWindow::audioRecSettingsCopyObserver);
    set_observer(C_AUDIO_REC, &MainWindow::audioRecObserver);
    set_observer(C_AUDIO_REC_FILENAME, &MainWindow::audioRecFilenameObserver);
    set_observer(C_SQUELCH_LEVEL, &MainWindow::sqlLevelObserver);
    set_observer(C_MODE, &MainWindow::modeObserver);
    set_observer(C_MODE_CHANGED, &MainWindow::modeChangeObserver);
    set_observer(C_FILTER_WIDTH, &MainWindow::filterWidthObserver);
    set_observer(C_VFO_FREQUENCY, &MainWindow::frequencyObserver);
    set_observer(C_AUTO_BOOKMARKS, &MainWindow::autoBookmarksObserver);
    set_observer(C_DIGITS_RESET, &MainWindow::freqCtrlResetObserver);
    set_observer(C_WHEEL_INVERT, &MainWindow::invertScrollingObserver);
    set_observer(C_IGNORE_LIMITS, &MainWindow::ignoreLimitsObserver);
    set_observer(C_LNB_LO, &MainWindow::lnbLoObserver);
    set_observer(C_ENABLE_BANDPLAN, &MainWindow::bandPlanObserver);
    set_observer(C_WF_BG_THREADS, &MainWindow::wfBgThreadsObserver);
    set_observer(C_WF_COLORMAP, &MainWindow::wfColormapObserver);
    set_observer(C_PLOT_COLOR, &MainWindow::fftColorObserver);
    set_observer(C_PLOT_FILL, &MainWindow::fftFillObserver);
    set_observer(C_PLOT_RESET, &MainWindow::plotResetObserver);
    set_observer(C_PLOT_CENTER, &MainWindow::plotCenterObserver);
    set_observer(C_PLOT_DEMOD, &MainWindow::plotDemodObserver);
    set_observer(C_PLOT_ZOOM, &MainWindow::fftZoomLevelObserver);
    set_observer(C_FFT_PAND_MIN_DB, &MainWindow::fftMinDbObserver);
    set_observer(C_FFT_PAND_MAX_DB, &MainWindow::fftMaxDbObserver);
    set_observer(C_FFT_WF_MIN_DB, &MainWindow::fftMinDbObserver);
    set_observer(C_FFT_WF_MAX_DB, &MainWindow::fftMaxDbObserver);
    set_observer(C_FFT_RANGE_LOCKED, &MainWindow::fftLockObserver);
    set_observer(C_FFT_PEAK_DETECT, &MainWindow::peakDetectionObserver);
    set_observer(C_FFT_PEAK_HOLD, &MainWindow::fftPeakHoldObserver);
    set_observer(C_FFT_SPLIT, &MainWindow::iqFftSplitObserver);
    set_observer(C_FFT_AVG, &MainWindow::iqFftAvgObserver);
    set_observer(C_FFT_WINDOW, &MainWindow::iqFftWindowObserver);
    set_observer(C_FFT_WINDOW_CORR, &MainWindow::iqFftWindowCorrectionObserver);
    set_observer(C_FFT_TIMESPAN, &MainWindow::wfTimeSpanObserver);
    set_observer(C_FFT_RATE, &MainWindow::iqFftRateObserver);
    set_observer(C_FFT_SIZE, &MainWindow::iqFftSizeObserver);

    /* Setup demodulator switching SpinBox */
    rxSpinBox = new QSpinBox(ui->mainToolBar);
    rxSpinBox->setMaximum(255);
    rxSpinBox->setValue(0);
    ui->mainToolBar->insertWidget(ui->actionAddDemodulator, rxSpinBox);

    /* connect signals and slots */
    connect(rxSpinBox, SIGNAL(valueChanged(int)), this, SLOT(rxSpinBox_valueChanged(int)));
    connect(ui->freqCtrl, SIGNAL(newFrequency(qint64)), this, SLOT(setNewFrequency(qint64)));
    connect(uiDockInputCtl, SIGNAL(gainChanged(QString, double)), this, SLOT(setGain(QString,double)));
    connect(uiDockInputCtl, SIGNAL(gainChanged(QString, double)), remote, SLOT(setGain(QString,double)));
    connect(uiDockRxOpt, SIGNAL(filterOffsetChanged(qint64)), this, SLOT(setFilterOffset(qint64)));
    connect(uiDockRxOpt, SIGNAL(filterOffsetChanged(qint64)), remote, SLOT(setFilterOffset(qint64)));
    connect(uiDockAudio, SIGNAL(fftRateChanged(int)), this, SLOT(setAudioFftRate(int)));
    connect(uiDockAudio, SIGNAL(visibilityChanged(bool)), this, SLOT(dockAudioVisibilityChanged(bool)));

    connect(ui->plotter, SIGNAL(newSize()), this, SLOT(setWfSize()));
    connect(ui->plotter, SIGNAL(newFftCenterFreq(qint64)), this, SLOT(setFftCenterFreq(qint64)));

    // Bookmarks
    connect(uiDockBookmarks, SIGNAL(newBookmarkActivated(BookmarkInfo &)), this, SLOT(onBookmarkActivated(BookmarkInfo &)));
    //FIXME: create a new slot that would avoid changing hw frequency if the bookmark is in the current bandwidth
    connect(uiDockBookmarks, SIGNAL(newBookmarkActivated(qint64)), this, SLOT(setNewFrequency(qint64)));
    connect(uiDockBookmarks, SIGNAL(newBookmarkActivatedAddDemod(BookmarkInfo &)), this, SLOT(onBookmarkActivatedAddDemod(BookmarkInfo &)));
    connect(uiDockBookmarks->actionAddBookmark, SIGNAL(triggered()), this, SLOT(on_actionAddBookmark_triggered()));
    connect(&Bookmarks::Get(), SIGNAL(BookmarksChanged()), ui->plotter, SLOT(updateOverlay()));

    //DXC Spots
    connect(&DXCSpots::Get(), SIGNAL(dxcSpotsChanged()),this , SLOT(addClusterSpot()));
    connect(dxc_timer, SIGNAL(timeout()), this, SLOT(checkDXCSpotTimeout()));

    // I/Q playback
    connect(iq_tool, SIGNAL(startRecording(QString, file_formats)), this, SLOT(startIqRecording(QString, file_formats)));
    connect(iq_tool, SIGNAL(stopRecording()), this, SLOT(stopIqRecording()));
    connect(iq_tool, SIGNAL(startPlayback(QString, float, qint64, file_formats, qint64)),
                 this, SLOT(startIqPlayback(QString, float, qint64, file_formats, qint64)));
    connect(iq_tool, SIGNAL(stopPlayback()), this, SLOT(stopIqPlayback()));
    connect(iq_tool, SIGNAL(seek(qint64)), this,SLOT(seekIqFile(qint64)));
    connect(iq_tool, SIGNAL(saveFileRange(const QString &, file_formats, quint64,quint64)), this,SLOT(saveFileRange(const QString &, file_formats, quint64,quint64)));

    // remote control
    connect(remote, SIGNAL(newRDSmode(bool)), this, SLOT(setRDSmode(bool)));
    connect(remote, SIGNAL(newFilterOffset(qint64)), this, SLOT(setFilterOffset(qint64)));
    connect(remote, SIGNAL(newFilterOffset(qint64)), uiDockRxOpt, SLOT(setFilterOffset(qint64)));
    connect(remote, SIGNAL(newFrequency(qint64)), this, SLOT(setNewFrequency(qint64)));
    connect(remote, &RemoteControl::newLnbLo, [=](double v)
        {
            set_gui(C_LNB_LO,int64_t(v*1e6),true);
            changed_gui(C_LNB_LO,int64_t(v*1e6));
        });
    connect(remote, &RemoteControl::newMode, [=](Modulations::idx v)
        {
            set_gui(C_MODE, int(v), true);
            changed_gui(C_MODE, int(v));
        });
    connect(remote, &RemoteControl::newAudioGain, [=](float v)
        {
            set_gui(C_AGC_MAN_GAIN, v, true);
            changed_gui(C_AGC_MAN_GAIN, v);
        });
    connect(remote, &RemoteControl::newSquelchLevel, [=](double v)
        {
            set_gui(C_SQUELCH_LEVEL, v, true);
            changed_gui(C_SQUELCH_LEVEL, v);
        });
    connect(remote, &RemoteControl::startAudioRecorderEvent, [=]()
        {
            set_gui(C_AUDIO_REC, true, true);
            changed_gui(C_AUDIO_REC, true);
        });
    connect(remote, &RemoteControl::stopAudioRecorderEvent, [=]()
        {
            set_gui(C_AUDIO_REC, false, true);
            changed_gui(C_AUDIO_REC, false);
        });
    connect(remote, SIGNAL(newPassband(int)), this, SLOT(setPassband(int)));
    connect(remote, SIGNAL(gainChanged(QString, double)), uiDockInputCtl, SLOT(setGain(QString,double)));
    connect(remote, SIGNAL(dspChanged(bool)), this, SLOT(on_actionDSP_triggered(bool)));

    // enable frequency tooltips on FFT plot
    ui->plotter->setTooltipsEnabled(true);


    // Create list of input devices. This must be done before the configuration is
    // restored because device probing might change the device configuration
    auto t2 = high_resolution_clock::now();
    CIoConfig::getDeviceList(devList);
    auto t3 = high_resolution_clock::now();

    m_recent_config = new RecentConfig(m_cfg_dir, ui->menu_RecentConfig);
    connect(m_recent_config, SIGNAL(loadConfig(const QString &)), this, SLOT(loadConfigSlot(const QString &)));
    connect(this,SIGNAL(observer_signal(const c_id, const c_def::v_union)), this, SLOT(observer_slot(const c_id, const c_def::v_union)), Qt::QueuedConnection);

    // restore last session
    if (!loadConfig(cfgfile, true, true))
    {

      // first time config
        qDebug() << "Launching I/O device editor";
        if (firstTimeConfig() != QDialog::Accepted)
        {
            qDebug() << "I/O device configuration cancelled.";
            configOk = false;
        }
        else
        {
            configOk = true;
        }
    }
    else if (edit_conf)
    {
        qDebug() << "Launching I/O device editor";
        if (on_actionIoConfig_triggered() != QDialog::Accepted)
        {
            qDebug() << "I/O device configuration cancelled.";
            configOk = false;
        }
        else
        {
            configOk = true;
        }
    }

    qsvg_dummy = new QSvgWidget();
    connect(this,SIGNAL(requestPlotterUpdate()), this, SLOT(plotterUpdate()), Qt::QueuedConnection);
    connect(this,SIGNAL(sigSaveProgress(const qint64)), this, SLOT(updateSaveProgress(const qint64)), Qt::QueuedConnection);
    rx->set_iq_save_progress_cb([=](int64_t saved_ms)
        {
            emit sigSaveProgress(saved_ms);
        });
    auto t4 = high_resolution_clock::now();
    duration<double, std::milli> diff = (t2 - t1) + (t4 - t3);
    std::cout << "*********************************\n"
                 "* started in "<<diff.count()<<" ms\n"
                 "*********************************\n";
}

MainWindow::~MainWindow()
{
    /* It is better to stop background thread first */
    {
        std::unique_lock<std::mutex> lock(waterfall_background_mutex);
        waterfall_background_request = MainWindow::WF_EXIT;
        waterfall_background_wake.notify_one();
    }
    waterfall_background_thread.join();
    on_actionDSP_triggered(false);

    /* stop and delete timers */
    dec_timer->stop();
    delete dec_timer;

    meter_timer->stop();
    delete meter_timer;

    iq_fft_timer->stop();
    delete iq_fft_timer;

    audio_fft_timer->stop();
    delete audio_fft_timer;

    dxc_timer->stop();
    delete dxc_timer;

    if (m_settings)
    {
        m_settings->setValue("configversion", 3);
        m_settings->setValue("crashed", false);

        // hide toolbar (default=false)
        if (ui->mainToolBar->isHidden())
            m_settings->setValue("gui/hide_toolbar", true);
        else
            m_settings->remove("gui/hide_toolbar");

        m_settings->setValue("gui/geometry", saveGeometry());
        m_settings->setValue("gui/state", saveState());

        // save session
        storeSession();

        m_settings->sync();
        delete m_settings;
    }

    delete m_recent_config;

    delete rx;
    delete iq_tool;
    delete dxc_options;
    delete ui;
    delete uiDockRxOpt;
    delete uiDockAudio;
    delete uiDockBookmarks;
    delete uiDockFft;
    delete uiDockInputCtl;
    delete uiDockProbe;
    delete uiDockRDS;
    delete remote;
    delete [] d_fftData;
    delete [] d_realFftData;
    delete [] d_iirFftData;
    delete qsvg_dummy;
    delete rxSpinBox;
}

void MainWindow::add_control(const c_id id)
{
}

void MainWindow::observer_slot(const c_id id, const c_def::v_union value)
{
    set_gui(id, value, true);
}

//user action
void MainWindow::changed_gui(const c_id id, const c_def::v_union & value, bool trigger_observers)
{
    if(observers[id] && trigger_observers)
        observers[id](id, value);
    if(c_def::all()[id].scope()!=S_GUI)
        rx->set_value(id, value);
}

//state update
void MainWindow::set_gui(const c_id id, const c_def::v_union & value, bool trigger_observers)
{
    docks[c_def::all()[id].dock()]->set_gui(id, value, trigger_observers);
    if(observers[id] && trigger_observers)
        observers[id](id, value);
}

//current state
void MainWindow::get_gui(const c_id id, c_def::v_union & value) const
{
    docks[c_def::all()[id].dock()]->get_gui(id, value);
}

static void loadSetting(QPointer<QSettings> m_settings, const c_def & def, c_def::v_union & v)
{
    const auto vkey = QString::fromStdString(def.config_key());
    bool conv_ok = false;
    if(def.presets().size()>0)
    {
        //may be stored as preset key
        QString qdef("");
        {
            auto it=def.ipresets().find(def.def());
            if(it!=def.ipresets().end())
                qdef=QString::fromStdString(def.presets()[it->second].key);
        }
        std::string ss = m_settings->value(vkey,qdef).toString().toStdString();
        auto it=def.kpresets().find(ss);
        if(it!=def.kpresets().end())
        {
            v=def.presets()[it->second].value;
            return;
        }
    }
    switch(def.v_type())
    {
    case V_INT:
        v=m_settings->value(vkey,qlonglong(def.def())).toLongLong(&conv_ok);
        if(!conv_ok)
            v=def.def();
    break;
    case V_DOUBLE:
        v=m_settings->value(vkey,double(def.def())).toDouble(&conv_ok);
        if(!conv_ok)
            v=def.def();
    break;
    case V_STRING:
        v=m_settings->value(vkey,QString::fromStdString(def.def())).toString().toStdString();
    break;
    case V_BOOLEAN:
        v=m_settings->value(vkey,bool(def.def())).toBool();
    break;
    }
    if(def.clip(v))
        std::cerr<<"Clipped "<<def.name()<<" "<<v.to_string()<<"\n";
}

/**
 * Load new configuration.
 * @param cfgfile
 * @returns True if config is OK, False if not (e.g. no input device specified).
 *
 * If cfgfile is an absolute path it will be used as is, otherwise it is assumed
 * to be the name of a file under m_cfg_dir.
 *
 * If cfgfile does not exist it will be created.
 *
 * If no input device is specified, we return false to signal that the I/O
 * configuration dialog should be run.
 *
 * FIXME: Refactor.
 */
bool MainWindow::loadConfig(const QString& cfgfile, bool check_crash,
                            bool restore_mainwindow)
{
    double      actual_rate;
    qint64      int64_val;
    int         int_val;
    bool        bool_val;
    bool        conf_ok = false;
    bool        conv_ok;
    bool        skip_loading_cfg = false;
    int         ver = 0;
    qint64      hw_freq = 0;

    qDebug() << "Loading configuration from:" << cfgfile;

    if (m_settings)
    {
        // set current config to not crashed before loading new config
        m_settings->setValue("crashed", false);
        m_settings->sync();
        delete m_settings;
    }

    if (QDir::isAbsolutePath(cfgfile))
        m_settings = new QSettings(cfgfile, QSettings::IniFormat);
    else
        m_settings = new QSettings(QString("%1/%2").arg(m_cfg_dir).arg(cfgfile),
                                   QSettings::IniFormat);

    qDebug() << "Configuration file:" << m_settings->fileName();

    if (check_crash)
    {
        if (m_settings->value("crashed", false).toBool())
        {
            qDebug() << "Crash guard triggered!";
            auto* askUserAboutConfig =
                    new QMessageBox(QMessageBox::Warning, tr("Crash Detected!"),
                                    tr("<p>Gqrx has detected problems with the current configuration. "
                                       "Loading the configuration again could cause the application to crash.</p>"
                                       "<p>Do you want to edit the settings?</p>"),
                                    QMessageBox::Yes | QMessageBox::No);
            askUserAboutConfig->setDefaultButton(QMessageBox::Yes);
            askUserAboutConfig->setTextFormat(Qt::RichText);
            askUserAboutConfig->exec();
            if (askUserAboutConfig->result() == QMessageBox::Yes)
                skip_loading_cfg = true;

            delete askUserAboutConfig;
        }
        else
        {
            m_settings->setValue("crashed", true); // clean exit will set this to FALSE
            m_settings->sync();
        }
    }

    if (skip_loading_cfg)
        return false;

    // manual reconf (FIXME: check status)
    conv_ok = false;

    ver = m_settings->value("configversion").toInt(&conv_ok);
    // hide toolbar
    bool_val = m_settings->value("gui/hide_toolbar", false).toBool();
    if (bool_val)
        ui->mainToolBar->hide();

    // main window settings
    if (restore_mainwindow)
    {
        restoreGeometry(m_settings->value("gui/geometry",
                                          saveGeometry()).toByteArray());
        restoreState(m_settings->value("gui/state", saveState()).toByteArray());
    }

    int_val = m_settings->value("output/sample_rate", 48000).toInt(&conv_ok);
    if (conv_ok && (int_val > 0))
    {
        rx->set_audio_rate(int_val);
        uiDockAudio->setFftSampleRate(int_val);
    }

    QString indev = m_settings->value("input/device", "").toString();
    if (!indev.isEmpty())
    {
        try
        {
            rx->set_input_device(indev.toStdString());
            conf_ok = true;
        }
        catch (std::runtime_error &x)
        {
            QMessageBox::warning(nullptr,
                             QObject::tr("Failed to set input device"),
                             QObject::tr("<p><b>%1</b></p>"
                                         "Please select another device.")
                                     .arg(x.what()),
                             QMessageBox::Ok);
        }

        // Update window title
        setWindowTitle(QString("Gqrx %1 - %2").arg(VERSION).arg(indev));

        // Add available antenna connectors to the UI
        std::vector<std::string> antennas = rx->get_antennas();
        uiDockInputCtl->setAntennas(antennas);

        // Update gain stages.
        if (indev.contains("rtl", Qt::CaseInsensitive)
                && !m_settings->contains("input/gains"))
        {
            /* rtlsdr gain is 0 by default making users think their device is
             * deaf. Therefore, we don't read gain from the device, but initialize
             * it to the midpoint.
             */
            updateGainStages(false);
        }
        else
            updateGainStages(true);
    }

    QString outdev = m_settings->value("output/device", "").toString();

    try {
        rx->set_output_device(outdev.toStdString());
    } catch (std::exception &x) {
        QMessageBox::warning(nullptr,
                         QObject::tr("Failed to set output device"),
                         QObject::tr("<p><b>%1</b></p>"
                                     "Please select another device.")
                                 .arg(x.what()),
                         QMessageBox::Ok);
    }

    int_val = m_settings->value("input/sample_rate", 0).toInt(&conv_ok);
    if (conv_ok && (int_val > 0))
    {
        actual_rate = rx->set_input_rate(int_val);

        if (actual_rate == 0)
        {
            // There is an error with the device (perhaps not attached)
            // Warn user and use 100 ksps (rate used by gr-osmocom null_source)
            auto *dialog =
                    new QMessageBox(QMessageBox::Warning, tr("Device Error"),
                                    tr("There was an error configuring the input device.\n"
                                       "Please make sure that a supported device is attached "
                                       "to the computer and restart gqrx."),
                                    QMessageBox::Ok);
            dialog->setModal(true);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();

            actual_rate = int_val;
        }

        qDebug() << "Requested sample rate:" << int_val;
        qDebug() << "Actual sample rate   :" << QString("%1").arg(actual_rate, 0, 'f', 6);
    }
    else
        actual_rate = rx->get_input_rate();

    if (actual_rate > 0.)
    {
        int_val = m_settings->value("input/decimation", 1).toInt(&conv_ok);
        if (conv_ok && int_val >= 2)
        {
            if (rx->set_input_decim(int_val) != (unsigned int)int_val)
            {
                qDebug() << "Failed to set decimation" << int_val;
                qDebug() << "  actual decimation:" << rx->get_input_decim();
            }
            else
            {
                // update actual rate
                actual_rate /= (double)int_val;
                qDebug() << "Input decimation:" << int_val;
                qDebug() << "Quadrature rate:" << QString("%1").arg(actual_rate, 0, 'f', 6);
            }
        }
        else
            rx->set_input_decim(1);

        // update various widgets that need a sample rate
        uiDockRxOpt->setFilterOffsetRange((qint64)(actual_rate));
        uiDockFft->setSampleRate(actual_rate);
        ui->plotter->setSampleRate(actual_rate);
        uiDockProbe->setSampleRate(actual_rate);
        uiDockProbe->setDecimOsr(rx->get_chan_decim(), rx->get_chan_osr());
        ui->plotter->setSpanFreq((quint32)actual_rate);
        remote->setBandwidth((qint64)actual_rate);
        iq_tool->setSampleRate((qint64)actual_rate);
    }
    else
        qDebug() << "Error: Actual sample rate is" << actual_rate;

    int64_val = m_settings->value("input/bandwidth", 0).toInt(&conv_ok);
    if (conv_ok)
    {
        // set analog bw even if 0 since for some devices 0 Hz means "auto"
        double actual_bw = rx->set_analog_bandwidth((double) int64_val);
        qDebug() << "Requested bandwidth:" << int64_val << "Hz";
        qDebug() << "Actual bandwidth   :" << actual_bw << "Hz";
    }

    uiDockInputCtl->readSettings(m_settings);
    //update frequency range. Actual "ignore limits" value will be read later.
    ignoreLimitsObserver(C_IGNORE_LIMITS, 1);

    auto defs=c_def::all();
    for(int j=0;j<C_COUNT;j++)
    {
        const auto & def=defs[j];
        if(!(def.writable()&&def.readable()))
            continue;
        if((def.scope()!=S_VFO)&&(def.config_key()!=""))
        {
            c_def::v_union v(0);
            const c_id id=c_id(j);
            m_settings->beginGroup(QString::fromStdString(def.v3_config_group()));
            loadSetting(m_settings, def,v);
            m_settings->endGroup();
            if(def.scope()==S_RX)
                rx->set_value(id,v);
            set_gui(id, v, true);
        }
    }
    int64_val = m_settings->value("input/frequency", 14236000).toLongLong(&conv_ok);

    // If frequency is out of range set frequency to the center of the range.
    hw_freq = int64_val - d_lnb_lo;
    if (hw_freq < d_hw_freq_start || hw_freq > d_hw_freq_stop)
    {
        hw_freq = (d_hw_freq_stop - d_hw_freq_start) / 2;
        int64_val = hw_freq + d_lnb_lo;
    }

    rx->set_rf_freq(hw_freq);
    if (ver >= 4)
    {
        ui->freqCtrl->setFrequency(int64_val  + (qint64)(rx->get_filter_offset()));
        setNewFrequency(ui->freqCtrl->getFrequency()); // ensure all GUI and RF is updated
    }
    readRXSettings(ver, actual_rate);
    if (ver < 4)
    {
        rx->set_rf_freq(hw_freq - rx->get_filter_offset());
        ui->freqCtrl->setFrequency(hw_freq + d_lnb_lo);
        setNewFrequency(hw_freq + d_lnb_lo);
    }

    // Center frequency for FFT plotter
    int64_val = m_settings->value("fft/fft_center", 0).toLongLong(&conv_ok);

    if (conv_ok) {
        ui->plotter->setFftCenterFreq(int64_val);
    }

    uiDockBookmarks->readSettings(m_settings);
    rx->commit_audio_rate();

    /*
     * Initialization the remote control at the end.
     * We must be sure that all variables initialized before starting RC server.
     */
    remote->readSettings(m_settings);
    bool_val = m_settings->value("remote_control/enabled", false).toBool();
    if (bool_val)
    {
       remote->start_server();
       ui->actionRemoteControl->setChecked(true);
    }

    emit m_recent_config->configLoaded(m_settings->fileName());

    return conf_ok;
}

/**
 * @brief Save current configuration to a file.
 * @param cfgfile
 * @returns True if the operation was successful.
 *
 * If cfgfile is an absolute path it will be used as is, otherwise it is
 * assumed to be the name of a file under m_cfg_dir.
 *
 * If cfgfile already exists it will be overwritten (we assume that a file
 * selection dialog has already asked for confirmation of overwrite).
 *
 * Since QSettings does not support "save as" we do this by copying the current
 * settings to a new file.
 */
bool MainWindow::saveConfig(const QString& cfgfile)
{
    QString oldfile = m_settings->fileName();
    QString newfile;

    qDebug() << "Saving configuration to:" << cfgfile;

    m_settings->sync();

    if (QDir::isAbsolutePath(cfgfile))
        newfile = cfgfile;
    else
        newfile = QString("%1/%2").arg(m_cfg_dir).arg(cfgfile);

    if (newfile == oldfile) {
        qDebug() << "New file is equal to old file => SYNCING...";
        emit m_recent_config->configSaved(newfile);
        return true;
    }

    if (QFile::exists(newfile))
    {
        qDebug() << "File" << newfile << "already exists => DELETING...";
        if (QFile::remove(newfile))
            qDebug() << "Deleted" << newfile;
        else
            qDebug() << "Failed to delete" << newfile;
    }
    if (QFile::copy(oldfile, newfile))
    {
        loadConfig(cfgfile, false, false);
        return true;
    }
    else
    {
        qDebug() << "Error saving configuration to" << newfile;
        return false;
    }
}

static void storeSetting(QPointer<QSettings> m_settings, const c_def & def, const c_def::v_union & v)
{
    const auto vkey = QString::fromStdString(def.config_key());
    if(def.presets().size()>0)
    {
        //try to store by preset key
        auto it=def.ipresets().find(v);
        if(it!=def.ipresets().end())
        {
            if(def.def()==v)
                m_settings->remove(vkey);
            else
                m_settings->setValue(vkey,QString::fromStdString(def.presets()[it->second].key));
            return;
        }
    }
    switch(def.v_type())
    {
    case V_INT:
        if(int64_t(def.def())!=int64_t(v))
            m_settings->setValue(vkey,qlonglong(v));
        else
            m_settings->remove(vkey);
    break;
    case V_DOUBLE:
        {
            QString sv=QString::number(double(v),'f',def.frac_digits());
            if(QString::number(double(def.def()),'f',def.frac_digits()) != sv)
                m_settings->setValue(vkey,sv);
            else
                m_settings->remove(vkey);
        }
    break;
    case V_STRING:
        if(std::string(def.def())!=std::string(v))
            m_settings->setValue(vkey,QString::fromStdString(v));
        else
            m_settings->remove(vkey);
    break;
    case V_BOOLEAN:
        if(bool(def.def())!=bool(v))
            m_settings->setValue(vkey,bool(v));
        else
            m_settings->remove(vkey);
    break;
    }
}

/**
 * Store session-related parameters (frequency, gain,...)
 *
 * This needs to be called when we switch input source, otherwise the
 * new source would use the parameters stored on last exit.
 */
void MainWindow::storeSession()
{
    if (m_settings)
    {
        auto defs=c_def::all();
        int rx_count = rx->get_rx_count();
        m_settings->setValue("configversion", (rx_count <= 1) ? 3 : 4);
        for (int i = 0; true; i++)
        {
            QString grp = QString("rx%1").arg(i);
            QString offset = QString("rx%1/offset").arg(i);
            if (m_settings->contains(offset))
                m_settings->remove(grp);
            else
                break;
        }
        m_settings->remove("audio");
        m_settings->remove("receiver");
        m_settings->setValue("fft/fft_center", ui->plotter->getFftCenterFreq());
        if (rx_count <= 1)
            m_settings->setValue("input/frequency", qint64(rx->get_rf_freq() + d_lnb_lo + rx->get_filter_offset()));
        else
            m_settings->setValue("input/frequency", qint64(rx->get_rf_freq() + d_lnb_lo));

        uiDockInputCtl->saveSettings(m_settings);
        uiDockBookmarks->saveSettings(m_settings);

        remote->saveSettings(m_settings);

        int old_current = rx->get_current();
        for (int i = 0; i < rx_count; i++)
        {
            if (rx_count <= 1)
                m_settings->beginGroup("receiver");
            else
                m_settings->beginGroup(QString("rx%1").arg(i));
            m_settings->remove("");
            rx->fake_select_rx(i);

            qint64 offs = rx->get_filter_offset();
                m_settings->setValue("offset", offs);

            if (rx_count <= 1)
            {
                m_settings->endGroup();
                m_settings->beginGroup("audio");
            }

            m_settings->endGroup();

            for(int j = 0; j < C_COUNT; j++)
            {
                const auto & def=defs[j];
                if(!(def.writable()&&def.readable()))
                    continue;
                if((def.scope()==S_VFO)&&(def.config_key()!=""))
                {
                    c_def::v_union v(0);
                    const c_id id = c_id(j);
                    rx->get_value(id, v);
                    if(rx_count <= 1)
                        m_settings->beginGroup(QString::fromStdString(def.v3_config_group()));
                    else
                        m_settings->beginGroup(QString("rx%1").arg(i));
                    storeSetting(m_settings, def, v);
                    m_settings->endGroup();
                }
            }

            if (rx_count <= 1)
                break;
        }
        rx->fake_select_rx(old_current);
        if (rx_count > 1)
            m_settings->setValue("gui/current_rx", old_current);
        else
            m_settings->remove("gui/current_rx");
        for(int j = 0; j < C_COUNT; j++)
        {
            const auto & def = defs[j];
            if(!(def.writable()&&def.readable()))
                continue;
            if((def.scope()==S_RX)&&(def.config_key()!=""))
            {
                c_def::v_union v(0);
                const c_id id = c_id(j);
                rx->get_value(id, v);
                m_settings->beginGroup(QString::fromStdString(def.v3_config_group()));
                storeSetting(m_settings, def, v);
                m_settings->endGroup();
            }
            if((def.scope()==S_GUI)&&(def.config_key()!=""))
            {
                c_def::v_union v(0);
                const c_id id = c_id(j);
                get_gui(id, v);
                m_settings->beginGroup(QString::fromStdString(def.v3_config_group()));
                storeSetting(m_settings, def, v);
                m_settings->endGroup();
            }
        }
    }
}

void MainWindow::readRXSettings(int ver, double actual_rate)
{
    bool conv_ok;
    int int_val = 0;
    int i = 0;
    qint64 offs = 0;
    c_def::v_union v;
    rxSpinBox->setMaximum(0);
    while (rx->get_rx_count() > 1)
        rx->delete_rx();
    ui->plotter->setCurrentVfo(0);
    ui->plotter->clearVfos();
    QString grp = (ver >= 4) ? QString("rx%1").arg(i) : "receiver";
    while (1)
    {


        auto defs=c_def::all();
        for(int j=0;j<C_COUNT;j++)
        {
            const auto & def=defs[j];
            if(!(def.writable()&&def.readable()))
                continue;
            if((def.scope()==S_VFO)&&(def.config_key()!=""))
            {
                const c_id id=c_id(j);
                if(ver < 4)
                    m_settings->beginGroup(QString::fromStdString(def.v3_config_group()));
                else
                    m_settings->beginGroup(grp);
                loadSetting(m_settings, def,v);
                m_settings->endGroup();
                rx->set_value(id,v);
            }
        }
        m_settings->beginGroup(grp);
        offs = m_settings->value("offset", 0).toInt(&conv_ok);
        get_gui(C_FREQ_LOCK,v);
        if (conv_ok)
        {
            if(!bool(v) || ver < 4)
                if(std::abs(offs) > actual_rate / 2)
                    offs = (offs > 0) ? (actual_rate / 2) : (-actual_rate / 2);
            rx->set_filter_offset(offs);
        }
        m_settings->endGroup();

        ui->plotter->addVfo(rx->get_current_vfo());
        i++;
        if (ver < 4)
            break;
        grp = QString("rx%1").arg(i);
        if (!m_settings->contains(grp + "/offset"))
            break;
        rx->add_rx();
    }
    if (ver >= 4)
        int_val = m_settings->value("gui/current_rx", 0).toInt(&conv_ok);
    else
        conv_ok = false;
    if (!conv_ok)
        int_val = 0;
    rxSpinBox->setMaximum(rx->get_rx_count() - 1);
    if(int_val >= rx->get_rx_count())
        int_val = 0;
    ui->plotter->removeVfo(rx->get_vfo(int_val));
    rx->select_rx(int_val);
    ui->plotter->setCurrentVfo(int_val);
    if (rxSpinBox->value() != int_val)
        rxSpinBox->setValue(int_val);
    offs = rx->get_filter_offset();
    if(std::abs(offs) > actual_rate / 2)
        rx->set_filter_offset((offs > 0) ? (actual_rate / 2) : (-actual_rate / 2));
    loadRxToGUI();
}

/**
 * @brief Update hardware RF frequency range.
 * @param ignore_limits Whether ignore the hardware specd and allow DC-to-light
 *                      range.
 *
 * This function fetches the frequency range of the receiver. Useful when we
 * read a new configuration with a new input device or when the ignore_limits
 * setting is changed.
 */
void MainWindow::updateHWFrequencyRange(bool ignore_limits)
{
    double startd, stopd, stepd;

    d_ignore_limits = ignore_limits;

    if (ignore_limits)
    {
        d_hw_freq_start = (quint64) 0;
        d_hw_freq_stop  = (quint64) 9999e6;
    }
    else if (rx->get_rf_range(&startd, &stopd, &stepd) == receiver::STATUS_OK)
    {
        d_hw_freq_start = (quint64) startd;
        d_hw_freq_stop  = (quint64) stopd;
    }
    else
    {
        qDebug() << __func__ << "failed fetching new hardware frequency range";
        d_hw_freq_start = (quint64) 0;
        d_hw_freq_stop  = (quint64) 9999e6;
    }

    updateFrequencyRange(); // Also update the available frequency range
}

/**
 * @brief Update available frequency range.
 *
 * This function sets the available frequency range based on the hardware
 * frequency range, the selected filter offset and the LNB LO.
 *
 * This function must therefore be called whenever the LNB LO or the filter
 * offset has changed.
 */
void MainWindow::updateFrequencyRange()
{
    auto start = (qint64)(rx->get_filter_offset()) + d_hw_freq_start + d_lnb_lo;
    auto stop  = (qint64)(rx->get_filter_offset()) + d_hw_freq_stop  + d_lnb_lo;

    ui->freqCtrl->setup(0, start, stop, 1, FCTL_UNIT_NONE);
    uiDockRxOpt->setRxFreqRange(start, stop);
    ui->plotter->setFrequencyRange(start, stop);
}

/**
 * @brief Update gain stages.
 * @param read_from_device If true, the gain value will be read from the device,
 *                         otherwise we set gain to the midpoint.
 *
 * This function fetches a list of available gain stages with their range
 * and sends them to the input control UI widget.
 */
void MainWindow::updateGainStages(bool read_from_device)
{
    gain_list_t gain_list;
    std::vector<std::string> gain_names = rx->get_gain_names();
    gain_t gain;

    std::vector<std::string>::iterator it;
    for (it = gain_names.begin(); it != gain_names.end(); ++it)
    {
        gain.name = *it;
        rx->get_gain_range(gain.name, &gain.start, &gain.stop, &gain.step);
        if (read_from_device)
        {
            gain.value = rx->get_gain(gain.name);
        }
        else
        {
            gain.value = (gain.start + gain.stop) / 2;
            rx->set_gain(gain.name, gain.value);
        }
        gain_list.push_back(gain);
    }

    uiDockInputCtl->setGainStages(gain_list);
    remote->setGainStages(gain_list);
}

/**
 * @brief Slot for receiving frequency change signals.
 * @param[in] freq The new frequency.
 *
 * This slot is connected to the CFreqCtrl::newFrequency() signal and is used
 * to set new receive frequency.
 */
void MainWindow::setNewFrequency(qint64 rx_freq)
{
    auto new_offset = rx->get_filter_offset();
    auto hw_freq = (double)(rx_freq - d_lnb_lo) - new_offset;
    auto center_freq = rx_freq - (qint64)new_offset;
    auto delta_freq = d_hw_freq;
    QList<BookmarkInfo> bml;
    auto max_offset = rx->get_input_rate() / 2;
    bool update_offset = rx->is_playing_iq() || rx->is_recording_iq();

    rx->set_rf_freq(hw_freq);
    d_hw_freq = d_ignore_limits ? hw_freq : (qint64)rx->get_rf_freq();
    update_offset |= (d_hw_freq != (qint64)hw_freq);

    if (rx_freq - d_lnb_lo - d_hw_freq > max_offset)
    {
        rx_freq = d_lnb_lo + d_hw_freq + max_offset;
        update_offset = true;
    }
    if (rx_freq - d_lnb_lo - d_hw_freq < -max_offset)
    {
        rx_freq = d_lnb_lo + d_hw_freq - max_offset;
        update_offset = true;
    }
    if (update_offset)
    {
        new_offset = rx_freq - d_lnb_lo - d_hw_freq;
        if (d_hw_freq != (qint64)hw_freq)
        {
            center_freq = d_hw_freq + d_lnb_lo;
            // set RX filter
            rx->set_filter_offset((double)new_offset);

            // update RF freq label and channel filter offset
            rx_freq = center_freq + new_offset;
         }
    }
    delta_freq -= d_hw_freq;

    // update widgets
    ui->plotter->setCenterFreq(center_freq);
    if(rx->get_current() == 0)
        uiDockProbe->setCenterOffset(center_freq, new_offset);
    ui->plotter->setFilterOffset(new_offset);
    set_gui(C_VFO_FREQUENCY, rx_freq * 1e-3);
    uiDockRxOpt->setFilterOffset(new_offset);
    ui->freqCtrl->setFrequency(rx_freq);
    uiDockBookmarks->setNewFrequency(rx_freq);
    remote->setNewFrequency(rx_freq);
    uiDockAudio->setRxFrequency(rx_freq);
    if (delta_freq)
    {
        std::set<int> del_list;
        if (rx->get_rx_count() > 1)
        {
            std::vector<vfo::sptr> locked_vfos;
            int offset_lim = (int)(ui->plotter->getSampleRate() / 2);
            ui->plotter->getLockedVfos(locked_vfos);
            for (auto& cvfo : locked_vfos)
            {
                ui->plotter->removeVfo(cvfo);
                int new_offset = cvfo->get_offset() + delta_freq;
                if ((new_offset > offset_lim) || (new_offset < -offset_lim))
                    del_list.insert(cvfo->get_index());
                else
                {
                    rx->set_filter_offset(cvfo->get_index(), new_offset);
                    ui->plotter->addVfo(cvfo);
                }
            }
        }

        if (d_auto_bookmarks)
        {
            //calculate frequency range to search for auto bookmarks
            qint64 from = 0, to = 0;
            qint64 sr = ui->plotter->getSampleRate();
            if (delta_freq > 0)
            {
                if (delta_freq > sr)
                {
                    from = center_freq - sr / 2;
                    to = center_freq + sr / 2;
                }
                else
                {
                    from = center_freq - sr / 2;
                    to = center_freq - sr / 2 + delta_freq;
                }
            }
            else
            {
                if (-delta_freq > sr)
                {
                    from = center_freq - sr / 2;
                    to = center_freq + sr / 2;
                }
                else
                {
                    from = center_freq + sr / 2 + delta_freq;
                    to = center_freq + sr / 2;
                }
            }
            bml = Bookmarks::Get().getBookmarksInRange(from, to, true);
        }

        if ((del_list.size() > 0)||(bml.size() > 0))
        {
            int current = rx->get_current();
            if (ui->actionDSP->isChecked())
                rx->stop();
            for (auto& bm : bml)
            {
                int n = rx->add_rx();
                if (n > 0)
                {
                    c_def::v_union old_sql;
                    rxSpinBox->setMaximum(rx->get_rx_count() - 1);
                    rx->set_demod(bm.get_demod());
                    // preserve squelch level, force locked state
                    auto old_vfo = rx->get_current_vfo();
                    old_vfo->get_sql_level(old_sql);
                    old_vfo->restore_settings(bm, false);
                    old_vfo->set_sql_level(old_sql);
                    old_vfo->set_offset(bm.frequency - center_freq);
                    old_vfo->set_freq_lock(true);
                    ui->plotter->addVfo(old_vfo);
                    rx->select_rx(current);
                }
            }
            if (del_list.size() > 0)
            {
                int lastCurrent = rx->get_current();
                for (auto i = del_list.rbegin(); i != del_list.rend(); ++i)
                {
                    int last = rx->get_rx_count() - 1;
                    rx->select_rx(*i);
                    if (lastCurrent == last)
                    {
                        lastCurrent = *i;
                        last = -1;
                    }
                    else
                        if (*i != last)
                        {
                            ui->plotter->removeVfo(rx->get_vfo(last));
                            last = *i;
                        }
                        else
                            last = -1;
                    rx->delete_rx();
                    if (last != -1)
                        ui->plotter->addVfo(rx->get_vfo(last));
                }
                rx->select_rx(lastCurrent);
                ui->plotter->setCurrentVfo(lastCurrent);
                rxSpinBox->setMaximum(rx->get_rx_count() - 1);
                rxSpinBox->setValue(lastCurrent);
            }
            if (ui->actionDSP->isChecked())
                rx->start();
            ui->plotter->updateOverlay();
        }
    }
}

/**
 * @brief Set new LNB LO frequency.
 * @param freq_mhz The new frequency in MHz.
 */
void MainWindow::lnbLoObserver(c_id, const c_def::v_union &v)
{
    const qint64 lnb_lo_hz = v;
    if(d_lnb_lo==lnb_lo_hz)
        return;
    // calculate current RF frequency
    auto rf_freq = ui->freqCtrl->getFrequency() - d_lnb_lo;

    d_lnb_lo = lnb_lo_hz;
    qDebug() << "New LNB LO:" << d_lnb_lo << "Hz";

    remote->setLnbLo(int64_t(v)*1e-6);
    // Update ranges and show updated frequency
    updateFrequencyRange();
    setNewFrequency(d_lnb_lo + rf_freq);

    #if 0
    // update LNB LO in settings
    if (freq_mhz == 0.)
        m_settings->remove("input/lnb_lo");
    else
        m_settings->setValue("input/lnb_lo", d_lnb_lo);
    #endif
}

/**
 * @brief Set new channel filter offset.
 * @param freq_hz The new filter offset in Hz.
 */
void MainWindow::setFilterOffset(qint64 freq_hz)
{
    rx->set_filter_offset((double) freq_hz);

    updateFrequencyRange();

    auto rx_freq = d_hw_freq + d_lnb_lo + freq_hz;
    setNewFrequency(rx_freq);

}

/**
 * @brief Set a specific gain.
 * @param name The name of the gain stage to adjust.
 * @param gain The new value.
 */
void MainWindow::setGain(const QString& name, double gain)
{
    rx->set_gain(name.toStdString(), gain);
}


/**
 * @brief Ignore hardware limits.
 * @param ignore_limits Whether hardware limits should be ignored or not.
 *
 * This slot is triggered when the user changes the "Ignore hardware limits"
 * option. It will update the allowed frequency range and also update the
 * current RF center frequency, which may change when we switch from ignore to
 * don't ignore.
 */
void MainWindow::ignoreLimitsObserver(c_id, const c_def::v_union & ignore_limits)
{
    updateHWFrequencyRange(ignore_limits);

    auto filter_offset = qRound64(rx->get_filter_offset());
    auto freq = qRound64(rx->get_rf_freq());
    ui->freqCtrl->setFrequency(d_lnb_lo + freq + filter_offset);

    // This will ensure that if frequency is clamped and that
    // the UI is updated with the correct frequency.
    freq = ui->freqCtrl->getFrequency();
    setNewFrequency(freq);
}


/** Reset lower digits of main frequency control widget */
void MainWindow::freqCtrlResetObserver(c_id, const c_def::v_union & enabled)
{
    ui->freqCtrl->setResetLowerDigits(enabled);
    uiDockRxOpt->setResetLowerDigits(enabled);
}

/** Invert scroll wheel direction */
void MainWindow::invertScrollingObserver(c_id, const c_def::v_union & enabled)
{
    ui->freqCtrl->setInvertScrolling(enabled);
    ui->plotter->setInvertScrolling(enabled);
    uiDockRxOpt->setInvertScrolling(enabled);
    uiDockAudio->setInvertScrolling(enabled);
    uiDockProbe->setInvertScrolling(enabled);
}

/** Automatic demodulators */
void MainWindow::autoBookmarksObserver(c_id, const c_def::v_union & v)
{
    d_auto_bookmarks = v;
}

/**
 * @brief Update GUI after demodulator selection.
 *
 * Update plotter demod ranges
 * Update audio dock fft range
 * Update plotter cut frequencies
 * Update plotter click resolution
 * Update plotter filter click resolution
 * Update remote settings too
 *
 */
void MainWindow::updateDemodGUIRanges(const Modulations::idx mode_idx)
{
    int click_res=100;
    int     flo=0, fhi=0, loMin, loMax, hiMin,hiMax;
    int     audio_rate = rx->get_audio_rate();
    Modulations::filter_shape filter_shape;
    rx->get_filter(flo, fhi, filter_shape);
    Modulations::GetFilterRanges(mode_idx, loMin, loMax, hiMin, hiMax);
    ui->plotter->setDemodRanges(loMin, loMax, hiMin, hiMax, hiMax == -loMin);
    switch (mode_idx) {

    case Modulations::MODE_OFF:
        /* Spectrum analyzer only */
        click_res = 1000;
        break;

    case Modulations::MODE_RAW:
        /* Raw I/Q; max 96 ksps*/
        uiDockAudio->setFftRange(-std::min(24000, audio_rate / 2), std::min(24000, audio_rate / 2));
        click_res = 100;
        break;

    case Modulations::MODE_AM:
        uiDockAudio->setFftRange(0, std::min(6000, audio_rate / 2));
        click_res = 100;
        break;

    case Modulations::MODE_AM_SYNC:
        uiDockAudio->setFftRange(0, std::min(6000, audio_rate / 2));
        click_res = 100;
        break;

    case Modulations::MODE_NFM:
        uiDockAudio->setFftRange(0, std::min(5000, audio_rate / 2));
        click_res = 100;
        break;

    case Modulations::MODE_NFMPLL:
        uiDockAudio->setFftRange(0, 5000);
        click_res = 100;
        break;

    case Modulations::MODE_WFM_MONO:
    case Modulations::MODE_WFM_STEREO:
    case Modulations::MODE_WFM_STEREO_OIRT:
        /* Broadcast FM */
        uiDockAudio->setFftRange(0, std::min(24000, audio_rate / 2));
        click_res = 1000;
        break;

    case Modulations::MODE_LSB:
        /* LSB */
        uiDockAudio->setFftRange(0, std::min(3000, audio_rate / 2));
        click_res = 100;
        break;

    case Modulations::MODE_USB:
        /* USB */
        uiDockAudio->setFftRange(0, std::min(3000, audio_rate / 2));
        click_res = 100;
        break;

    case Modulations::MODE_CWL:
        /* CW-L */
        uiDockAudio->setFftRange(0, std::min(1500, audio_rate / 2));
        click_res = 10;
        break;

    case Modulations::MODE_CWU:
        /* CW-U */
        uiDockAudio->setFftRange(0, std::min(1500, audio_rate / 2));
        click_res = 10;
        break;

    default:
        click_res = 100;
        break;
    }

    qDebug() << "Filter preset for mode" << mode_idx << "LO:" << flo << "HI:" << fhi;
    ui->plotter->setHiLowCutFrequencies(flo, fhi);
    ui->plotter->setClickResolution(click_res);
    ui->plotter->setFilterClickResolution(click_res);

    remote->setMode(mode_idx);
    remote->setPassband(flo, fhi);

    d_have_audio = (mode_idx != Modulations::MODE_OFF);

    //Update demod-specific GUI widgets to keep shared options in sync
    auto defs=c_def::all();
    for(int j=0;j<C_COUNT;j++)
    {
        const auto & def=defs[j];
        if(!def.readable())
            continue;
        if((def.scope()==S_VFO)&&(def.demodgroup()==Modulations::modes[mode_idx].group))
        {
            const c_id id=c_id(j);
            c_def::v_union v(c_def::all()[id].def());
            rx->get_value(id,v);
            set_gui(id, v, true);
        }
    }
    d_have_audio = (mode_idx != Modulations::MODE_OFF);
    switch (mode_idx)
    {
        case Modulations::MODE_WFM_MONO:
        case Modulations::MODE_WFM_STEREO:
        case Modulations::MODE_WFM_STEREO_OIRT:
            /* Broadcast FM */
            uiDockRDS->setEnabled();
            break;
        default:
            uiDockRDS->setDisabled();
            rx->set_value(C_RDS_ON, false);
            set_gui(C_RDS_ON, false);
    }
}

void MainWindow::modeObserver(const c_id id, const c_def::v_union &value)
{
    Modulations::idx mode_idx = Modulations::idx(int(value));
    c_def::v_union tmp;
    rx->get_value(C_MODE, tmp);
    const Modulations::idx mode_old = Modulations::idx(int(tmp));
    if(mode_old != mode_idx)
    {

        switch (mode_idx)
        {
            case Modulations::MODE_OFF:
                /* Spectrum analyzer only */
                //TODO: check if demod has audio/debug stream output???
                rx->set_value(C_AUDIO_REC, false);
                set_gui(C_AUDIO_REC, false);
                //TODO:check for NFM here
                if (dec_afsk1200 != nullptr)
                {
                    dec_afsk1200->close();
                }
                break;
            case Modulations::MODE_AM:
            case Modulations::MODE_AM_SYNC:
            case Modulations::MODE_USB:
            case Modulations::MODE_LSB:
            case Modulations::MODE_CWL:
            case Modulations::MODE_CWU:
            case Modulations::MODE_NFM:
            case Modulations::MODE_NFMPLL:
            case Modulations::MODE_WFM_MONO:
            case Modulations::MODE_WFM_STEREO:
            case Modulations::MODE_WFM_STEREO_OIRT:
                break;
            default:
                qDebug() << "Unsupported mode selection (can't happen!): " << mode_idx;
                break;
        }
    }
}

void MainWindow::modeChangeObserver(const c_id id, const c_def::v_union &value)
{
    c_def::v_union tmp;
    rx->get_value(C_MODE, tmp);
    const Modulations::idx mode_idx = Modulations::idx(int(tmp));
    updateDemodGUIRanges(mode_idx);
}

void MainWindow::filterWidthObserver(const c_id id, const c_def::v_union &value)
{
    int     flo=0, fhi=0;
    Modulations::filter_shape filter_shape;
    rx->get_filter(flo, fhi, filter_shape);
    if(Modulations::GetFilterPreset(rx->get_demod(), value, flo, fhi))
    {
        rx->set_filter(flo, fhi, filter_shape);
        ui->plotter->setHiLowCutFrequencies(flo, fhi);
    }
}

void MainWindow::frequencyObserver(const c_id id, const c_def::v_union &value)
{
    setNewFrequency(std::llround(double(value)*1e3));
}

/**
 * @brief Squelch level changed.
 * @param level_db The new squelch level in dBFS.
 */
void MainWindow::sqlLevelObserver(const c_id id, const c_def::v_union &value)
{
    ui->sMeter->setSqlLevel(value);
    remote->setSquelchLevel(value);
}

/** Signal strength meter timeout. */
void MainWindow::meterTimeout()
{
    float level;
    struct receiver::iq_tool_stats iq_stats;

    level = rx->get_signal_pwr();
    ui->sMeter->setLevel(level);
    remote->setSignalLevel(level);
    // As it looks like this timer is always active (when the DSP is running),
    // check iq recorder state here too
    rx->get_iq_tool_stats(iq_stats);
    if (iq_stats.recording)
    {
        if (iq_stats.failed)
        {
            //stop the recorder
            iq_tool->updateStats(iq_stats.failed, iq_stats.buffer_usage, iq_stats.file_pos);
            iq_tool->cancelRecording();
        }
        else
            //update status
            iq_tool->updateStats(iq_stats.failed, iq_stats.buffer_usage, iq_stats.file_pos);
    }
    if (iq_stats.playing)
    {
        iq_tool->updateStats(iq_stats.failed, iq_stats.buffer_usage, iq_stats.file_pos);
        d_seek_pos = iq_stats.file_pos;
    }
    c_def::v_union tmp;
    rx->get_value(C_AGC_ON, tmp);
    if (tmp)
    {
        auto current_gain=rx->get_agc_gain();
        set_gui(C_AGC_MAN_GAIN, current_gain);
        rx->set_value(C_AGC_MAN_GAIN, current_gain);
    }
}

#define LOG2_10 3.321928094887362

/** Baseband FFT plot timeout. */
void MainWindow::iqFftTimeout()
{
    unsigned int    fftsize;
    unsigned int    i;
    qint64 fft_approx_timestamp;
    qint64 fft_start=QDateTime::currentMSecsSinceEpoch();

    // FIXME: fftsize is a reference
    rx->get_iq_fft_data(d_fftData, fftsize);
    fft_approx_timestamp = rx->is_playing_iq() ? rx->get_filesource_timestamp_ms() : QDateTime::currentMSecsSinceEpoch();

    if (fftsize == 0)
    {
        /* nothing to do, wait until next activation. */
        return;
    }

    iqFftToMag(fftsize, d_fftData, d_realFftData);

    for (i = 0; i < fftsize; i++)
    {
        /* FFT averaging */
        d_iirFftData[i] += d_fftAvg * (d_realFftData[i] - d_iirFftData[i]);
    }

    ui->plotter->setNewFftData(d_iirFftData, d_realFftData, fftsize, fft_approx_timestamp);
    d_fft_duration+=(double(QDateTime::currentMSecsSinceEpoch()-fft_start)-d_fft_duration)*0.1;
    uiDockFft->setFftLag(d_fft_duration>iq_fft_timer->interval());
}

void MainWindow::iqFftToMag(unsigned int fftsize, std::complex<float>* fftData, float* realFftData) const
{
    // NB: without cast to float the multiplication will overflow at 64k
    // and pwr_scale will be inf
    float pwr_scale = 1.0f/ ((float)fftsize * (float)fftsize);

    /* Normalize, calculate power and shift the FFT */
    volk_32fc_magnitude_squared_32f(realFftData, fftData + (fftsize/2), fftsize/2);
    volk_32fc_magnitude_squared_32f(realFftData + (fftsize/2), fftData, fftsize/2);
    volk_32f_s32f_multiply_32f(realFftData, realFftData, pwr_scale, fftsize);
    volk_32f_log2_32f(realFftData, realFftData, fftsize);
    volk_32f_s32f_multiply_32f(realFftData, realFftData, 10 / LOG2_10, fftsize);
}

/** Audio FFT plot timeout. */
void MainWindow::audioFftTimeout()
{
    unsigned int    fftsize;

    if(uiDockProbe->isVisible())
    {
        rx->get_probe_fft_data(d_fftData, fftsize);
        if (fftsize > 0)
        {
            iqFftToMag(fftsize, d_fftData, d_realFftData);
            uiDockProbe->setNewFftData(d_realFftData, fftsize);
        }
    }

    if (!d_have_audio || !uiDockAudio->isVisible())
        return;

    rx->get_audio_fft_data(d_fftData, fftsize);

    if (fftsize == 0)
    {
        /* nothing to do, wait until next activation. */
        qDebug() << "No audio FFT data.";
        return;
    }

    iqFftToMag(fftsize, d_fftData, d_realFftData);
    uiDockAudio->setNewFftData(d_realFftData, fftsize);
}

void MainWindow::agcOnObserver(const c_id id, const c_def::v_union &value)
{
    uiDockAudio->setGainEnabled(!bool(value));
}

void MainWindow::agcManualGainObserver(const c_id id, const c_def::v_union &value)
{
    remote->setAudioGain(value);
}

void MainWindow::audioRecSettingsCopyObserver(const c_id id, const c_def::v_union &value)
{
    std::vector<vfo::sptr> vfos = rx->get_vfos();
    c_def::v_union rec_dir, min_time, max_gap;
    rx->get_value(C_AUDIO_REC_DIR, rec_dir);
    rx->get_value(C_AUDIO_REC_MIN_TIME, min_time);
    rx->get_value(C_AUDIO_REC_MAX_GAP, max_gap);
    for (auto& cvfo : vfos)
        if (cvfo->get_index() != rx->get_current())
        {
            cvfo->set_audio_rec_dir(rec_dir);
            cvfo->set_audio_rec_min_time(min_time);
            cvfo->set_audio_rec_max_gap(max_gap);
        }
}

void MainWindow::rdsPIObserver(const c_id id, const c_def::v_union &value)
{
    remote->rdsPI(QString::fromStdString(value));
}

void MainWindow::rdsPSObserver(const c_id id, const c_def::v_union &value)
{
    remote->setRdsStation(QString::fromStdString(value));
}

void MainWindow::rdsRTObserver(const c_id id, const c_def::v_union &value)
{
    remote->setRdsRadiotext(QString::fromStdString(value));
}

void MainWindow::audioRecObserver(const c_id id, const c_def::v_union &value)
{
    if(!bool(value))
    {
        ui->statusBar->showMessage(tr("Audio recorder stopped"), 5000);
        remote->stopAudioRecorder();
    }else
        remote->startAudioRecorder();
}

void MainWindow::audioRecFilenameObserver(const c_id id, const c_def::v_union &value)
{
    ui->statusBar->showMessage(tr("Recording audio to %1").arg(QString::fromStdString(value)));
}

#if QT_VERSION < QT_VERSION_CHECK(5, 8, 0)
namespace Qt
{
    constexpr const char * ISODateWithMs = "yyyy-MM-ddTHH:mm:ss.zzz[Z|[+|-]HH:mm]";
}
#endif

QString MainWindow::makeIQFilename(const QString& recdir, file_formats fmt, const QDateTime ts)
{
    // generate file name using date, time, rf freq in kHz and BW in Hz
    // gqrx_iq_yyyymmdd_hhmmss_freq_bw_fc.raw
    auto freq = qRound64(rx->get_rf_freq());
    auto sr = qRound64(rx->get_input_rate());
    auto dec = (quint32)(rx->get_input_decim());
    bool sigmf = (fmt == FILE_FORMAT_SIGMF);
    QString suffix = any_to_any_base::fmt[fmt].suffix;
    auto filenameTemplate = ts.toString("%1/gqrx_yyyyMMdd_hhmmss_%2_%3_%4")
            .arg(recdir).arg(freq).arg(sr/dec);
    QString filename = filenameTemplate.arg(suffix);

    if(sigmf)
    {
        metaFile = new QFile(filenameTemplate.arg("fc.sigmf-meta"), this);
        auto meta = QJsonDocument { QJsonObject {
            {"global", QJsonObject {
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
                {"core:datatype", "cf32_be"},
#else
                {"core:datatype", "cf32_le"},
#endif
                {"core:sample_rate", sr/dec},
                {"core:version", "1.0.0"},
                {"core:recorder", "Gqrx " VERSION},
                {"core:hw", QString("OsmoSDR: ") + m_settings->value("input/device", "").toString()},
            }}, {"captures", QJsonArray {
                QJsonObject {
                    {"core:sample_start", 0},
                    {"core:frequency", freq},
                    {"core:datetime", ts.toString(Qt::ISODateWithMs)},
                },
            }}, {"annotations", QJsonArray {}},
        }}.toJson();

        if (!metaFile->open(QIODevice::WriteOnly) || metaFile->write(meta) != meta.size()) {
            return "";
        }
    }
    return filename;
}

/** Start I/Q recording. */
void MainWindow::startIqRecording(const QString& recdir, file_formats fmt)
{

    bool sigmf = (fmt == FILE_FORMAT_SIGMF);
    auto lastRec = makeIQFilename(recdir, fmt, QDateTime::currentDateTimeUtc());
    ui->actionIoConfig->setDisabled(true);
    ui->actionLoadSettings->setDisabled(true);
    // start recorder; fails if recording already in progress
    if (lastRec.isEmpty() || rx->start_iq_recording(lastRec.toStdString(), fmt))
    {
        // remove metadata file if we managed to open it
        if (sigmf && metaFile->isOpen())
            metaFile->remove();

        // reset action status
        ui->statusBar->showMessage(tr("Error starting I/Q recoder"));
        iq_tool->cancelRecording();

        // show an error message to user
        QMessageBox msg_box;
        msg_box.setIcon(QMessageBox::Critical);
        msg_box.setText(tr("There was an error starting the I/Q recorder.\n"
                           "Check write permissions for the selected location."));
        msg_box.exec();

    }
    else
    {
        ui->statusBar->showMessage(tr("Recording I/Q data to: %1").arg(lastRec),
                                   5000);
    }
}

/** Stop current I/Q recording. */
void MainWindow::stopIqRecording()
{
    qDebug() << __func__;

    if (rx->stop_iq_recording())
        ui->statusBar->showMessage(tr("Error stopping I/Q recoder"));
    else
        ui->statusBar->showMessage(tr("I/Q data recoding stopped"), 5000);
    ui->actionIoConfig->setDisabled(false);
    ui->actionLoadSettings->setDisabled(false);
}

void MainWindow::startIqPlayback(const QString& filename, float samprate,
                                 qint64 center_freq,
                                 file_formats fmt,
                                 qint64 time_ms)
{
    if (ui->actionDSP->isChecked())
    {
        // suspend DSP while we reload settings
        on_actionDSP_triggered(false);
    }
    bool reopening = !rx->is_running() && rx->is_playing_iq();
    if(reopening)
        stopIQFftRedraw(true);
    else
        storeSession();

    auto sri = (int)samprate;
    auto cf  = center_freq;
    double current_offset = rx->get_filter_offset();
    QString escapedFilename = receiver::escape_filename(filename.toStdString()).c_str();
    auto devstr = QString("file=%1,rate=%2,freq=%3,throttle=true,repeat=false")
            .arg(escapedFilename).arg(sri).arg(cf);

    qDebug() << __func__ << ":" << devstr;

    rx->set_input_device(devstr.toStdString());
    updateHWFrequencyRange(false);
    rx->set_input_file(filename.toStdString(), samprate, fmt, time_ms);

    // sample rate
    auto actual_rate = rx->set_input_rate((double)samprate);
    qDebug() << "Requested sample rate:" << samprate;
    qDebug() << "Actual sample rate   :" << QString("%1")
                .arg(actual_rate, 0, 'f', 6);

    uiDockRxOpt->setFilterOffsetRange((qint64)(actual_rate));
    ui->plotter->setSampleRate(actual_rate);
    ui->plotter->setPlayingIQ(true);
    uiDockFft->setSampleRate(actual_rate);
    uiDockProbe->setSampleRate(actual_rate);
    uiDockProbe->setDecimOsr(rx->get_chan_decim(), rx->get_chan_osr());
    ui->plotter->setSpanFreq((quint32)actual_rate);
    if (std::abs(current_offset) > actual_rate / 2)
        on_plotter_newDemodFreq(center_freq, 0);
    else
        on_plotter_newDemodFreq(center_freq + current_offset, current_offset);
    ui->plotter->resetHorizontalZoom();

    remote->setBandwidth(actual_rate);

    // FIXME: would be nice with good/bad status
    ui->statusBar->showMessage(tr("Playing %1").arg(filename));
    ui->actionIoConfig->setDisabled(true);
    ui->actionLoadSettings->setDisabled(true);
    ui->actionSaveSettings->setDisabled(true);

    if(reopening)
    {
        //struct receiver::iq_tool_stats iq_stats;
        int lines=0;
        double ms_per_line = 0.0;
        ui->plotter->getWaterfallMetrics(lines, ms_per_line);
        uint64_t pos = std::llround(double(lines)*ms_per_line*1e-3*double(actual_rate)/any_to_any_base::fmt[rx->get_last_format()].nsamples);
        seekIqFile(std::min(rx->get_iq_file_size(),pos));
        iq_tool->updateStats(false, 0, pos);
        triggerIQFftRedraw(true);
    }else
        on_actionDSP_triggered(true);
}

void MainWindow::stopIqPlayback()
{
    if (ui->actionDSP->isChecked())
    {
        // suspend DSP while we reload settings
        on_actionDSP_triggered(false);
    }else{
        /* Make shure, that background thread will not interfere normal waterfall rendering */
        stopIQFftRedraw();
    }

    ui->statusBar->showMessage(tr("I/Q playback stopped"), 5000);
    ui->actionIoConfig->setDisabled(false);
    ui->actionLoadSettings->setDisabled(false);
    ui->actionSaveSettings->setDisabled(false);

    // restore original input device
    auto indev = m_settings->value("input/device", "").toString();
    try{
        rx->set_input_device(indev.toStdString());
    }catch(std::runtime_error &x)
    {
    }

    // restore sample rate
    bool conv_ok;
    auto sr = m_settings->value("input/sample_rate", 0).toInt(&conv_ok);
    if (conv_ok && (sr > 0))
    {
        auto actual_rate = rx->set_input_rate(sr);
        qDebug() << "Requested sample rate:" << sr;
        qDebug() << "Actual sample rate   :" << QString("%1")
                    .arg(actual_rate, 0, 'f', 6);

        uiDockRxOpt->setFilterOffsetRange((qint64)(actual_rate));
        ui->plotter->setSampleRate(actual_rate);
        uiDockFft->setSampleRate(actual_rate);
        uiDockProbe->setSampleRate(actual_rate);
        uiDockProbe->setDecimOsr(rx->get_chan_decim(), rx->get_chan_osr());
        ui->plotter->setSpanFreq((quint32)actual_rate);
        remote->setBandwidth(sr);
    }

    // restore frequency, gain, etc...
    uiDockInputCtl->readSettings(m_settings);
    auto defs = c_def::all();
    c_def::v_union tmp;
    for(int k=0;k<C_COUNT;k++)
        if(defs[k].dock()==D_INPUTCTL)
            if(defs[k].scope()==S_RX)
            {
                get_gui(c_id(k), tmp);
                changed_gui(c_id(k), tmp);
            }
    bool centerOK = false;
    bool offsetOK = false;
    qint64 oldCenter = m_settings->value("input/frequency", 0).toLongLong(&centerOK);
    qint64 oldOffset = m_settings->value("receiver/offset", 0).toLongLong(&offsetOK);
    if (centerOK && offsetOK)
    {
        on_plotter_newDemodFreq(oldCenter + oldOffset, oldOffset);
    }

    ui->plotter->setPlayingIQ(false);

    if (ui->actionDSP->isChecked())
    {
        // restsart DSP
        on_actionDSP_triggered(true);
    }
}


/**
 * Go to a specific offset in the IQ file.
 * @param seek_pos The byte offset from the beginning of the file.
 */
void MainWindow::seekIqFile(qint64 seek_pos)
{
    rx->seek_iq_file((long)seek_pos);
    if(!rx->is_running() && rx->is_playing_iq())
    {
        std::unique_lock<std::mutex> lock(waterfall_background_mutex);
        d_seek_pos = seek_pos;
        waterfall_background_request = MainWindow::WF_RESTART;
        waterfall_background_wake.notify_one();
    }
}

void MainWindow::saveFileRange(const QString& recdir, file_formats fmt, quint64 from_ms, quint64 len_ms)
{
    auto rectime=QDateTime::fromMSecsSinceEpoch(from_ms).toUTC();
    std::string name=makeIQFilename(recdir,fmt,rectime).toStdString();
    rx->save_file_range_ts(from_ms,len_ms,name);
}

void MainWindow::updateSaveProgress(const qint64 saved_ms)
{
    ui->statusBar->showMessage(QString("Saving fragment ... %1 %").arg(saved_ms*100ll/iq_tool->selectionLength()));
    iq_tool->updateSaveProgress(saved_ms);
}

/**
 * IQ tool player waterfall backgroung rendering thread function.
 */
void MainWindow::waterfall_background_func()
{
    int lines=0;
    double ms_per_line = 0.0;
    int maxlines = 0;
    int k = 0;
    int line = 0;
    quint64 seek_pos = 0;
    quint64 old_seek_pos = 0;
    qint64 seek_delta = 0;
    int background_request = MainWindow::WF_NONE;
    int set_request = MainWindow::WF_NONE;
    int last_request = MainWindow::WF_NONE;
    receiver::fft_reader_sptr rd;
    std::unique_lock<std::mutex> lock(waterfall_background_mutex);
    lock.unlock();
    while(1)
    {
        background_request = waterfall_background_request;
        if (background_request == last_request)
        {
            if (background_request != set_request)
                background_request = last_request = waterfall_background_request = set_request;
        }else{
            last_request = background_request;
            seek_pos = d_seek_pos;
        }
        if(background_request == MainWindow::WF_NONE)
        {
            lock.lock();
            waterfall_background_ready.notify_one();
            waterfall_background_wake.wait(lock);
            lock.unlock();
        }
        if(background_request == MainWindow::WF_SET_POS)
        {
            old_seek_pos = seek_pos = d_seek_pos;
            set_request = MainWindow::WF_NONE;
        }
        if(background_request == MainWindow::WF_EXIT)
        {
            return;
        }
        if(background_request == MainWindow::WF_RESTART)
        {
            //update parameters
            lines=0;
            ms_per_line = 0.0;
            ui->plotter->getWaterfallMetrics(lines, ms_per_line);
            seek_delta = seek_pos - old_seek_pos;
            old_seek_pos = seek_pos;
            if(ms_per_line > 0.0)
            {
                qint64 nlines = std::round(double(seek_delta * any_to_any_base::fmt[rx->get_last_format()].nsamples) * 1000.0/double(rx->get_input_rate()*ms_per_line));
                ui->plotter->scrollWaterfall(nlines);
                emit requestPlotterUpdate();
                rd = rx->get_fft_reader(seek_pos, std::bind(plotterWfCbWr, this,
                                std::placeholders::_1,
                                std::placeholders::_2,
                                std::placeholders::_3,
                                std::placeholders::_4,
                                std::placeholders::_5
                                ), waterfall_background_threads);
                quint64 ms_available = rd->ms_available();
                maxlines = std::min(lines, int(ms_available / ms_per_line));
                k = 0;
                if(std::abs(nlines) > lines)
                    line = 0;
                else{
                    if(nlines>=0)
                        line=0;
                    else
                        line=lines+nlines;
                }
                set_request = MainWindow::WF_RUNNING;
            }else{
                rd.reset();
                set_request = MainWindow::WF_NONE;
            }
        }
        if(background_request == MainWindow::WF_RUNNING)
        {
            if(k<=lines)
            {
                if(line<=maxlines)
                {
                    rd->get_iq_fft_data(line * ms_per_line, line);
                }else{
                    ui->plotter->drawBlackWaterfallLine(line);
                }
            }
            k++;
            line++;
            if(line>=lines)
                line = 0;
            if(k >= lines && background_request == MainWindow::WF_RUNNING)
            {
                rd->wait();
                emit requestPlotterUpdate();
                set_request = MainWindow::WF_NONE;
            }
        }
        if(background_request == MainWindow::WF_STOP)
        {
            //FIXME: Is it better to fill remaining lines with black color?
            emit requestPlotterUpdate();
            if(rd)
                rd->wait();
            set_request = MainWindow::WF_NONE;
        }
        if(background_request > MainWindow::WF_EXIT)
            set_request = MainWindow::WF_NONE;
    }
}

void MainWindow::plotterWfCbWr(MainWindow *self, int line, gr_complex* data, float *tmpbuf, unsigned n, quint64 ts)
{
    self->plotterWfCb(line, data, tmpbuf, n, ts);
}

void MainWindow::plotterWfCb(int line, gr_complex* data, float *tmpbuf, unsigned n, quint64 ts)
{
    if(n > 0)
    {
        if(line==0)
        {
            iqFftToMag(n,data,d_realFftData);
            ui->plotter->drawOneWaterfallLine(line, d_realFftData, n, ts);
        }else{
            iqFftToMag(n,data,tmpbuf);
            ui->plotter->drawOneWaterfallLine(line, tmpbuf, n, ts);
        }
        if((line & 15) == 0)
            emit requestPlotterUpdate();
    }
}

/**
 * Pltter forced update slot to make it possible to trigger plotter update from background thread.
 */
void MainWindow::plotterUpdate()
{
    ui->plotter->update();
}

void MainWindow::triggerIQFftRedraw(bool resume)
{
    if(d_fft_redraw_susended && resume)
        d_fft_redraw_susended = false;
    if(!rx->is_running() && rx->is_playing_iq() && !d_fft_redraw_susended)
    {
        std::unique_lock<std::mutex> lock(waterfall_background_mutex);
        waterfall_background_request = MainWindow::WF_RESTART;
        waterfall_background_wake.notify_one();
    }
}

void MainWindow::stopIQFftRedraw(bool suspend)
{
    std::unique_lock<std::mutex> lock(waterfall_background_mutex);
    d_fft_redraw_susended = suspend;
    while(waterfall_background_request != MainWindow::WF_NONE)
    {
        waterfall_background_request = MainWindow::WF_STOP;
        waterfall_background_ready.wait_for(lock, 100ms);
    }
}

/** FFT size has changed. */
void MainWindow::iqFftSizeObserver(c_id, const c_def::v_union & v)
{
    //Prevent crash when FFT size is changed during waterfall background update
    stopIQFftRedraw();
    d_fft_size = v;
    qDebug() << "Changing baseband FFT size to" << d_fft_size;
    rx->set_iq_fft_size(d_fft_size);
    for (int i = 0; i < d_fft_size; i++)
        d_iirFftData[i] = -140.0;  // dBFS
    uiDockFft->updateInfoLabels(d_fft_rate, d_fft_size);//FIXME
    triggerIQFftRedraw();
}

/** Baseband FFT rate has changed. */
void MainWindow::iqFftRateObserver(c_id, const c_def::v_union & v)
{
    int interval;
    d_fft_rate = v;

    if (d_fft_rate == 0)
    {
        interval = 36e7; // 100 hours
        ui->plotter->setRunningState(false);
        rx->set_iq_fft_enabled(false);
    }
    else
    {
        interval = 1000 / d_fft_rate;

        ui->plotter->setFftRate(d_fft_rate);
        if (iq_fft_timer->isActive())
            ui->plotter->setRunningState(true);
        rx->set_iq_fft_enabled(true);
    }

    if (interval > 0 && iq_fft_timer->isActive())
        iq_fft_timer->setInterval(interval);

    uiDockFft->updateInfoLabels(d_fft_rate, d_fft_size);//FIXME
    set_gui(C_FFT_TIMESPAN_LABEL,double(ui->plotter->getWfTimeRes()*1e-3));
    triggerIQFftRedraw();
}

void MainWindow::iqFftWindowObserver(c_id, const c_def::v_union & v)
{
    c_def::v_union corr;
    get_gui(C_FFT_WINDOW_CORR,corr);
    rx->set_iq_fft_window(v, corr);
    triggerIQFftRedraw();
}

void MainWindow::iqFftWindowCorrectionObserver(c_id, const c_def::v_union & v)
{
    c_def::v_union win;
    get_gui(C_FFT_WINDOW,win);
    rx->set_iq_fft_window(win, v);
    triggerIQFftRedraw();
}

/** Waterfall time span has changed. */
void MainWindow::wfTimeSpanObserver(c_id, const c_def::v_union & v)
{
    // set new time span, then send back new resolution to be shown by GUI label
    ui->plotter->setWaterfallSpan(qint64(v));
    set_gui(C_FFT_TIMESPAN_LABEL,double(ui->plotter->getWfTimeRes()*1e-3));
    triggerIQFftRedraw();
}

void MainWindow::setWfSize()
{
    set_gui(C_FFT_TIMESPAN_LABEL,double(ui->plotter->getWfTimeRes()*1e-3));
    triggerIQFftRedraw();
}

void MainWindow::bandPlanObserver(c_id, const c_def::v_union & v)
{
    ui->plotter->toggleBandPlan(v);
}

/**
 * @brief Vertical split between waterfall and pandapter changed.
 * @param pct_wf The percentage of the waterfall.
 */
void MainWindow::iqFftSplitObserver(c_id, const c_def::v_union & pct_wf)
{
    if ((int(pct_wf) >= 0) && (int(pct_wf) <= 100))
    {
        stopIQFftRedraw();
        ui->plotter->setPercent2DScreen(pct_wf);
        triggerIQFftRedraw();
    }
}

void MainWindow::iqFftAvgObserver(c_id, const c_def::v_union & v)
{
    int value=int(v);
    float avg = std::pow(10.0f, -value / 20.0f);
    set_gui(C_FFT_AVG_LABEL,1.0f/avg);
    if ((avg >= 0.0f) && (avg <= 1.0f))
        d_fftAvg = avg;
}

/** Audio FFT rate has changed. */
void MainWindow::setAudioFftRate(int fps)
{
    auto interval = 1000 / fps;

    if (interval < 10)
        return;

    if (audio_fft_timer->isActive())
        audio_fft_timer->setInterval(interval);
}

void  MainWindow::fftZoomLevelObserver(c_id, const c_def::v_union & v)
{
    ui->plotter->blockSignals(true);
    ui->plotter->zoomOnXAxis(v);
    ui->plotter->blockSignals(false);
    set_gui(C_PLOT_ZOOM_LABEL,v);
    triggerIQFftRedraw();
}

void  MainWindow::on_plotter_newZoomLevel(float level)
{
    set_gui(C_PLOT_ZOOM,level,false);
    set_gui(C_PLOT_ZOOM_LABEL,level);
    triggerIQFftRedraw();
}

void MainWindow::setFftCenterFreq(qint64 f)
{
    triggerIQFftRedraw();
}

void MainWindow::plotResetObserver(c_id, const c_def::v_union &)
{
    ui->plotter->resetHorizontalZoom();
}

void MainWindow::plotDemodObserver(c_id, const c_def::v_union &)
{
    ui->plotter->moveToDemodFreq();
    triggerIQFftRedraw();
}

void MainWindow::plotCenterObserver(c_id, const c_def::v_union & v)
{
    ui->plotter->moveToCenterFreq();
    triggerIQFftRedraw();
}

void MainWindow::wfColormapObserver(c_id, const c_def::v_union & v)
{
    auto colormap = QString::fromStdString(v);
    ui->plotter->setWfColormap(colormap);
    uiDockAudio->setWfColormap(colormap);
    uiDockProbe->setWfColormap(colormap);
    triggerIQFftRedraw();
}

void MainWindow::wfBgThreadsObserver(c_id, const c_def::v_union & v)
{
    waterfall_background_threads = v;
}

void MainWindow::on_plotter_pandapterRangeChanged(float lo, float hi)
{
    set_gui(C_FFT_PAND_MIN_DB,int(lo),false);
    set_gui(C_FFT_PAND_MAX_DB,int(hi),false);
    c_def::v_union locked(0);
    get_gui(C_FFT_RANGE_LOCKED,locked);
    if(locked)
    {
        set_gui(C_FFT_WF_MIN_DB,int(lo),false);
        set_gui(C_FFT_WF_MAX_DB,int(hi),false);
        ui->plotter->setWaterfallRange(lo, hi);
    }
    triggerIQFftRedraw();
}

void MainWindow::fftMinDbObserver(c_id id, const c_def::v_union & value)
{
    c_def::v_union max(0);
    if(id==C_FFT_PAND_MIN_DB)
    {
        get_gui(C_FFT_PAND_MAX_DB,max);
        ui->plotter->setPandapterRange(int(value), int(max));
    }else{
        get_gui(C_FFT_WF_MAX_DB,max);
        ui->plotter->setWaterfallRange(int(value), int(max));
    }
    c_def::v_union locked(0);
    get_gui(C_FFT_RANGE_LOCKED,locked);
    if(locked)
    {
        c_def::v_union min(0);
        ui->plotter->setWaterfallRange(int(value), int(max));
        if(id == C_FFT_PAND_MIN_DB)
        {
            get_gui(C_FFT_WF_MIN_DB,min);
            set_gui(C_FFT_WF_MIN_DB,value,false);
        }else{
            get_gui(C_FFT_PAND_MIN_DB,min);
            set_gui(C_FFT_PAND_MIN_DB,value,false);
        }
    }
    triggerIQFftRedraw();
}

void MainWindow::fftMaxDbObserver(c_id id, const c_def::v_union & value)
{
    c_def::v_union min(0);
    if(id==C_FFT_PAND_MAX_DB)
    {
        get_gui(C_FFT_PAND_MIN_DB,min);
        ui->plotter->setPandapterRange(int(min), int(value));
    }else{
        get_gui(C_FFT_WF_MIN_DB,min);
        ui->plotter->setWaterfallRange(int(min), int(value));
    }
    c_def::v_union locked(0);
    get_gui(C_FFT_RANGE_LOCKED,locked);
    if(locked)
    {
        c_def::v_union max(0);
        ui->plotter->setWaterfallRange(int(min), int(value));
        if(id == C_FFT_PAND_MAX_DB)
        {
            get_gui(C_FFT_WF_MAX_DB,max);
            set_gui(C_FFT_WF_MAX_DB,value,false);
        }else{
            get_gui(C_FFT_PAND_MAX_DB,max);
            set_gui(C_FFT_PAND_MAX_DB,value,false);
        }
    }
    triggerIQFftRedraw();
}

void MainWindow::fftLockObserver(c_id, const c_def::v_union & v)
{
    if(!bool(v))
        return;
    c_def::v_union min,max;
    get_gui(C_FFT_PAND_MIN_DB,min);
    set_gui(C_FFT_WF_MIN_DB,min,false);
    get_gui(C_FFT_PAND_MAX_DB,max);
    set_gui(C_FFT_WF_MAX_DB,max,false);
    ui->plotter->setWaterfallRange(int(min), int(max));
}

/** Set FFT plot color. */
void MainWindow::fftColorObserver(c_id, const c_def::v_union & v)
{
    auto color = QColor::fromRgb(qint64(v));
    ui->plotter->setFftPlotColor(color);
    uiDockAudio->setFftColor(color);
    uiDockProbe->setFftColor(color);
    triggerIQFftRedraw();
}

/** Enable/disable filling the aread below the FFT plot. */
void MainWindow::fftFillObserver(c_id, const c_def::v_union & enable)
{
    ui->plotter->setFftFill(enable);
    uiDockAudio->setFftFill(enable);
    uiDockProbe->setFftFill(enable);
    triggerIQFftRedraw();
}

void MainWindow::fftPeakHoldObserver(c_id, const c_def::v_union & enable)
{
    ui->plotter->setPeakHold(enable);
}

void MainWindow::peakDetectionObserver(c_id, const c_def::v_union & enable)
{
    ui->plotter->setPeakDetection(enable ,2);
}

/**
 * @brief Start/Stop DSP processing.
 * @param checked Flag indicating whether DSP processing should be ON or OFF.
 *
 * This slot is executed when the actionDSP is toggled by the user. This can
 * either be via the menu bar or the "power on" button in the main toolbar or
 * by remote control.
 */
void MainWindow::on_actionDSP_triggered(bool checked)
{
    remote->setReceiverStatus(checked);

    if (checked)
    {
        /* Make shure, that background thread will not interfere normal waterfall rendering */
        stopIQFftRedraw();
        /* start receiver */
        rx->start();

        /* start GUI timers */
        meter_timer->start(100);

        if (d_fft_rate)
        {
            iq_fft_timer->start(1000/d_fft_rate);
            ui->plotter->setRunningState(true);
        }
        else
        {
            iq_fft_timer->start(36e7); // 100 hours
            ui->plotter->setRunningState(false);
        }
        iq_tool->setRunningState(true);

        audio_fft_timer->start(40);

        /* update menu text and button tooltip */
        ui->actionDSP->setToolTip(tr("Stop DSP processing"));
        ui->actionDSP->setText(tr("Stop DSP"));
    }
    else
    {
        {
            std::unique_lock<std::mutex> lock(waterfall_background_mutex);
            waterfall_background_request = MainWindow::WF_SET_POS;
            waterfall_background_wake.notify_one();
        }
        /* stop GUI timers */
        meter_timer->stop();
        iq_fft_timer->stop();
        audio_fft_timer->stop();

        /* stop receiver */
        rx->stop();

        /* update menu text and button tooltip */
        ui->actionDSP->setToolTip(tr("Start DSP processing"));
        ui->actionDSP->setText(tr("Start DSP"));

        ui->plotter->setRunningState(false);
        iq_tool->setRunningState(false);
    }

    ui->actionDSP->setChecked(checked); //for remote control

}

void MainWindow::on_plotter_setPlaying(bool state)
{
    if(ui->actionDSP->isChecked() && rx->is_playing_iq())
    {
        if(state)
        {
            /* Make shure, that background thread will not interfere normal waterfall rendering */
            stopIQFftRedraw();
            /* start receiver */
            rx->start();

            /* start GUI timers */
            meter_timer->start(100);

            if (d_fft_rate)
            {
                iq_fft_timer->start(1000/d_fft_rate);
                ui->plotter->setRunningState(true);
            }
            else
            {
                iq_fft_timer->start(36e7); // 100 hours
                ui->plotter->setRunningState(false);
            }

            audio_fft_timer->start(40);
        }
        else
        {
            {
                std::unique_lock<std::mutex> lock(waterfall_background_mutex);
                waterfall_background_request = MainWindow::WF_SET_POS;
                waterfall_background_wake.notify_one();
            }
            /* stop GUI timers */
            meter_timer->stop();
            iq_fft_timer->stop();
            audio_fft_timer->stop();

            /* stop receiver */
            rx->stop();

            ui->plotter->setRunningState(false);
        }
    }
}

/**
 * @brief Action: I/O device configurator triggered.
 *
 * This slot is activated when the user selects "I/O Devices" in the
 * menu. It activates the I/O configurator and if the user closes the
 * configurator using the OK button, the new configuration is read and
 * sent to the receiver.
 */
int MainWindow::on_actionIoConfig_triggered()
{
    qDebug() << "Configure I/O devices.";

    storeSession();
    auto *ioconf = new CIoConfig(m_settings, devList);
    auto confres = ioconf->exec();

    if (confres == QDialog::Accepted)
    {
        bool dsp_running = ui->actionDSP->isChecked();

        if (dsp_running)
            // suspend DSP while we reload settings
            on_actionDSP_triggered(false);

        // Refresh LNB LO in dock widget, otherwise changes will be lost
        c_def::v_union tmp;
        auto def=c_def::all()[C_LNB_LO];
        m_settings->beginGroup(QString::fromStdString(def.v3_config_group()));
        loadSetting(m_settings, def,tmp);
        set_gui(C_LNB_LO,tmp);
        m_settings->endGroup();
        storeSession();
        loadConfig(m_settings->fileName(), false, false);

        if (dsp_running)
            // restsart DSP
            on_actionDSP_triggered(true);
    }

    delete ioconf;

    return confres;
}


/** Run first time configurator. */
int MainWindow::firstTimeConfig()
{
    qDebug() << __func__;

    auto *ioconf = new CIoConfig(m_settings, devList);
    auto confres = ioconf->exec();

    if (confres == QDialog::Accepted)
        loadConfig(m_settings->fileName(), false, false);

    delete ioconf;

    return confres;
}


/** Load configuration activated by user. */
void MainWindow::on_actionLoadSettings_triggered()
{
    auto cfgfile = QFileDialog::getOpenFileName(this, tr("Load settings"),
                                           m_last_dir.isEmpty() ? m_cfg_dir : m_last_dir,
                                           tr("Settings (*.conf)"));

    qDebug() << "File to open:" << cfgfile;

    if (cfgfile.isEmpty())
        return;

    if (!cfgfile.endsWith(".conf", Qt::CaseSensitive))
        cfgfile.append(".conf");

    loadConfig(cfgfile, cfgfile != m_settings->fileName(), cfgfile != m_settings->fileName());

    // store last dir
    QFileInfo fi(cfgfile);
    if (m_cfg_dir != fi.absolutePath())
        m_last_dir = fi.absolutePath();
}

/** Save configuration activated by user. */
void MainWindow::on_actionSaveSettings_triggered()
{
    auto cfgfile = QFileDialog::getSaveFileName(this, tr("Save settings"),
                                           m_last_dir.isEmpty() ? m_cfg_dir : m_last_dir,
                                           tr("Settings (*.conf)"));

    qDebug() << "File to save:" << cfgfile;

    if (cfgfile.isEmpty())
        return;

    if (!cfgfile.endsWith(".conf", Qt::CaseSensitive))
        cfgfile.append(".conf");

    storeSession();
    saveConfig(cfgfile);

    // store last dir
    QFileInfo fi(cfgfile);
    if (m_cfg_dir != fi.absolutePath())
        m_last_dir = fi.absolutePath();
}

/** Show I/Q player. */
void MainWindow::on_actionIqTool_triggered()
{
    iq_tool->show();
}

/* CPlotter::NewDemodFreq() is emitted */
void MainWindow::on_plotter_seekIQ(qint64 ts)
{
    struct receiver::iq_tool_stats iq_stats;

    if(ts >= 0 && rx->is_playing_iq())
    {
        uint64_t res_point = 0;
        rx->seek_iq_file_ts(ts, res_point);
        if(!rx->is_running() && rx->is_playing_iq())
        {
            rx->get_iq_tool_stats(iq_stats);
            iq_tool->updateStats(iq_stats.failed, iq_stats.buffer_usage, iq_stats.file_pos);
            std::unique_lock<std::mutex> lock(waterfall_background_mutex);
            d_seek_pos = res_point;
            waterfall_background_request = MainWindow::WF_RESTART;
            waterfall_background_wake.notify_one();
        }
    }
}

/* CPlotter::NewDemodFreq() is emitted */
void MainWindow::on_plotter_newDemodFreq(qint64 freq, qint64 delta)
{
    // set RX filter
    if (delta != qint64(rx->get_filter_offset()))
    {
        rx->set_filter_offset((double) delta);
        updateFrequencyRange();
    }

    setNewFrequency(freq);
}

/* CPlotter::NewDemodFreqLoad() is emitted */
/* tune and load demodulator settings */
void MainWindow::on_plotter_newDemodFreqLoad(qint64 freq, qint64 delta)
{
    // set RX filter
    if (delta != qint64(rx->get_filter_offset()))
    {
        rx->set_filter_offset((double) delta);
        updateFrequencyRange();
    }

    QList<BookmarkInfo> tags =
        Bookmarks::Get().getBookmarksInRange(freq, freq);
    if (tags.size() > 0)
    {
        onBookmarkActivated(tags.first());
    }
    else
        setNewFrequency(freq);
}

/* CPlotter::NewDemodFreqLoad() is emitted */
/* new demodulator here */
void MainWindow::on_plotter_newDemodFreqAdd(qint64 freq, qint64 delta)
{
    vfo::sptr found = rx->find_vfo(freq - d_lnb_lo);
    if (!found)
        on_actionAddDemodulator_triggered();
    else
    {
        rxSpinBox->setValue(found->get_index());
        rxSpinBox_valueChanged(found->get_index());
    }
    on_plotter_newDemodFreqLoad(freq, delta);
}

/* CPlotter::NewfilterFreq() is emitted or bookmark activated */
void MainWindow::on_plotter_newFilterFreq(int low, int high)
{   /* parameter correctness will be checked in receiver class */
    c_def::v_union filter_shape;
    rx->get_value(C_FILTER_SHAPE, filter_shape);
    rx->set_value(C_FILTER_LO, low);
    rx->set_value(C_FILTER_HI, high);
    rx->set_value(C_FILTER_SHAPE, filter_shape);
    set_gui(C_FILTER_LO, low, true);
    set_gui(C_FILTER_HI, high, true);

    /* Update filter range of plotter, in case this slot is triggered by
     * switching to a bookmark */
    ui->plotter->setHiLowCutFrequencies(low, high);
    remote->setPassband(low, high);

}

/** Full screen button or menu item toggled. */
void MainWindow::on_actionFullScreen_triggered(bool checked)
{
    if (checked)
    {
        ui->statusBar->hide();
        showFullScreen();
    }
    else
    {
        ui->statusBar->show();
        showNormal();
    }
}

/** Remote control button (or menu item) toggled. */
void MainWindow::on_actionRemoteControl_triggered(bool checked)
{
    if (checked)
        remote->start_server();
    else
        remote->stop_server();
}

/** Remote control configuration button (or menu item) clicked. */
void MainWindow::on_actionRemoteConfig_triggered()
{
    auto *rcs = new RemoteControlSettings();

    rcs->setPort(remote->getPort());
    rcs->setHosts(remote->getHosts());

    if (rcs->exec() == QDialog::Accepted)
    {
        remote->setPort(rcs->getPort());
        remote->setHosts(rcs->getHosts());
    }

    delete rcs;
}


#define DATA_BUFFER_SIZE 48000

/**
 * AFSK1200 decoder action triggered.
 *
 * This slot is called when the user activates the AFSK1200
 * action. It will create an AFSK1200 decoder window and start
 * and start pushing data from the receiver to it.
 */
void MainWindow::on_actionAFSK1200_triggered()
{
    if (!d_have_audio)
    {
        QMessageBox msg_box;
        msg_box.setIcon(QMessageBox::Critical);
        msg_box.setText(tr("AFSK1200 decoder requires a demodulator.\n"
                           "Currently, demodulation is switched off "
                           "(Mode->Demod Off)."));
        msg_box.exec();
        return;
    }
    if (dec_afsk1200 != nullptr)
    {
        qDebug() << "AFSK1200 decoder already active.";
        dec_afsk1200->raise();
    }
    else
    {
        qDebug() << "Starting AFSK1200 decoder.";

        /* start sample sniffer */
        if (rx->start_sniffer(22050, DATA_BUFFER_SIZE) == receiver::STATUS_OK)
        {
            dec_afsk1200 = new Afsk1200Win(this);
            connect(dec_afsk1200, SIGNAL(windowClosed()), this, SLOT(afsk1200win_closed()));
            dec_afsk1200->setAttribute(Qt::WA_DeleteOnClose);
            dec_afsk1200->show();

            dec_timer->start(100);
        }
        else
            QMessageBox::warning(this, tr("Gqrx error"),
                                 tr("Error starting sample sniffer.\n"
                                    "Close all data decoders and try again."),
                                 QMessageBox::Ok, QMessageBox::Ok);
    }
}


void MainWindow::setRDSmode(bool mode)
{
    set_gui(C_RDS_ON,mode);
    rx->set_value(C_RDS_ON,mode);
}

/**
 * Destroy AFSK1200 decoder window got closed.
 *
 * This slot is connected to the windowClosed() signal of the AFSK1200 decoder
 * object. We need this to properly destroy the object, stop timeout and clean
 * up whatever need to be cleaned up.
 */
void MainWindow::afsk1200win_closed()
{
    /* stop cyclic processing */
    dec_timer->stop();
    rx->stop_sniffer();

    dec_afsk1200 = nullptr;
}

/** Show DXC Options. */
void MainWindow::on_actionDX_Cluster_triggered()
{
    dxc_options->show();
}

/**
 * Cyclic processing for acquiring samples from receiver and processing them
 * with data decoders (see dec_* objects)
 */
void MainWindow::decoderTimeout()
{
    float buffer[DATA_BUFFER_SIZE];
    unsigned int num;

    rx->get_sniffer_data(&buffer[0], num);
    if (dec_afsk1200)
        dec_afsk1200->process_samples(&buffer[0], num);
}

void MainWindow::rdsOnObserver(const c_id id, const c_def::v_union &value)
{
    remote->setRDSstatus(value);
}

/* AudioDock */
void MainWindow::dockAudioVisibilityChanged(bool visible)
{
    rx->set_audio_fft_enabled(visible);
}

void MainWindow::onBookmarkActivated(BookmarkInfo & bm)
{
    c_def::v_union old_sql;
    setNewFrequency(bm.frequency);
    rx->set_value(C_MODE, bm.get_demod());
    // preserve offset, squelch level, force locked state
    auto old_vfo = rx->get_current_vfo();
    auto old_offset = old_vfo->get_offset();
    old_vfo->get_sql_level(old_sql);
    rx->get_current_vfo()->restore_settings(bm, false);
    old_vfo->set_sql_level(old_sql);
    old_vfo->set_offset(old_offset);
    old_vfo->set_freq_lock(true);
    loadRxToGUI();
}

void MainWindow::onBookmarkActivatedAddDemod(BookmarkInfo & bm)
{
    if (!rx->find_vfo(bm.frequency - d_lnb_lo))
    {
        on_actionAddDemodulator_triggered();
        onBookmarkActivated(bm);
    }
}

void MainWindow::setPassband(int bandwidth)
{
    /* Check if filter is symmetric or not by checking the presets */
    c_def::v_union tmp;
    rx->get_value(C_MODE, tmp);
    Modulations::idx mode = Modulations::idx(int(tmp));
    get_gui(C_FILTER_WIDTH, tmp);
    int preset = tmp;

    int lo, hi;
    Modulations::GetFilterPreset(mode, preset, lo, hi);

    if (lo + hi == 0)
    {
        lo = -bandwidth / 2;
        hi =  bandwidth / 2;
    }
    else if (lo >= 0 && hi >= 0)
    {
        hi = lo + bandwidth;
    }
    else if (lo <= 0 && hi <= 0)
    {
        lo = hi - bandwidth;
    }

    on_plotter_newFilterFreq(lo, hi);
}

/** Launch Gqrx google group website. */
void MainWindow::on_actionUserGroup_triggered()
{
    auto res = QDesktopServices::openUrl(QUrl("https://groups.google.com/g/gqrx",
                                              QUrl::TolerantMode));
    if (!res)
        QMessageBox::warning(this, tr("Error"),
                             tr("Failed to open website:\n"
                                "https://groups.google.com/g/gqrx"),
                             QMessageBox::Close);
}

/**
 * Show ftxt in a dialog window.
 */
void MainWindow::on_actionNews_triggered()
{
    showSimpleTextFile(":/textfiles/news.txt", tr("Release news"));
}

/**
 * Show remote-contol.txt in a dialog window.
 */
void MainWindow::on_actionRemoteProtocol_triggered()
{
    showSimpleTextFile(":/textfiles/remote-control.txt",
                       tr("Remote control protocol"));
}

/**
 * Show kbd-shortcuts.txt in a dialog window.
 */
void MainWindow::on_actionKbdShortcuts_triggered()
{
    showSimpleTextFile(":/textfiles/kbd-shortcuts.txt",
                       tr("Keyboard shortcuts"));
}

/**
 * Show simple text file in a window.
 */
void MainWindow::showSimpleTextFile(const QString &resource_path,
                                    const QString &window_title)
{
    QResource resource(resource_path);
    QFile news(resource.absoluteFilePath());

    if (!news.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Unable to open file: " << news.fileName() <<
                    " because of error " << news.errorString();

        return;
    }

    QTextStream in(&news);
    auto content = in.readAll();
    news.close();

    auto *browser = new QTextBrowser();
    browser->setLineWrapMode(QTextEdit::NoWrap);
    browser->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    browser->append(content);
    browser->adjustSize();

    // scroll to the beginning
    auto cursor = browser->textCursor();
    cursor.setPosition(0);
    browser->setTextCursor(cursor);


    auto *layout = new QVBoxLayout();
    layout->addWidget(browser);

    auto *dialog = new QDialog(this);
    dialog->setWindowTitle(window_title);
    dialog->setLayout(layout);
    dialog->resize(800, 400);
    dialog->exec();

    delete dialog;
    // browser and layout deleted automatically
}

/**
 * @brief Slot for handling loadConfig signals
 * @param cfgfile
 */
void MainWindow::loadConfigSlot(const QString &cfgfile)
{
    loadConfig(cfgfile, cfgfile != m_settings->fileName(), cfgfile != m_settings->fileName());
}

/**
 * @brief Action: About gqrx.
 *
 * This slot is called when the user activates the
 * Help|About menu item (or Gqrx|About on Mac)
 */
void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("About Gqrx"),
        tr("<p>This is Gqrx %1</p>"
           "<p>Copyright (C) 2011-2024 Alexandru Csete & contributors.</p>"
           "<p>Gqrx is a software defined radio (SDR) receiver powered by "
           "<a href='https://www.gnuradio.org/'>GNU Radio</a> and the Qt toolkit. "
           "<p>Gqrx uses the <a href='https://osmocom.org/projects/gr-osmosdr/wiki/GrOsmoSDR'>GrOsmoSDR</a> "
           "input source block and works with any input device supported by it, including "
           "Funcube Dongle, RTL-SDR, Airspy, HackRF, RFSpace, BladeRF and USRP receivers."
           "</p>"
           "<p>You can download the latest version from the "
           "<a href='https://gqrx.dk/'>Gqrx website</a>."
           "</p>"
           "<p>"
           "Gqrx is licensed under the <a href='https://www.gnu.org/licenses/gpl-3.0.html'>GNU General Public License</a>."
           "</p>").arg(VERSION));
}

/**
 * @brief Action: About Qt
 *
 * This slot is called when the user activates the
 * Help|About Qt menu item (or Gqrx|About Qt on Mac)
 */
void MainWindow::on_actionAboutQt_triggered()
{
    QMessageBox::aboutQt(this, tr("About Qt"));
}

void MainWindow::on_actionAddBookmark_triggered()
{
    bool ok=false;
    QString name;
    QStringList tags;
    const qint64 freq = ui->freqCtrl->getFrequency();

    QList<BookmarkInfo> bookmarkFound = Bookmarks::Get().getBookmarksInRange(freq, freq);
    // Create and show the Dialog for a new Bookmark.
    // Write the result into variable 'name'.
    {
        QDialog dialog(this);
        dialog.setWindowTitle("New bookmark");

        auto* LabelAndTextfieldName = new QGroupBox(&dialog);
        auto* label1 = new QLabel("Bookmark name:", LabelAndTextfieldName);
        auto* textfield = new QLineEdit(LabelAndTextfieldName);
        auto *layout = new QHBoxLayout;
        layout->addWidget(label1);
        layout->addWidget(textfield);
        LabelAndTextfieldName->setLayout(layout);

        auto* buttonCreateTag = new QPushButton("Create new Tag", &dialog);

        auto* taglist = new BookmarksTagList(&dialog, false);
        taglist->updateTags();
        taglist->DeselectAll();

        auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                              | QDialogButtonBox::Cancel);
        connect(buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
        connect(buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
        connect(buttonCreateTag, SIGNAL(clicked()), taglist, SLOT(AddNewTag()));

        auto *mainLayout = new QVBoxLayout(&dialog);
        mainLayout->addWidget(LabelAndTextfieldName);
        mainLayout->addWidget(buttonCreateTag);
        mainLayout->addWidget(taglist);
        mainLayout->addWidget(buttonBox);

        if (bookmarkFound.size())
        {
            textfield->setText(bookmarkFound.first().name);
            taglist->setSelectedTags(bookmarkFound.first().tags);
        }
        ok = dialog.exec();
        if (ok)
        {
            name = textfield->text();
            tags = taglist->getSelectedTags();
            qDebug() << "Tags: " << tags;
        }
        else
        {
            name.clear();
            tags.clear();
        }
    }

    // Add new Bookmark to Bookmarks.
    if (ok)
    {
        int i;

        BookmarkInfo info;
        info.restore_settings(*rx->get_current_vfo().get());
        info.frequency = freq;
        info.name=name;
        info.tags.clear();
        if (tags.empty())
            info.tags.append(Bookmarks::Get().findOrAddTag(""));


        for (i = 0; i < tags.size(); ++i)
            info.tags.append(Bookmarks::Get().findOrAddTag(tags[i]));

        //FIXME: implement Bookmarks::replace(&BookmarkInfo, &BookmarkInfo) method
        if (bookmarkFound.size())
        {
            info.set_autostart(bookmarkFound.first().get_autostart());
            Bookmarks::Get().remove(bookmarkFound.first());
        }
        else
            info.set_freq_lock(false);
        Bookmarks::Get().add(info);
        uiDockBookmarks->updateTags();
    }
}

void MainWindow::on_actionAddDemodulator_triggered()
{
    ui->plotter->addVfo(rx->get_current_vfo());
    int n = rx->add_rx();
    ui->plotter->setCurrentVfo(rx->get_rx_count() - 1);
    rxSpinBox->setMaximum(rx->get_rx_count() - 1);
    rxSpinBox->setValue(n);
}

void MainWindow::on_actionRemoveDemodulator_triggered()
{
    int old_current = rx->get_current();
    if (old_current != rx->get_rx_count() - 1)
        ui->plotter->removeVfo(rx->get_vfo(rx->get_rx_count() - 1));
    int n = rx->delete_rx();
    rxSpinBox->setValue(n);
    rxSpinBox->setMaximum(rx->get_rx_count() - 1);
    loadRxToGUI();
    if (old_current != n)
        ui->plotter->removeVfo(rx->get_vfo(n));
    ui->plotter->setCurrentVfo(n);
}

void MainWindow::rxSpinBox_valueChanged(int i)
{
    if (i == rx->get_current())
        return;
    ui->plotter->blockUpdates(true);
    ui->plotter->addVfo(rx->get_current_vfo());
    int n = rx->select_rx(i);
    ui->plotter->removeVfo(rx->get_current_vfo());
    if (n == receiver::STATUS_OK)
        loadRxToGUI();
    ui->plotter->setCurrentVfo(i);
    ui->plotter->blockUpdates(false);
}

void MainWindow::on_plotter_selectVfo(int i)
{
    rxSpinBox->setValue(i);
}

void MainWindow::loadRxToGUI()
{
    auto rf_freq = rx->get_rf_freq();
    auto new_offset = rx->get_filter_offset();
    auto rx_freq = (double)(rf_freq + d_lnb_lo + new_offset);

    auto mode_idx = rx->get_demod();

    ui->plotter->setFilterOffset(new_offset);
    set_gui(C_VFO_FREQUENCY, rx_freq * 1e-3);
    uiDockRxOpt->setFilterOffset(new_offset);

    ui->freqCtrl->setFrequency(rx_freq);
    uiDockBookmarks->setNewFrequency(rx_freq);
    remote->setNewFrequency(rx_freq);
    uiDockAudio->setRxFrequency(rx_freq);

    auto defs=c_def::all();
    for(int j=0;j<C_COUNT;j++)
    {
        const auto & def=defs[j];
        if(!def.readable())
            continue;
        if(def.scope()==S_VFO)
        {
            const c_id id=c_id(j);
            c_def::v_union v(c_def::all()[id].def());
            rx->get_value(id,v);
            set_gui(id,v,true);
        }
    }

    updateDemodGUIRanges(mode_idx);
}

void MainWindow::addClusterSpot()
{
    ui->plotter->updateOverlay();
}

void MainWindow::frequencyFocusShortcut()
{
    ui->freqCtrl->setFrequencyFocus();
}

void MainWindow::rxOffsetZeroShortcut()
{
    uiDockRxOpt->setFilterOffset(0);
}

void MainWindow::checkDXCSpotTimeout()
{
    DXCSpots::Get().checkSpotTimeout();
}
