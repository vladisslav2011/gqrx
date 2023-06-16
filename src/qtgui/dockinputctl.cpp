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
#include <QDebug>
#include <QComboBox>
#include "dockinputctl.h"
#include "ui_dockinputctl.h"

DockInputCtl::DockInputCtl(QWidget * parent) :
    QDockWidget(parent),
    ui(new Ui::DockInputCtl)
{
    ui->setupUi(this);

    gainLayout = new QGridLayout();
    grid_init(ui->gridLayout,ui->gridLayout->rowCount(),0/*ui->gridLayout->columnCount()*/);
    ui->gridLayout->addLayout(gainLayout,2,0,1,3);
    set_observer(C_IQ_AGC_ACK, &DockInputCtl::hwagcObserver);
}

DockInputCtl::~DockInputCtl()
{
    delete ui;
}

void DockInputCtl::readSettings(QSettings * settings)
{
    // gains are stored as a QMap<QString, QVariant(int)>
    // note that we store the integer values, i.e. dB*10
    if (settings->contains("input/gains"))
    {
        QMap <QString, QVariant>    allgains;
        QString     gain_name;
        double      gain_value;

        allgains = settings->value("input/gains").toMap();
        QMapIterator <QString, QVariant> gain_iter(allgains);

        while (gain_iter.hasNext())
        {
            gain_iter.next();

            gain_name = gain_iter.key();
            gain_value = 0.1 * (double)(gain_iter.value().toInt());
            if (setGain(gain_name, gain_value))
                emit gainChanged(gain_name, gain_value);
        }
    }

}

void DockInputCtl::saveSettings(QSettings * settings)
{
    // gains are stored as a QMap<QString, QVariant(int)>
    // note that we store the integer values, i.e. dB*10
    QMap <QString, QVariant> gains;
    getGains(&gains);
    if (gains.empty())
        settings->remove("input/gains");
    else
        settings->setValue("input/gains", gains);

}

/**
 * @brief Set new value of a specific gain.
 * @param name The name of the gain to change.
 * @param value The new value.
 */
bool DockInputCtl::setGain(QString name, double value)
{
    int gain = -1;
    bool success = false;

    for (int idx = 0; idx < gain_labels.length(); idx++)
    {
        if (gain_labels.at(idx)->text().contains(name))
        {
            gain = (int)(10 * value);
            gain_sliders.at(idx)->setValue(gain);
            success = true;
            break;
        }
    }

    return success;
}

/**
 * @brief Get current gain.
 * @returns The relative gain between 0.0 and 1.0 or -1 if HW AGC is enabled.
 */
double DockInputCtl::gain(QString &name)
{
    double gain = 0.0;

    for (int idx = 0; idx < gain_labels.length(); ++idx)
    {
        if (gain_labels.at(idx)->text() == name)
        {
            gain = 0.1 * (double)gain_sliders.at(idx)->value();
            break;
        }
    }

    return gain;
}

/** Populate antenna selector combo box with strings. */
void DockInputCtl::setAntennas(std::vector<std::string> &antennas)
{
    QComboBox * antSelector=dynamic_cast<QComboBox *>(getWidget(C_ANTENNA));
    antSelector->clear();
    for (std::vector<std::string>::iterator it = antennas.begin(); it != antennas.end(); ++it)
        antSelector->addItem(QString(it->c_str()),QString(it->c_str()));
    antSelector->setEnabled(antSelector->count() > 1);
}

/**
 * Set gain stages.
 * @param gain_list A list containing the gain stages for this device.
 */
