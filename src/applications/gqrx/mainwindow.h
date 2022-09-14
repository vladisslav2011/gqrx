/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2014 Alexandru Csete OZ9AEC.
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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QColor>
#include <QMainWindow>
#include <QPointer>
#include <QSettings>
#include <QString>
#include <QTimer>
#include <QMessageBox>
#include <QFileDialog>
#include <QSvgWidget>
#include <QSpinBox>

#include "qtgui/dockrxopt.h"
#include "qtgui/dockaudio.h"
#include "qtgui/dockinputctl.h"
#include "qtgui/dockfft.h"
#include "qtgui/dockbookmarks.h"
#include "qtgui/dockprobe.h"
#include "qtgui/dockrds.h"
#include "qtgui/afsk1200win.h"
#include "qtgui/iq_tool.h"
#include "qtgui/dxc_options.h"

#include "applications/gqrx/recentconfig.h"
#include "applications/gqrx/remote_control.h"
#include "applications/gqrx/receiver.h"

namespace Ui {
    class MainWindow;  /*! The main window UI */
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

signals:
    void sigAudioRecEvent(const QString filename, bool is_running);

public:
    explicit MainWindow(const QString& cfgfile, bool edit_conf, QWidget *parent = nullptr);
    ~MainWindow() override;

    bool loadConfig(const QString& cfgfile, bool check_crash, bool restore_mainwindow);
    bool saveConfig(const QString& cfgfile);
    void readRXSettings(int ver, double actual_rate);
    void storeSession();

    bool configOk; /*!< Main app uses this flag to know whether we should abort or continue. */

public slots:
    void setNewFrequency(qint64 rx_freq);

private:
    Ui::MainWindow *ui;

    QPointer<QSettings> m_settings;  /*!< Application wide settings. */
    QString             m_cfg_dir;   /*!< Default config dir, e.g. XDG_CONFIG_HOME. */
    QString             m_last_dir;
    RecentConfig       *m_recent_config; /* Menu File Recent config */

    qint64 d_lnb_lo;  /* LNB LO in Hz. */
    qint64 d_hw_freq;
    qint64 d_hw_freq_start{};
    qint64 d_hw_freq_stop{};

    bool d_ignore_limits;
    bool d_auto_bookmarks;

    Modulations::filter_shape d_filter_shape;
    std::complex<float>* d_fftData;
    float          *d_realFftData;
    float          *d_iirFftData;
    float           d_fftAvg;      /*!< FFT averaging parameter set by user (not the true gain). */

    bool d_have_audio;  /*!< Whether we have audio (i.e. not with demod_off. */

    /* dock widgets */
    DockRxOpt      *uiDockRxOpt;
    DockAudio      *uiDockAudio;
    DockInputCtl   *uiDockInputCtl;
    DockFft        *uiDockFft;
    DockBookmarks  *uiDockBookmarks;
    DockProbe      *uiDockProbe;
    DockRDS        *uiDockRDS;

    CIqTool        *iq_tool;
    DXCOptions     *dxc_options;


    /* data decoders */
    Afsk1200Win    *dec_afsk1200;
    bool            dec_rds{};

    QTimer   *dec_timer;
    QTimer   *meter_timer;
    QTimer   *iq_fft_timer;
    QTimer   *audio_fft_timer;
    QTimer   *rds_timer;

    receiver *rx;

    RemoteControl *remote;

    std::map<QString, QVariant> devList;

    QSpinBox *rxSpinBox;
    // dummy widget to enforce linking to QtSvg
    QSvgWidget      *qsvg_dummy;

private:
    void updateHWFrequencyRange(bool ignore_limits);
    void updateFrequencyRange();
    void updateGainStages(bool read_from_device);
    void showSimpleTextFile(const QString &resource_path,
                            const QString &window_title);
    /* key shortcuts */
    void frequencyFocusShortcut();
    void rxOffsetZeroShortcut();
    void audioRecEventEmitter(std::string filename, bool is_running);
    static void audio_rec_event(MainWindow *self, std::string filename, bool is_running);
    void loadRxToGUI();

private slots:
    /* RecentConfig */
    void loadConfigSlot(const QString &cfgfile);

    /* rf */
    void setLnbLo(double freq_mhz);
    void setAntenna(const QString& antenna);

