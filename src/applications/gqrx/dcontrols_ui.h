/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2023 vladisslav2011@gmail.com.
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
#ifndef INCLUDED_DCONTROLS_UI_H
#define INCLUDED_DCONTROLS_UI_H
#include "applications/gqrx/dcontrols.h"
#include <QGridLayout>
#include <QSettings>
#include <QMenu>
#include <QMap>
#include <QTabWidget>
#include <QStackedWidget>

struct dcontrols_ui
{
    dcontrols_ui()
    {
        ui_windows[W_BASE]=this;
    }

    virtual void add_control(const c_id id);
    //user action
    virtual void changed_gui(const c_id id, const c_def::v_union & value, bool trigger_observers = true);
    //state update
    virtual void set_gui(const c_id id, const c_def::v_union & value, bool trigger_observers = false);
    //current state
    virtual void get_gui(const c_id id, c_def::v_union & value) const;
    //add spacers
    virtual void finalize();
    QWidget * construct_widget(const c_id id);
    void grid_init(QGridLayout * layout, int n_row,int n_column)
    {
        gridLayout=layout;
        cur_placement.column=n_column;
        cur_placement.row=n_row;
    }
    template<class T> void set_observer(const c_id id, void (T::* observer)(const c_id, const c_def::v_union &))
    {
        observers[id] = [=](const c_id id, const c_def::v_union & value){ (dynamic_cast<T*>(this)->*observer)(id,value);};
    }

    std::array<std::function<void (const c_id, const c_def::v_union &)>, C_COUNT> observers{};
    std::array<dcontrols_ui *, W_COUNT> ui_windows{};

private:

    QWidget * gen_none(const c_id id);
    QWidget * gen_text(const c_id id);
    void set_text(QWidget * w, const c_id, const c_def::v_union &value);
    void get_text(QWidget * w, const c_id, c_def::v_union &value) const;
    QWidget * gen_spinbox(const c_id id);
    void set_spinbox(QWidget * w, const c_id, const c_def::v_union &value);
    void get_spinbox(QWidget * w, const c_id, c_def::v_union &value) const;
    QWidget * gen_doublespinbox(const c_id id);
    void set_doublespinbox(QWidget * w, const c_id, const c_def::v_union &value);
    void get_doublespinbox(QWidget * w, const c_id, c_def::v_union &value) const;
    QWidget * gen_checkbox(const c_id id);
    void set_checkbox(QWidget * w, const c_id, const c_def::v_union &value);
    void get_checkbox(QWidget * w, const c_id, c_def::v_union &value) const;
    QWidget * gen_combo(const c_id id);
    void set_combo(QWidget * w, const c_id, const c_def::v_union &value);
    void get_combo(QWidget * w, const c_id, c_def::v_union &value) const;
    QWidget * gen_stringcombo(const c_id id);
    void set_stringcombo(QWidget * w, const c_id, const c_def::v_union &value);
    void get_stringcombo(QWidget * w, const c_id, c_def::v_union &value) const;
    QWidget * gen_slider(const c_id id);
    void set_slider(QWidget * w, const c_id, const c_def::v_union &value);
    void get_slider(QWidget * w, const c_id, c_def::v_union &value) const;
    QWidget * gen_logslider(const c_id id);
    void set_logslider(QWidget * w, const c_id, const c_def::v_union &value);
    void get_logslider(QWidget * w, const c_id, c_def::v_union &value) const;
    QWidget * gen_rangeslider(const c_id id);
    void set_rangeslider(QWidget * w, const c_id, const c_def::v_union &value);
    void get_rangeslider(QWidget * w, const c_id, c_def::v_union &value) const;
    QWidget * gen_menuaction(const c_id id);
    QWidget * gen_menucheckbox(const c_id id);
    void set_menucheckbox(QWidget * w, const c_id id, const c_def::v_union &value);
    void get_menucheckbox(QWidget * w, const c_id id, c_def::v_union &value) const;
    QWidget * gen_button(const c_id id);
    QWidget * gen_togglebutton(const c_id id);
    void set_togglebutton(QWidget * w, const c_id id, const c_def::v_union &value);
    void get_togglebutton(QWidget * w, const c_id, c_def::v_union &value) const;
    QWidget * gen_label(const c_id id);
    void set_label(QWidget * w, const c_id id, const c_def::v_union &value);
    void get_label(QWidget * w, const c_id, c_def::v_union &value) const;
    QWidget * gen_hlabel(const c_id id);
    void set_hlabel(QWidget * w, const c_id id, const c_def::v_union &value);
    QWidget * gen_line(const c_id id);
    QWidget * gen_colorpicker(const c_id id);
    void set_colorpicker(QWidget * w, const c_id, const c_def::v_union &value);
    void get_colorpicker(QWidget * w, const c_id, c_def::v_union &value) const;

    std::array<std::function<void (const c_id, c_def::v_union &)>, C_COUNT> getters{};
    static std::array<QMenu *, C_COUNT> ui_menus;
    static std::array<QAction *, C_COUNT> ui_actions;
    static std::array<QWidget *, C_COUNT> ui_widgets;
protected:
    QWidget *getWidget(c_id id) const {return ui_widgets[id];}
    QAction *getAction(c_id id) const {return ui_actions[id];}
    QWidget *obtainParent();
    void set_placement(c_def::grid_placement & what, const c_def::grid_placement & to);
    void grid_insert(QGridLayout * into, c_def::grid_placement & placement, QWidget * what, const c_id id);
    virtual void finalizeInner();
    Qt::Alignment toQtAlignment(const alignments from, const Qt::Alignment def);
    QSizePolicy::Policy toQtSizePol(const alignments from, const QSizePolicy::Policy def);

    QGridLayout * gridLayout{nullptr};
    c_def::grid_placement cur_placement{0,0};
    static std::array<QWidget * (dcontrols_ui::*)(const c_id), G_GUI_TYPE_COUNT> generators;
    static std::array<void (dcontrols_ui::*)(QWidget *, const c_id, const c_def::v_union &), G_GUI_TYPE_COUNT> managed_setters;
    static std::array<void (dcontrols_ui::*)(QWidget *, const c_id, c_def::v_union &) const, G_GUI_TYPE_COUNT> managed_getters;
};

struct dcontrols_ui_tabbed:public dcontrols_ui
{
    dcontrols_ui_tabbed():dcontrols_ui(){};
    void add_control(const c_id id) override;
protected:
    void finalizeInner() override;
private:
    QTabWidget *ui_tabWidget{nullptr};
    QMap<QString, QWidget*> ui_tabs{};
    QMap<QString, c_def::grid_placement> ui_tabs_placements{};
};

struct dcontrols_ui_stacked:public dcontrols_ui
{
    dcontrols_ui_stacked():dcontrols_ui(){};
    void add_control(const c_id id) override;
    void setCurrentIndex(int n);
protected:
    void finalizeInner() override;
private:
    QStackedWidget *ui_stackedWidget{nullptr};
    QVector<QWidget*> ui_tabs{};
    QVector<c_def::grid_placement> ui_tabs_placements{};
};



#endif //INCLUDED_DCONTROLS_UI_H