void DockInputCtl::setGainStages(gain_list_t &gain_list)
{
    QLabel  *label;
    QSlider *slider;
    QLabel  *value;
    int start, stop, step, gain;

    // ensure that gain lists are empty
    clearWidgets();

    for (unsigned int i = 0; i < gain_list.size(); i++)
    {
        start = (int)(10.0 * gain_list[i].start);
        stop  = (int)(10.0 * gain_list[i].stop);
        step  = (int)(10.0 * gain_list[i].step);
        gain  = (int)(10.0 * gain_list[i].value);

        label = new QLabel(QString("%1 ").arg(gain_list[i].name.c_str()), this);
        label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);

        value = new QLabel(QString(" %1 dB").arg(gain_list[i].value, 0, 'f', 1), this);
        value->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);

        slider = new QSlider(Qt::Horizontal, this);
        slider->setProperty("idx", i);
        slider->setProperty("name", QString(gain_list[i].name.c_str()));
        slider->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        slider->setRange(start, stop);
        slider->setSingleStep(step);
        slider->setValue(gain);
        if (abs(stop - start) > 10 * step)
            slider->setPageStep(10 * step);

        gainLayout->addWidget(label, i, 0, Qt::AlignLeft);
        gainLayout->addWidget(slider, i, 1);        // setting alignment would force minimum size
        gainLayout->addWidget(value, i, 2, Qt::AlignLeft);

        gain_labels.push_back(label);
        gain_sliders.push_back(slider);
        value_labels.push_back(value);

        connect(slider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
    }

    qDebug() << "********************";
    for (gain_list_t::iterator it = gain_list.begin(); it != gain_list.end(); ++it)
    {
        qDebug() << "Gain name:" << QString(it->name.c_str());
        qDebug() << "      min:" << it->start;
        qDebug() << "      max:" << it->stop;
        qDebug() << "     step:" << it->step;
    }
    qDebug() << "********************";
}


/**
 * Load all gains from the settings.
 *
 * Can be used for restoring the manual gains after auto-gain has been
 * disabled.
 */
void DockInputCtl::restoreManualGains(void)
{
    QString gain_stage;
    double  gain_value;
    int     i;

    for (i = 0; i < gain_sliders.length(); i++)
    {
        gain_stage = gain_sliders.at(i)->property("name").toString();
        gain_value = 0.1 * (double)gain_sliders.at(i)->value();
        emit gainChanged(gain_stage, gain_value);
    }
}

/** Automatic gain control button has been toggled. */
void DockInputCtl::hwagcObserver(c_id, const c_def::v_union & v)
{
    for (int i = 0; i < gain_sliders.length(); ++i)
    {
        gain_sliders.at(i)->setEnabled(!bool(v));
    }
    if(!bool(v))
        restoreManualGains();
}

/** Remove all widgets from the lists. */
void DockInputCtl::clearWidgets()
{
    QWidget *widget;

    // sliders
    while (!gain_sliders.isEmpty())
    {
        widget = gain_sliders.takeFirst();
        gainLayout->removeWidget(widget);
        delete widget;
    }

    // labels
    while (!gain_labels.isEmpty())
    {
        widget = gain_labels.takeFirst();
        gainLayout->removeWidget(widget);
        delete widget;
    }

    // value labels
    while (!value_labels.isEmpty())
    {
        widget = value_labels.takeFirst();
        gainLayout->removeWidget(widget);
        delete widget;
    }
}

/**
 * Slot for managing slider value changed signals.
 * @param value The value of the slider.
 *
 * Note. We use the sender() function to find out which slider has emitted the signal.
 */
void DockInputCtl::sliderValueChanged(int value)
{
    QSlider *slider = (QSlider *) sender();
    int idx = slider->property("idx").toInt();

    // convert to discrete value according to step
    if (slider->singleStep()) {
        value = slider->singleStep() * (value / slider->singleStep());
    }

    // convert to double and send signal
    double gain = (double)value / 10.0;
    updateLabel(idx, gain);

    emit gainChanged(slider->property("name").toString(), gain);
}

/**
 * Update value label
 * @param idx The index of the gain
 * @param value The new value
 */
void DockInputCtl::updateLabel(int idx, double value)
{
    QLabel *label = value_labels.at(idx);

    label->setText(QString("%1 dB").arg(value, 0, 'f', 1));
}

/**
 * Get all gains.
 * @param gains Pointer to a map where the gains and their names are stored.
 *
 * This is a private utility function used when storing the settings.
 */
void DockInputCtl::getGains(QMap<QString, QVariant> *gains)
{
    for (int idx = 0; idx < gain_sliders.length(); ++idx)
    {
        gains->insert(gain_sliders.at(idx)->property("name").toString(),
                      QVariant(gain_sliders.at(idx)->value()));
    }
}