    /* baseband receiver */
    void setFilterOffset(qint64 freq_hz);
    void setGain(const QString& name, double gain);
    void setAutoGain(bool enabled);
    void setFreqCorr(double ppm);
    void setIqSwap(bool reversed);
    void setDcCancel(bool enabled);
    void setIqBalance(bool enabled);
    void setIgnoreLimits(bool ignore_limits);
    void setFreqCtrlReset(bool enabled);
    void setInvertScrolling(bool enabled);
    void setAutoBookmarks(bool enabled);
    void selectDemod(const QString& demod);
    void selectDemod(Modulations::idx index);
    void updateDemodGUIRanges();
    void setFmMaxdev(float max_dev);
    void setFmEmph(double tau);
    void setAmDcr(bool enabled);
    void setCwOffset(int offset);
    void setAmSyncDcr(bool enabled);
    void setAmSyncPllBw(float pll_bw);
    void setAgcOn(bool agc_on);
    void setAgcHang(int hang);
    void setAgcTargetLevel(int targetLevel);
    void setAgcAttack(int attack);
    void setAgcDecay(int msec);
    void setAgcMaxGain(int gain);
    void setAgcPanning(int panning);
    void setAgcPanningAuto(bool panningAuto);
    void setNoiseBlanker(int nbid, bool on, float threshold);
    void setSqlLevel(double level_db);
    double setSqlLevelAuto(bool global);
    void resetSqlLevelGlobal();
    void setAudioGain(float gain);
    void setAudioMute(bool mute, bool global);
    void setPassband(int bandwidth);
    void setFreqLock(bool lock, bool all);
    void setChanelizer(int n);

    /* audio recording and playback */
    void recDirChanged(const QString dir);
    void recSquelchTriggeredChanged(const bool enabled);
    void recMinTimeChanged(const int time_ms);
    void recMaxGapChanged(const int time_ms);
    void startAudioRec();
    void stopAudioRec();
    void audioRecEvent(const QString filename, bool is_running);
    void startAudioPlayback(const QString& filename);
    void stopAudioPlayback();
    void copyRecSettingsToAllVFOs();

    /* audio UDP streaming */
    void audioStreamHostChanged(const QString udp_host);
    void audioStreamPortChanged(const int udp_port);
    void audioStreamStereoChanged(const bool udp_stereo);
    void startAudioStream();
    void stopAudioStreaming();

    void audioDedicatedDevChanged(bool enabled, std::string name);

    /* I/Q playback and recording*/
    QString makeIQFilename(const QString& recdir, file_formats fmt, const QDateTime ts);
    void startIqRecording(const QString& recdir, file_formats fmt, int buffers_max);
    void stopIqRecording();
    void startIqPlayback(const QString& filename, float samprate,
                         qint64 center_freq, file_formats fmt,
                         qint64 time_ms,
                         int buffers_max, bool repeat);
    void stopIqPlayback();
    void seekIqFile(qint64 seek_pos);

    /* FFT settings */
    void setIqFftSize(int size);
    void setIqFftRate(int fps);
    void setIqFftWindow(int type);
    void setIqFftSplit(int pct_wf);
    void setIqFftAvg(float avg);
    void setAudioFftRate(int fps);
    void setFftColor(const QColor& color);
    void setFftFill(bool enable);
    void setPeakDetection(bool enabled);
    void setFftPeakHold(bool enable);
    void setWfTimeSpan(quint64 span_ms);
    void setWfSize();

    /* FFT plot */
    void on_plotter_newDemodFreq(qint64 freq, qint64 delta, qint64 ts);    /*! New demod freq (aka. filter offset). */
    void on_plotter_newDemodFreqLoad(qint64 freq, qint64 delta, qint64 ts);/* tune and load demodulator settings */
    void on_plotter_newDemodFreqAdd(qint64 freq, qint64 delta, qint64 ts); /* new demodulator here */
    void on_plotter_newFilterFreq(int low, int high);    /*! New filter width */
    void on_plotter_selectVfo(int i);

    /* RDS */
    void setRdsDecoder(bool checked);

    /* Bookmarks */
    void onBookmarkActivated(BookmarkInfo & bm);
    void onBookmarkActivatedAddDemod(BookmarkInfo & bm);

    /* DXC Spots */
    void updateClusterSpots();

    /* menu and toolbar actions */
    void on_actionDSP_triggered(bool checked);
    int  on_actionIoConfig_triggered();
    void on_actionLoadSettings_triggered();
    void on_actionSaveSettings_triggered();
    void on_actionSaveWaterfall_triggered();
    void on_actionIqTool_triggered();
    void on_actionFullScreen_triggered(bool checked);
    void on_actionRemoteControl_triggered(bool checked);
    void on_actionRemoteConfig_triggered();
    void on_actionAFSK1200_triggered();
    void on_actionUserGroup_triggered();
    void on_actionNews_triggered();
    void on_actionRemoteProtocol_triggered();
    void on_actionKbdShortcuts_triggered();
    void on_actionAbout_triggered();
    void on_actionAboutQt_triggered();
    void on_actionAddBookmark_triggered();
    void on_actionDX_Cluster_triggered();
    void on_actionAddDemodulator_triggered();
    void on_actionRemoveDemodulator_triggered();
    void rxSpinBox_valueChanged(int i);


    /* window close signals */
    void afsk1200win_closed();
    int  firstTimeConfig();

    /* cyclic processing */
    void decoderTimeout();
    void meterTimeout();
    void iqFftTimeout();
    void audioFftTimeout();
    void rdsTimeout();
};

#endif // MAINWINDOW_H
