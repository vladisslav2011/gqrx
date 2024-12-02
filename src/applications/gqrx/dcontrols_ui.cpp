#include "dcontrols_ui.h"
#include <array>
#include <stdexcept>
#include <cmath>
#include <QMenu>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QSlider>
#include <QPushButton>
#include <QSpacerItem>
#include <QIcon>
#include "qtgui/ctk/ctkRangeSlider.h"
#include "qtgui/qtcolorpicker.h"

QWidget * dcontrols_ui::obtainParent()
{
    QWidget * parent = dynamic_cast<QWidget *>(this);
    if(!parent)
        throw std::runtime_error("dcontrols_ui requires to be inherited by QWidget decsendant");
    return parent;
}

QWidget * dcontrols_ui::gen_none(const c_id id)
{
    QWidget * parent = obtainParent();
    auto & def = c_def::all()[id];
    if(def.shortcut())
    {
        QAction * action = new QAction(parent);
        action->setShortcut(QKeySequence(def.shortcut()));
        parent->addAction(action);
        parent->connect(action, &QAction::triggered, [=](){changed_gui(id, def.def());});
    }
    return NULL;
}

QWidget * dcontrols_ui::gen_text(const c_id id)
{
    QWidget * parent = obtainParent();
    QLineEdit * widget = new QLineEdit(parent);
    auto & def = c_def::all()[id];
    if(def.writable())
    {
        parent->connect(widget, &QLineEdit::textChanged, [=](const QString & val){changed_gui(id, val.toStdString());} );
        if(def.presets().size() > 0)
        {
            QMenu *m = ui_menus[id];
            if(!m)
            {
                m=ui_menus[id]=widget->createStandardContextMenu();
                widget->setContextMenuPolicy(Qt::CustomContextMenu);
                parent->connect(widget,&QWidget::customContextMenuRequested,
                    [=](const QPoint& pos){ui_menus[id]->popup(widget->mapToGlobal(pos));});
            }
            for(auto it=def.presets().begin(); it!=def.presets().end();++it)
            {
                QAction* action = new QAction(it->display, parent);
                m->addAction(action);
                parent->connect(action, &QAction::triggered, [=](){widget->setText(QString(it->value));});
                if(it->shortcut)
                {
                    action->setShortcut(QKeySequence(it->shortcut));
                    parent->addAction(action);
                }
            }
        }
        if(def.shortcut())
        {
            QAction * action = new QAction(parent);
            action->setShortcut(QKeySequence(def.shortcut()));
            parent->addAction(action);
            parent->connect(action, &QAction::triggered, [=](){widget->setFocus();});
        }
    }else
        widget->setReadOnly(true);
    return widget;
}

void dcontrols_ui::set_text(QWidget * w, const c_id, const c_def::v_union &value)
{
    QLineEdit * widget=dynamic_cast<QLineEdit *>(w);
    widget->blockSignals(true);
    widget->setText(QString::fromStdString(value));
    widget->blockSignals(false);
}

void dcontrols_ui::get_text(QWidget * w, const c_id, c_def::v_union &value) const
{
    value = dynamic_cast<QLineEdit *>(w)->text().toStdString();
}

QWidget * dcontrols_ui::gen_spinbox(const c_id id)
{
    QWidget * parent = obtainParent();
    QSpinBox * widget = new QSpinBox(parent);
    auto & def = c_def::all()[id];
    if(def.writable())
    {
        parent->connect(widget, QOverload<int>::of(&QSpinBox::valueChanged), [=](int val){changed_gui(id, val);} );
        widget->setMinimum(def.min());
        widget->setMaximum(def.max());
        widget->setSingleStep(def.step());
        widget->setSuffix(QString(def.suffix()));
        widget->setPrefix(QString(def.prefix()));
        widget->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
        if(def.presets().size() > 0)
        {
            QMenu *m = ui_menus[id];
            if(!m)
            {
                m=ui_menus[id]=new QMenu(parent);
                widget->setContextMenuPolicy(Qt::CustomContextMenu);
                parent->connect(widget,&QWidget::customContextMenuRequested,
                    [=](const QPoint& pos){ui_menus[id]->popup(widget->mapToGlobal(pos));});
            }
            for(auto it=def.presets().begin(); it!=def.presets().end();++it)
            {
                QAction* action = new QAction(it->display, parent);
                m->addAction(action);
                auto lambda = [=](){widget->setValue(it->value);};
                parent->connect(action, &QAction::triggered, lambda);
                if(it->shortcut)
                {
                    action->setShortcut(QKeySequence(it->shortcut));
                    parent->addAction(action);
                }
            }
        }
        if(def.shortcut())
        {
            QAction * action = new QAction(parent);
            action->setShortcut(QKeySequence(def.shortcut()));
            parent->addAction(action);
            parent->connect(action, &QAction::triggered, [=](){widget->setFocus();});
        }
    }else
        widget->setReadOnly(true);
    return widget;
}

void dcontrols_ui::set_spinbox(QWidget * w, const c_id, const c_def::v_union &value)
{
    QSpinBox * widget=dynamic_cast<QSpinBox *>(w);
    widget->blockSignals(true);
    widget->setValue(value);
    widget->blockSignals(false);
}

void dcontrols_ui::get_spinbox(QWidget * w, const c_id, c_def::v_union &value) const
{
    value = dynamic_cast<QSpinBox *>(w)->value();
}

QWidget * dcontrols_ui::gen_doublespinbox(const c_id id)
{
    QWidget * parent = obtainParent();
    QDoubleSpinBox * widget = new QDoubleSpinBox(parent);
    auto & def = c_def::all()[id];
    if(def.writable())
    {
        switch(def.v_type())
        {
        case V_DOUBLE:
            parent->connect(widget, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double val){changed_gui(id, val);} );
            widget->setMinimum(def.min());
            widget->setMaximum(def.max());
            widget->setSingleStep(def.step());
        break;
        case V_INT:
            {
            const double scale = std::pow(10.0,def.frac_digits());
            const double iscale = std::pow(10.0,-def.frac_digits());
            parent->connect(widget, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double val)
                {
                    changed_gui(id, qint64(val*scale));
                });
            widget->setMinimum(qint64(def.min())*iscale);
            widget->setMaximum(qint64(def.max())*iscale);
            widget->setSingleStep(qint64(def.step())*iscale);
            }
        break;
        default:
            std::cerr<<"Invalid value type "<<def.v_type()<<" for doublespinbox "<<def.name()<<" \""<<def.title()<<"\"\n";
            exit(1);
        }
        widget->setDecimals(def.frac_digits());
        widget->setSuffix(QString(def.suffix()));
        widget->setPrefix(QString(def.prefix()));
        widget->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
        if(def.presets().size() > 0)
        {
            QMenu *m = ui_menus[id];
            if(!m)
                m=ui_menus[id]=new QMenu(parent);
            for(auto it=def.presets().begin(); it!=def.presets().end();++it)
            {
                QAction* action = new QAction(it->display, parent);
                m->addAction(action);
                switch(def.v_type())
                {
                case V_DOUBLE:
                    parent->connect(action, &QAction::triggered, [=](){widget->setValue(it->value);});
                break;
                case V_INT:
                    {
                    const double scale = std::pow(10.0,-def.frac_digits());
                    parent->connect(action, &QAction::triggered, [=](){widget->setValue(scale*qint64(it->value));});
                    }
                break;
                default:;
                }
                if(it->shortcut)
                {
                    action->setShortcut(QKeySequence(it->shortcut));
                    parent->addAction(action);
                }
            }
        }
        if(def.shortcut())
        {
            QAction * action = new QAction(parent);
            action->setShortcut(QKeySequence(def.shortcut()));
            parent->addAction(action);
            parent->connect(action, &QAction::triggered, [=](){widget->setFocus();});
        }
    }else
        widget->setReadOnly(true);
    if(ui_menus[id])
    {
        widget->setContextMenuPolicy(Qt::CustomContextMenu);
        parent->connect(widget,&QWidget::customContextMenuRequested,
            [=](const QPoint& pos){ui_menus[id]->popup(widget->mapToGlobal(pos));});
    }
    return widget;
}

void dcontrols_ui::set_doublespinbox(QWidget * w, const c_id id, const c_def::v_union &value)
{
    QDoubleSpinBox * widget=dynamic_cast<QDoubleSpinBox *>(w);
    auto & def = c_def::all()[id];
    widget->blockSignals(true);
    switch(def.v_type())
    {
    case V_DOUBLE:
        widget->setValue(value);
    break;
    case V_INT:
        {
        const double scale = std::pow(10.0,-def.frac_digits());
        widget->setValue(scale*qint64(value));
        }
    break;
    default:;
    }
    widget->blockSignals(false);
}

void dcontrols_ui::get_doublespinbox(QWidget * w, const c_id id, c_def::v_union &value) const
{
    QDoubleSpinBox * widget=dynamic_cast<QDoubleSpinBox *>(w);
    auto & def = c_def::all()[id];
    switch(def.v_type())
    {
    case V_DOUBLE:
        value = widget->value();
    break;
    case V_INT:
        {
        const double scale = std::pow(10.0,def.frac_digits());
        value = qint64(scale*widget->value());
        }
    break;
    default:;
    }
}

QWidget * dcontrols_ui::gen_checkbox(const c_id id)
{
    QWidget * parent = obtainParent();
    auto & def = c_def::all()[id];
    QCheckBox * widget = new QCheckBox(QString(def.name()), parent);
    if(def.writable())
    {
        parent->connect(widget, QOverload<int>::of(&QCheckBox::stateChanged), [=](int val){changed_gui(id, val==Qt::Checked);} );
        if(def.shortcut())
        {
            QAction * action = new QAction(parent);
            action->setShortcut(QKeySequence(def.shortcut()));
            parent->addAction(action);
            parent->connect(action, &QAction::triggered, [=](){widget->toggle();});
        }
    }else
        widget->setEnabled(false);
    return widget;
}

void dcontrols_ui::set_checkbox(QWidget * w, const c_id, const c_def::v_union &value)
{
    QCheckBox * widget=dynamic_cast<QCheckBox *>(w);
    widget->blockSignals(true);
    widget->setCheckState(int(value)?Qt::Checked:Qt::Unchecked);
    widget->blockSignals(false);
}

void dcontrols_ui::get_checkbox(QWidget * w, const c_id, c_def::v_union &value) const
{
    value = dynamic_cast<QCheckBox *>(w)->checkState() == Qt::Checked;
}

//TODO: Implement plain text input mode for all type and remove string combo?
QWidget * dcontrols_ui::gen_combo(const c_id id)
{
    QWidget * parent = obtainParent();
    QComboBox * widget = new QComboBox(parent);
    auto & def = c_def::all()[id];
    if(def.writable())
    {
        switch(def.v_type())
        {
        case V_INT:
            for(auto it=def.presets().begin(); it!=def.presets().end();++it)
                widget->addItem(it->display,(long long int)it->value);
            parent->connect(widget, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [=](int val){changed_gui(id, widget->currentData().toLongLong());} );
            break;
        case V_BOOLEAN:
            for(auto it=def.presets().begin(); it!=def.presets().end();++it)
                widget->addItem(it->display,bool(it->value));
            parent->connect(widget, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [=](int val){changed_gui(id, widget->currentData().toBool());} );
            break;
        case V_DOUBLE:
            for(auto it=def.presets().begin(); it!=def.presets().end();++it)
                widget->addItem(it->display,double(it->value));
            parent->connect(widget, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [=](int val){changed_gui(id, widget->currentData().toDouble());} );
            break;
        case V_STRING:
            for(auto it=def.presets().begin(); it!=def.presets().end();++it)
                widget->addItem(it->display,QString(it->value));
            parent->connect(widget, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [=](int val){changed_gui(id, widget->currentData().toString().toStdString());} );
            parent->connect(widget, &QComboBox::currentTextChanged,
                [=](const QString &text)
                {
                    if(widget->isEditable())
                        changed_gui(id, text.toStdString());
                } );
            break;
        }
        int k = 0;
        for(auto it=def.presets().begin(); it!=def.presets().end();++it,++k)
            if(it->shortcut)
            {
                QAction * action = new QAction(parent);
                action->setShortcut(QKeySequence(it->shortcut));
                parent->addAction(action);
                parent->connect(action, &QAction::triggered, [=](){widget->setCurrentIndex(k);});
            }
        if(def.shortcut())
        {
            QAction * action = new QAction(parent);
            action->setShortcut(QKeySequence(def.shortcut()));
            parent->addAction(action);
            parent->connect(action, &QAction::triggered, [=](){widget->setFocus();});
        }
    }else
        widget->setEnabled(false);
    return widget;
}

void dcontrols_ui::set_combo(QWidget * w, const c_id id, const c_def::v_union &value)
{
    QComboBox * widget=dynamic_cast<QComboBox *>(w);
    widget->blockSignals(true);
    auto & def = c_def::all()[id];
    int idx=-1;
    switch(def.v_type())
    {
    case V_INT:
            idx=widget->findData((long long int)value);
            if(idx==-1)
            {
                widget->setCurrentText(QString::number(qint64(value)));
            }else
                widget->setCurrentIndex(idx);
        break;
    case V_BOOLEAN:
            idx=widget->findData(bool(value));
            if(idx==-1)
            {
                widget->setCurrentText(QString::number(int(value)));
            }else
                widget->setCurrentIndex(idx);
        break;
    case V_DOUBLE:
        {
            const c_def::v_preset * p=&*def.presets().begin();
            double diff=std::abs(double(value)-double(p->value));
            for(auto it=def.presets().begin(); it!=def.presets().end();++it)
            {
                double ndiff=std::abs(double(value)-double(it->value));
                if(ndiff<diff)
                {
                    diff=ndiff;
                    p=&*it;
                }
            }
            idx=widget->findData(double(p->value));
            if(idx==-1)
            {
                widget->setCurrentText(QString::number(double(value)));
            }else
                widget->setCurrentIndex(idx);
        }
        break;
    case V_STRING:
            idx=widget->findData(QString::fromStdString(value));
            if(idx==-1)
            {
                widget->setCurrentText(QString::fromStdString(value));
            }else
                widget->setCurrentIndex(idx);
        break;
    }
    widget->blockSignals(false);
}

void dcontrols_ui::get_combo(QWidget * w, const c_id id, c_def::v_union &value) const
{
    auto & def = c_def::all()[id];
    QComboBox * widget = dynamic_cast<QComboBox *>(w);
    switch(def.v_type())
    {
    case V_INT:
        value = widget->currentData().toLongLong();
        break;
    case V_BOOLEAN:
        value = widget->currentData().toBool();
        break;
    case V_DOUBLE:
        value = widget->currentData().toDouble();
        break;
    case V_STRING:
        if(widget->isEditable())
            value = widget->currentText().toStdString();
        else
            value = widget->currentData().toString().toStdString();
        break;
    }
}

QWidget * dcontrols_ui::gen_stringcombo(const c_id id)
{
    QWidget * parent = obtainParent();
    QComboBox * widget = new QComboBox(parent);
    auto & def = c_def::all()[id];
    if(def.writable())
    {
        widget->setEditable(true);
        parent->connect(widget, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
            [=](const QString & val){changed_gui(id, val.toStdString());} );
        for(auto it=def.presets().begin(); it!=def.presets().end();++it)
        {
            if(it->shortcut)
            {
                QAction * action = new QAction(parent);
                action->setShortcut(QKeySequence(it->shortcut));
                parent->addAction(action);
                parent->connect(action, &QAction::triggered, [=](){widget->setCurrentText(QString(it->value));});
            }
            widget->addItem(it->display,QString(it->value));
        }
        if(def.shortcut())
        {
            QAction * action = new QAction(parent);
            action->setShortcut(QKeySequence(def.shortcut()));
            parent->addAction(action);
            parent->connect(action, &QAction::triggered, [=](){widget->setFocus();});
        }
    }else
        widget->setEnabled(false);
    return widget;
}

void dcontrols_ui::set_stringcombo(QWidget * w, const c_id, const c_def::v_union &value)
{
    QComboBox * widget=dynamic_cast<QComboBox *>(w);
    widget->blockSignals(true);
    widget->setEditText(QString::fromStdString(value));
    widget->blockSignals(false);
}

void dcontrols_ui::get_stringcombo(QWidget * w, const c_id, c_def::v_union &value) const
{
    value = dynamic_cast<QComboBox *>(w)->currentText().toStdString();
}

QWidget * dcontrols_ui::gen_slider(const c_id id)
{
    QWidget * parent = obtainParent();
    QSlider * widget = new QSlider(Qt::Horizontal, parent);
    auto & def = c_def::all()[id];
    switch(def.v_type())
    {
        case V_INT:
        case V_BOOLEAN:
            parent->connect(widget, QOverload<int>::of(&QSlider::valueChanged),
                [=](int val){changed_gui(id, val);} );
        break;
        case V_DOUBLE:
        {
            const double scale = std::pow(10.0,-def.frac_digits());
            parent->connect(widget, QOverload<int>::of(&QSlider::valueChanged),
                [=](int val){changed_gui(id,  scale * double(val));} );
        }
        break;
        case V_STRING://not supported
            throw std::runtime_error("QSlider with string value is not supported yet");
            //TODO: implement switching between string preset values?
        break;
    }
    if(def.writable())
    {
        switch(def.v_type())
        {
            case V_INT:
            case V_BOOLEAN:
                widget->setMinimum(def.min());
                widget->setMaximum(def.max());
                widget->setSingleStep(def.step());
            break;
            case V_DOUBLE:
            {
                const double scale = std::pow(10.0,def.frac_digits());
                widget->setMinimum(std::round(double(def.min())*scale));
                widget->setMaximum(std::round(double(def.max())*scale));
                widget->setSingleStep(std::round(double(def.step())*scale));
            }
            break;
            case V_STRING://not supported
                throw std::runtime_error("QSlider with string value is not supported yet");
                //TODO: implement switching between string preset values?
            break;
        }
        if(def.presets().size() > 0)
        {
            QMenu *m =ui_menus[id];
            if(!m)
            {
                m=ui_menus[id]=new QMenu(parent);
                widget->setContextMenuPolicy(Qt::CustomContextMenu);
                parent->connect(widget,&QWidget::customContextMenuRequested,
                    [=](const QPoint& pos){ui_menus[id]->popup(widget->mapToGlobal(pos));});
            }
            for(auto it=def.presets().begin(); it!=def.presets().end();++it)
            {
                QAction* action = new QAction(it->display, parent);
                m->addAction(action);
                switch(def.v_type())
                {
                    case V_INT:
                    case V_BOOLEAN:
                        parent->connect(action, &QAction::triggered, [=](){widget->setValue(it->value);});
                    break;
                    case V_DOUBLE:
                    {
                        const double scale = std::pow(10.0,def.frac_digits());
                        parent->connect(action, &QAction::triggered, [=](){widget->setValue(std::round(double(it->value)*scale));});
                    }
                    break;
                    case V_STRING://not supported
                        throw std::runtime_error("QSlider with string value is not supported yet");
                        //TODO: implement switching between string preset values?
                    break;
                }
                if(it->shortcut)
                {
                    action->setShortcut(QKeySequence(it->shortcut));
                    parent->addAction(action);
                }
            }
        }
        if(def.shortcut())
        {
            QAction * action = new QAction(parent);
            action->setShortcut(QKeySequence(def.shortcut()));
            parent->addAction(action);
            parent->connect(action, &QAction::triggered, [=](){widget->setFocus();});
        }
    }else
        widget->setEnabled(false);
    return widget;
}

void dcontrols_ui::set_slider(QWidget * w, const c_id id, const c_def::v_union &value)
{
    QSlider * widget=dynamic_cast<QSlider *>(w);
    auto & def = c_def::all()[id];
    widget->blockSignals(true);
    switch(def.v_type())
    {
        case V_INT:
        case V_BOOLEAN:
            widget->setValue(value);
        break;
        case V_DOUBLE:
        {
            const double scale = std::pow(10.0,def.frac_digits());
            widget->setValue(std::round(double(value)*scale));
        }
        break;
        case V_STRING://not supported
            throw std::runtime_error("QSlider with string value is not supported yet");
            //TODO: implement switching between string preset values?
        break;
    }
    widget->blockSignals(false);
}

void dcontrols_ui::get_slider(QWidget * w, const c_id id, c_def::v_union &value) const
{
    auto & def = c_def::all()[id];
    switch(def.v_type())
    {
        case V_INT:
        case V_BOOLEAN:
            value = dynamic_cast<QSlider *>(w)->value();
        break;
        case V_DOUBLE:
        {
            const double scale = std::pow(10.0,-def.frac_digits());
            value = scale * dynamic_cast<QSlider *>(w)->value();
        }
        break;
        case V_STRING://not supported
            throw std::runtime_error("QSlider with string value is not supported yet");
            //TODO: implement switching between string preset values?
        break;
    }
}

QWidget * dcontrols_ui::gen_logslider(const c_id id)
{
    QWidget * parent = obtainParent();
    QSlider * widget = new QSlider(Qt::Horizontal, parent);
    auto & def = c_def::all()[id];
    switch(def.v_type())
    {
        case V_DOUBLE:
        {
            parent->connect(widget, QOverload<int>::of(&QSlider::valueChanged),
                [=](int val){changed_gui(id,  std::pow(double(def.step()),val));} );
        }
        break;
        default://not supported
            throw std::runtime_error("logslider requires double value");
        break;
    }
    if(def.writable())
    {
        switch(def.v_type())
        {
            case V_DOUBLE:
            {
                const double scale = 1.0/std::log(double(def.step()));
                widget->setMinimum(std::round(std::log(double(def.min()))*scale));
                widget->setMaximum(std::round(std::log(double(def.max()))*scale));
                widget->setSingleStep(1);
            }
            break;
            default://not supported
                throw std::runtime_error("logslider requires double value");
            break;
        }
        if(def.presets().size() > 0)
        {
            QMenu *m =ui_menus[id];
            if(!m)
            {
                m=ui_menus[id]=new QMenu(parent);
                widget->setContextMenuPolicy(Qt::CustomContextMenu);
                parent->connect(widget,&QWidget::customContextMenuRequested,
                    [=](const QPoint& pos){ui_menus[id]->popup(widget->mapToGlobal(pos));});
            }
            for(auto it=def.presets().begin(); it!=def.presets().end();++it)
            {
                QAction* action = new QAction(it->display, parent);
                m->addAction(action);
                switch(def.v_type())
                {
                    case V_DOUBLE:
                    {
                        const double scale = 1.0/std::log(double(def.step()));
                        parent->connect(action, &QAction::triggered, [=](){widget->setValue(std::round(std::log(double(it->value))*scale));});
                    }
                    break;
                    default://not supported
                        throw std::runtime_error("logslider requires double value");
                    break;
                }
                if(it->shortcut)
                {
                    action->setShortcut(QKeySequence(it->shortcut));
                    parent->addAction(action);
                }
            }
        }
        if(def.shortcut())
        {
            QAction * action = new QAction(parent);
            action->setShortcut(QKeySequence(def.shortcut()));
            parent->addAction(action);
            parent->connect(action, &QAction::triggered, [=](){widget->setFocus();});
        }
    }else
        widget->setEnabled(false);
    return widget;
}

void dcontrols_ui::set_logslider(QWidget * w, const c_id id, const c_def::v_union &value)
{
    QSlider * widget=dynamic_cast<QSlider *>(w);
    auto & def = c_def::all()[id];
    widget->blockSignals(true);
    switch(def.v_type())
    {
        case V_DOUBLE:
        {
            const double scale = 1.0/std::log(double(def.step()));
            widget->setValue(std::round(std::log(double(value))*scale));
        }
        break;
        default://not supported
            throw std::runtime_error("logslider requires double value");
        break;
    }
    widget->blockSignals(false);
}

void dcontrols_ui::get_logslider(QWidget * w, const c_id id, c_def::v_union &value) const
{
    auto & def = c_def::all()[id];
    switch(def.v_type())
    {
        case V_DOUBLE:
            value = std::pow(double(def.step()), dynamic_cast<QSlider *>(w)->value());
        break;
        default://not supported
            throw std::runtime_error("logslider requires double value");
        break;
    }
}

QWidget * dcontrols_ui::gen_rangeslider(const c_id id)
{
    auto & def1 = c_def::all()[id];
    if(def1.base() == c_id(-1))
        return nullptr;
    const c_id baseId = def1.base();
    auto & def2 = c_def::all()[baseId];
    if(def1.v_type() != def2.v_type())
    {
        const QString ex=QString("ctkRangeSlider incorrect definition: %1 .v_type=%2 !=%3 .v_type=%4 ")
            .arg(def1.name()).arg(int(def1.v_type())).arg(def2.name()).arg(int(def2.v_type()));
        throw std::runtime_error(ex.toStdString().c_str());
    }
    QWidget * parent = obtainParent();
    ctkRangeSlider * widget = new ctkRangeSlider(Qt::Horizontal, parent);
    ui_widgets[baseId] = widget;
    switch(def1.v_type())
    {
        case V_INT:
        case V_BOOLEAN:
            parent->connect(widget, &ctkRangeSlider::minimumValueChanged,
                [=](int val){changed_gui(baseId, val);} );
            parent->connect(widget, &ctkRangeSlider::maximumValueChanged,
                [=](int val){changed_gui(id, val);} );
        break;
        case V_DOUBLE:
        {
            const double scale = std::pow(10.0,-def1.frac_digits());
            parent->connect(widget, &ctkRangeSlider::minimumValueChanged,
                [=](int val){changed_gui(baseId,  scale * double(val));} );
            parent->connect(widget, &ctkRangeSlider::maximumValueChanged,
                [=](int val){changed_gui(id,  scale * double(val));} );
        }
        break;
        case V_STRING://not supported
            throw std::runtime_error("ctkRangeSlider with string value is not supported yet");
            //TODO: implement switching between string preset values?
        break;
    }
    if(def1.writable())
    {
        switch(def1.v_type())
        {
            case V_INT:
            case V_BOOLEAN:
                widget->setMinimum(def1.min());
                widget->setMaximum(def1.max());
                widget->setSingleStep(def1.step());
            break;
            case V_DOUBLE:
            {
                const double scale = std::pow(10.0,def1.frac_digits());
                widget->setMinimum(std::round(double(def1.min())*scale));
                widget->setMaximum(std::round(double(def1.max())*scale));
                widget->setSingleStep(std::round(double(def1.step())*scale));
            }
            break;
            case V_STRING://not supported
                throw std::runtime_error("ctkRangeSlider with string value is not supported yet");
                //TODO: implement switching between string preset values?
            break;
        }
        if(def1.presets().size() > 0)
        {
            QMenu *m =ui_menus[id];
            if(!m)
                m =ui_menus[id]=ui_menus[baseId];
            if(!m)
            {
                m=ui_menus[id]=ui_menus[baseId]=new QMenu(parent);
                widget->setContextMenuPolicy(Qt::CustomContextMenu);
                parent->connect(widget,&QWidget::customContextMenuRequested,
                    [=](const QPoint& pos){ui_menus[id]->popup(widget->mapToGlobal(pos));});
            }
            for(auto it=def1.presets().begin(); it!=def1.presets().end();++it)
            {
                QAction* action = new QAction(it->display, parent);
                m->addAction(action);
                switch(def1.v_type())
                {
                    case V_INT:
                    case V_BOOLEAN:
                        parent->connect(action, &QAction::triggered, [=](){widget->setMaximumValue(it->value);});
                    break;
                    case V_DOUBLE:
                    {
                        const double scale = std::pow(10.0,def1.frac_digits());
                        parent->connect(action, &QAction::triggered, [=](){widget->setMaximumValue(std::round(double(it->value)*scale));});
                    }
                    break;
                    case V_STRING://not supported
                        throw std::runtime_error("ctkRangeSlider with string value is not supported yet");
                        //TODO: implement switching between string preset values?
                    break;
                }
                if(it->shortcut)
                {
                    action->setShortcut(QKeySequence(it->shortcut));
                    parent->addAction(action);
                }
            }
        }
        if(def2.presets().size() > 0)
        {
            QMenu *m =ui_menus[baseId];
            if(!m)
                m =ui_menus[baseId]=ui_menus[id];
            if(!m)
            {
                m=ui_menus[id]=ui_menus[baseId]=new QMenu(parent);
                widget->setContextMenuPolicy(Qt::CustomContextMenu);
                parent->connect(widget,&QWidget::customContextMenuRequested,
                    [=](const QPoint& pos){ui_menus[id]->popup(widget->mapToGlobal(pos));});
            }
            for(auto it=def2.presets().begin(); it!=def2.presets().end();++it)
            {
                QAction* action = new QAction(it->display, parent);
                m->addAction(action);
                switch(def2.v_type())
                {
                    case V_INT:
                    case V_BOOLEAN:
                        parent->connect(action, &QAction::triggered, [=](){widget->setMinimumValue(it->value);});
                    break;
                    case V_DOUBLE:
                    {
                        const double scale = std::pow(10.0,def1.frac_digits());
                        parent->connect(action, &QAction::triggered, [=](){widget->setMinimumValue(std::round(double(it->value)*scale));});
                    }
                    break;
                    case V_STRING://not supported
                        throw std::runtime_error("ctkRangeSlider with string value is not supported yet");
                        //TODO: implement switching between string preset values?
                    break;
                }
                if(it->shortcut)
                {
                    action->setShortcut(QKeySequence(it->shortcut));
                    parent->addAction(action);
                }
            }
        }
        if(def1.shortcut())
        {
            QAction * action = new QAction(parent);
            action->setShortcut(QKeySequence(def1.shortcut()));
            parent->addAction(action);
            parent->connect(action, &QAction::triggered, [=](){widget->setFocus();});
        }
    }else
        widget->setEnabled(false);
    return widget;
}

void dcontrols_ui::set_rangeslider(QWidget * w, const c_id id, const c_def::v_union &value)
{
    ctkRangeSlider * widget=dynamic_cast<ctkRangeSlider *>(w);
    auto & def = c_def::all()[id];
    widget->blockSignals(true);
    switch(def.v_type())
    {
        case V_INT:
        case V_BOOLEAN:
            if(def.base() == c_id(-1))
                widget->setMinimumValue(value);
            else
                widget->setMaximumValue(value);
        break;
        case V_DOUBLE:
        {
            const double scale = std::pow(10.0,def.frac_digits());
            if(def.base() == c_id(-1))
                widget->setMinimumValue(std::round(double(value)*scale));
            else
                widget->setMaximumValue(std::round(double(value)*scale));
        }
        break;
        case V_STRING://not supported
            throw std::runtime_error("QSlider with string value is not supported yet");
            //TODO: implement switching between string preset values?
        break;
    }
    widget->blockSignals(false);
}

void dcontrols_ui::get_rangeslider(QWidget * w, const c_id id, c_def::v_union &value) const
{
    auto & def = c_def::all()[id];
    switch(def.v_type())
    {
        case V_INT:
        case V_BOOLEAN:
            if(def.base() == c_id(-1))
                value = dynamic_cast<ctkRangeSlider *>(w)->minimumValue();
            else
                value = dynamic_cast<ctkRangeSlider *>(w)->maximumValue();
        break;
        case V_DOUBLE:
        {
            const double scale = std::pow(10.0,-def.frac_digits());
            if(def.base() == c_id(-1))
                value = scale * dynamic_cast<ctkRangeSlider *>(w)->minimumValue();
            else
                value = scale * dynamic_cast<ctkRangeSlider *>(w)->maximumValue();
        }
        break;
        case V_STRING://not supported
            throw std::runtime_error("QSlider with string value is not supported yet");
            //TODO: implement switching between string preset values?
        break;
    }
}


QWidget * dcontrols_ui::gen_menuaction(const c_id id)
{
    QWidget * parent = obtainParent();
    auto & def = c_def::all()[id];
    if(def.base()<0)
        throw std::runtime_error("gen_menuaction ("+std::to_string(id)+") \""+std::string(def.name())+"\" base is not set");
    QAction * widget = new QAction(QString(def.title()), parent);
    QMenu * m = ui_menus[def.base()];
    if(!m)
    {
        m=ui_menus[def.base()]=new QMenu(parent);
        ui_widgets[def.base()]->setContextMenuPolicy(Qt::CustomContextMenu);
        parent->connect(ui_widgets[def.base()],&QWidget::customContextMenuRequested,
            [=](const QPoint& pos){ui_menus[def.base()]->popup(ui_widgets[def.base()]->mapToGlobal(pos));});
    }
    m->addAction(widget);
    parent->connect(widget, QOverload<bool>::of(&QAction::triggered), [=](bool val){changed_gui(id, def.def());} );
    ui_actions[id]=widget;
    if(def.shortcut())
    {
        widget->setShortcut(QKeySequence(def.shortcut()));
        parent->addAction(widget);
    }
    return nullptr;
}

QWidget * dcontrols_ui::gen_menucheckbox(const c_id id)
{
    QWidget * parent = obtainParent();
    auto & def = c_def::all()[id];
    if(def.base()<0)
        throw std::runtime_error("gen_menucheckbox ("+std::to_string(id)+") \""+std::string(def.name())+"\" base is not set");
    QAction * widget = new QAction(QString(def.title()), parent);
    QMenu * m = ui_menus[def.base()];
    if(!m)
    {
        m=ui_menus[def.base()]=new QMenu(parent);
        ui_widgets[def.base()]->setContextMenuPolicy(Qt::CustomContextMenu);
        parent->connect(ui_widgets[def.base()],&QWidget::customContextMenuRequested,
            [=](const QPoint& pos){ui_menus[def.base()]->popup(ui_widgets[def.base()]->mapToGlobal(pos));});
    }
    m->addAction(widget);
    if(def.writable())
    {
        widget->setCheckable(true);
        parent->connect(widget, QOverload<bool>::of(&QAction::triggered), [=](bool val){changed_gui(id, val);} );
        if(def.shortcut())
        {
            widget->setShortcut(QKeySequence(def.shortcut()));
            parent->addAction(widget);
        }
    }
    ui_actions[id]=widget;
    return parent;
}

void dcontrols_ui::set_menucheckbox(QWidget * w, const c_id id, const c_def::v_union &value)
{
    ui_actions[id]->blockSignals(true);
    ui_actions[id]->setChecked(value);
    ui_actions[id]->blockSignals(false);
}

void dcontrols_ui::get_menucheckbox(QWidget * w, const c_id id, c_def::v_union &value) const
{
    value = ui_actions[id]->isChecked();
}

QWidget * dcontrols_ui::gen_button(const c_id id)
{
    QWidget * parent = obtainParent();
    auto & def = c_def::all()[id];
    QPushButton * widget = new QPushButton(QString(def.name()), parent);
    if(def.base() < 0)
        parent->connect(widget, QOverload<bool>::of(&QPushButton::clicked), [=](bool val){changed_gui(id, def.def());} );
    else
        parent->connect(widget, QOverload<bool>::of(&QPushButton::clicked), [=](bool val)
            {
                changed_gui(def.base(), def.def());
                set_gui(def.base(), def.def());
            } );
    widget->setMinimumSize(10,0);
    if(def.icon())
        widget->setIcon(QIcon(QString(def.icon())));
    if(def.shortcut())
    {
        QAction * action = new QAction(parent);
        action->setShortcut(QKeySequence(def.shortcut()));
        parent->addAction(action);
        parent->connect(action, &QAction::triggered, [=](){widget->click();});
    }
    return widget;
}

QWidget * dcontrols_ui::gen_togglebutton(const c_id id)
{
    QWidget * parent = obtainParent();
    auto & def = c_def::all()[id];
    QPushButton * widget = new QPushButton(QString(def.name()), parent);
    if(def.writable())
    {
        widget->setCheckable(true);
        parent->connect(widget, QOverload<bool>::of(&QPushButton::clicked), [=](bool val){changed_gui(id, val);} );
    }
    widget->setMinimumSize(10,0);
    if(def.icon())
        widget->setIcon(QIcon(QString(def.icon())));
    if(def.shortcut())
    {
        QAction * action = new QAction(parent);
        action->setShortcut(QKeySequence(def.shortcut()));
        parent->addAction(action);
        parent->connect(action, &QAction::triggered, [=](){widget->click();});
    }
    return widget;
}

void dcontrols_ui::set_togglebutton(QWidget * w, const c_id id, const c_def::v_union &value)
{
    QPushButton * widget=dynamic_cast<QPushButton *>(w);
    widget->blockSignals(true);
    widget->setChecked(value);
    widget->blockSignals(false);
}

void dcontrols_ui::get_togglebutton(QWidget * w, const c_id, c_def::v_union &value) const
{
    value = dynamic_cast<QPushButton *>(w)->isChecked();
}

QWidget * dcontrols_ui::gen_label(const c_id id)
{
    QWidget * parent = obtainParent();
    QLabel * widget = new QLabel("",parent);
    widget->setWordWrap(true);
    widget->setTextFormat(Qt::PlainText);
    set_label(widget,id,c_def::all()[id].def());
    return widget;
}

void dcontrols_ui::set_label(QWidget * w, const c_id id, const c_def::v_union &value)
{
    auto & def = c_def::all()[id];
    QLabel * widget = dynamic_cast<QLabel *>(w);
    switch(def.v_type())
    {
        case V_INT:
        case V_BOOLEAN:
            widget->setText(QString(def.prefix())
                +QString::number(qint64(value))
                +QString(def.suffix())
                );
        break;
        case V_DOUBLE:
            widget->setText(QString("%1%2%3")
                .arg(def.prefix())
                .arg(double(value),1,'f',def.frac_digits())
                .arg(def.suffix())
                );
        break;
        case V_STRING:
            widget->setText(QString(def.prefix())
                +QString::fromStdString(value.to_string())
                +QString(def.suffix())
                );
        break;
    }
}

void dcontrols_ui::get_label(QWidget * w, const c_id, c_def::v_union &value) const
{
//    value = dynamic_cast<QLabel *>(w)->text().toStdString();
// not supported
}

QWidget * dcontrols_ui::gen_hlabel(const c_id id)
{
    QLabel * widget = dynamic_cast<QLabel *>(gen_label(id));
    widget->setTextFormat(Qt::RichText);
    return widget;
}

void dcontrols_ui::set_hlabel(QWidget * w, const c_id id, const c_def::v_union &value)
{
    auto & def = c_def::all()[id];
    QLabel * widget = dynamic_cast<QLabel *>(w);
    QString str("");
    switch(def.v_type())
    {
        case V_INT:
        case V_BOOLEAN:
            str=QString::number(qint64(value));
        break;
        case V_DOUBLE:
            str=QString("%1").arg(double(value),1,'f',def.frac_digits());
        break;
        case V_STRING:
        {
            const std::string raw=value.to_string();
            static const std::array<const char *,3> colors{"#77ff77","#eeee44","#ffaaaa"};
            if(raw.size()<7)
                break;
            int p=-1;
            int l=-1;
            unsigned i=0;
            try{
                p=std::stoi(std::string(raw,0,2));
                l=std::stoi(std::string(raw,2,2));
                i=std::stoi(std::string(raw,4,1));
            }catch(std::exception &e){
                p=-1;
                l=-1;
                i=0;
            }
            if(p<0 || l<0)
                return;
            if(i>2)
                i=2;
            const std::string data = std::string(raw,5);
            const QString a = QString::fromStdString(std::string(data,0,p)).toHtmlEscaped();
            const QString b = QString::fromStdString(std::string(data,p,l)).toHtmlEscaped();
            const QString c = QString::fromStdString(std::string(data,p+l)).toHtmlEscaped();
            str=QString("<html>%1<span style='background-color:%4;'>%2</span>%3</html>").arg(a).arg(b).arg(c).arg(colors[i]);
        }
        break;
    }
    widget->setText(QString(def.prefix())
        +str
        +QString(def.suffix())
        );
}

QWidget * dcontrols_ui::gen_line(const c_id id)
{
    QWidget * parent = obtainParent();
    QFrame * widget = new QFrame(parent);
    widget->setFrameShape(QFrame::HLine);
    widget->setFrameShadow(QFrame::Sunken);
    return widget;
}

QWidget * dcontrols_ui::gen_colorpicker(const c_id id)
{
    QWidget * parent = obtainParent();
    auto & def = c_def::all()[id];
    QtColorPicker * widget = new QtColorPicker(parent);
    if(def.writable())
    {
        parent->connect(widget, &QtColorPicker::colorChanged, [=](const QColor & val){changed_gui(id, qint64(val.rgb()));} );
        if(def.shortcut())
        {
            QAction * action = new QAction(parent);
            action->setShortcut(QKeySequence(def.shortcut()));
            parent->addAction(action);
            parent->connect(action, &QAction::triggered, [=](){widget->setFocus();});
        }
    }
    for(auto it=def.presets().begin(); it!=def.presets().end();++it)
    {
        widget->insertColor(QColor::fromRgb(qint64(it->value)), it->display);
        if(it->shortcut)
        {
            QAction * action = new QAction(parent);
            action->setShortcut(QKeySequence(it->shortcut));
            parent->addAction(action);
            parent->connect(action, &QAction::triggered, [=](){widget->setCurrentColor(QColor::fromRgb(qint64(it->value)));});
        }
    }
    return widget;
}

void dcontrols_ui::set_colorpicker(QWidget * w, const c_id, const c_def::v_union &value)
{
    QtColorPicker * widget = dynamic_cast<QtColorPicker *>(w);
    widget->setCurrentColor(QColor::fromRgb(qint64(value)));
}

void dcontrols_ui::get_colorpicker(QWidget * w, const c_id, c_def::v_union &value) const
{
    QtColorPicker * widget = dynamic_cast<QtColorPicker *>(w);
    value = qint64(widget->currentColor().rgb());
}




std::array<QWidget * (dcontrols_ui::*)(const c_id), G_GUI_TYPE_COUNT> dcontrols_ui::generators{
    &dcontrols_ui::gen_none, //G_NONE
    &dcontrols_ui::gen_text,//G_TEXT,
    &dcontrols_ui::gen_spinbox,//G_SPINBOX,
    &dcontrols_ui::gen_doublespinbox,//G_DOUBLESPINBOX,
    &dcontrols_ui::gen_checkbox,//G_CHECKBOX,
    &dcontrols_ui::gen_combo,//G_COMBO,
    &dcontrols_ui::gen_stringcombo,//G_STRINGCOMBO,
    &dcontrols_ui::gen_slider,//G_SLIDER,
    &dcontrols_ui::gen_logslider,//G_LOGSLIDER,
    &dcontrols_ui::gen_rangeslider,//G_RANGESLIDER,
    &dcontrols_ui::gen_menuaction,//G_MENUACTION,
    &dcontrols_ui::gen_menucheckbox,//G_MENUCHECKBOX,
    &dcontrols_ui::gen_button,//G_BUTTON,
    &dcontrols_ui::gen_togglebutton,//G_TOGGLEBUTTON,
    &dcontrols_ui::gen_label,//G_LABEL,
    &dcontrols_ui::gen_hlabel,//G_HLABEL,
    &dcontrols_ui::gen_line,//G_LINE,
    &dcontrols_ui::gen_colorpicker,//G_COLORPICKER
};


std::array<void (dcontrols_ui::*)(QWidget*, const c_id, const c_def::v_union &), G_GUI_TYPE_COUNT> dcontrols_ui::managed_setters{
    NULL,//G_NONE
    &dcontrols_ui::set_text,//G_TEXT,
    &dcontrols_ui::set_spinbox,//G_SPINBOX,
    &dcontrols_ui::set_doublespinbox,//G_DOUBLESPINBOX,
    &dcontrols_ui::set_checkbox,//G_CHECKBOX,
    &dcontrols_ui::set_combo,//G_COMBO,
    &dcontrols_ui::set_stringcombo,//G_STRINGCOMBO,
    &dcontrols_ui::set_slider,//G_SLIDER,
    &dcontrols_ui::set_logslider,//G_LOGSLIDER,
    &dcontrols_ui::set_rangeslider,//G_RANGESLIDER,
    nullptr,//G_MENUACTION,
    &dcontrols_ui::set_menucheckbox,//G_MENUCHECKBOX,
    nullptr,//G_BUTTON,
    &dcontrols_ui::set_togglebutton,//G_TOGGLEBUTTON,
    &dcontrols_ui::set_label,//G_LABEL,
    &dcontrols_ui::set_hlabel,//G_HLABEL,
    nullptr,//G_LINE,
    &dcontrols_ui::set_colorpicker,//G_COLORPICKER
};

std::array<void (dcontrols_ui::*)(QWidget *, const c_id, c_def::v_union &) const, G_GUI_TYPE_COUNT> dcontrols_ui::managed_getters{
    nullptr,//G_NONE
    &dcontrols_ui::get_text,//G_TEXT,
    &dcontrols_ui::get_spinbox,//G_SPINBOX,
    &dcontrols_ui::get_doublespinbox,//G_DOUBLESPINBOX,
    &dcontrols_ui::get_checkbox,//G_CHECKBOX,
    &dcontrols_ui::get_combo,//G_COMBO,
    &dcontrols_ui::get_stringcombo,//G_STRINGCOMBO,
    &dcontrols_ui::get_slider,//G_SLIDER,
    &dcontrols_ui::get_logslider,//G_LOGSLIDER,
    &dcontrols_ui::get_rangeslider,//G_RANGESLIDER,
    nullptr,//G_MENUACTION,
    &dcontrols_ui::get_menucheckbox,//G_MENUCHECKBOX,
    nullptr,//G_BUTTON,
    &dcontrols_ui::get_togglebutton,//G_TOGGLEBUTTON,
    &dcontrols_ui::get_label,//G_LABEL,
    &dcontrols_ui::get_label,//G_HLABEL,
    nullptr,//G_LINE,
    &dcontrols_ui::get_colorpicker,//G_COLORPICKER
};

std::array<QMenu *, C_COUNT> dcontrols_ui::ui_menus{};
std::array<QAction *, C_COUNT> dcontrols_ui::ui_actions{};
std::array<QWidget *, C_COUNT> dcontrols_ui::ui_widgets{};

//user action
void dcontrols_ui::changed_gui(const c_id id, const c_def::v_union & value, bool trigger_observers)
{
    if(observers[id] && trigger_observers)
        observers[id](id, value);
    QWidget * widget = dynamic_cast<QWidget*>(this);
    dcontrols_ui * parent_widget=dynamic_cast<dcontrols_ui*>(widget->parentWidget());
    if(parent_widget)
        parent_widget->changed_gui(id, value, trigger_observers);
}

//state update
void dcontrols_ui::set_gui(const c_id id, const c_def::v_union & value, bool trigger_observers)
{
    dcontrols_ui * m = ui_windows[c_def::all()[id].window()];
    if(m && m!=this)
        m->set_gui(id, value, trigger_observers);
    else
        if(ui_widgets[id])
            if(managed_setters[c_def::all()[id].g_type()])
                (this->*managed_setters[c_def::all()[id].g_type()])(ui_widgets[id], id, value);
    if(observers[id] && trigger_observers)
        observers[id](id, value);
}

//current state
void dcontrols_ui::get_gui(const c_id id, c_def::v_union & value) const
{
    dcontrols_ui * m = ui_windows[c_def::all()[id].window()];
    if(m && m!=this)
    {
        m->get_gui(id, value);
        return;
    }
    if(ui_widgets[id])
    {
        if(managed_getters[c_def::all()[id].g_type()])
            (this->*managed_getters[c_def::all()[id].g_type()])(ui_widgets[id], id, value);
    }else if(getters[id])
        getters[id](id, value);
}

QWidget * dcontrols_ui::construct_widget(const c_id id)
{
    QWidget * parent = obtainParent();
    if(generators[c_def::all()[id].g_type()])
    {
        ui_widgets[id]=(this->*generators[c_def::all()[id].g_type()])(id);
        if(ui_widgets[id]==parent)
            return nullptr;
        if(ui_widgets[id]==nullptr)
            return nullptr;
        ui_widgets[id]->setToolTip(QString(c_def::all()[id].hint()));
        return ui_widgets[id];
    }
    return nullptr;
}

void dcontrols_ui::set_placement(c_def::grid_placement & what, const c_def::grid_placement & to)
{
    if(to.column==PLACE_NEXT)
        what.column+=what.colspan;
    else if(to.column!=PLACE_SAME)
        what.column=to.column;
    if(to.row==PLACE_NEXT)
        what.row+=what.rowspan;
    else if(to.row!=PLACE_SAME)
        what.row=to.row;
    what.colspan=to.colspan;
    what.rowspan=to.rowspan;
    what.align=to.align;
}

void dcontrols_ui::add_control(const c_id id)
{
    const c_def & d = c_def::all()[id];
    dcontrols_ui *win = ui_windows[d.window()];
    if(win && win != this)
        win->add_control(id);
    else{
        QWidget * control = construct_widget(id);
        if(!control)
            return;
        if(d.tab())
        {
            throw std::runtime_error("dcontrols_ui::add_control: Use dcontrols_ui_tabbed to create tabbed widget");
        }else if(d.demod_specific())
        {
            throw std::runtime_error("dcontrols_ui::add_control: Use dcontrols_ui_stacked to create stacked widget with demod_specific settings");
        }else{
            if(gridLayout)
                grid_insert(gridLayout, cur_placement, control, id);
        }
    }
}

void dcontrols_ui::grid_insert(QGridLayout * into, c_def::grid_placement & placement, QWidget * what, const c_id id)
{
    if(what==nullptr)
        return;
    const c_def & d = c_def::all()[id];
    if(d.title_placement().row!=PLACE_NONE)
    {
        set_placement(placement,d.title_placement());
        auto label = new QLabel(QString(d.title()), what->parentWidget());
        label->setTextFormat(Qt::PlainText);
        label->setToolTip(QString(c_def::all()[id].hint()));
        label->setSizePolicy(toQtSizePol(placement.align, QSizePolicy::Minimum), QSizePolicy::Minimum);
        //label->setMinimumSize(10,10);
        into->addWidget(label,placement.row,placement.column,placement.rowspan,placement.colspan,toQtAlignment(placement.align, Qt::AlignRight));
        what->setProperty("title",QVariant::fromValue(label));
    }
    if(d.placement().row!=PLACE_NONE)
    {
        set_placement(placement,d.placement());
        what->setSizePolicy(toQtSizePol(placement.align, QSizePolicy::MinimumExpanding), QSizePolicy::Minimum);
        into->addWidget(what,placement.row,placement.column,placement.rowspan,placement.colspan,toQtAlignment(placement.align, Qt::AlignmentFlag(0)));
    }
    if(d.next_placement().row!=PLACE_NONE)
    {
        set_placement(placement,d.next_placement());
        auto label = new QLabel(QString(d.next_title()), what->parentWidget());
        label->setTextFormat(Qt::PlainText);
        label->setToolTip(QString(c_def::all()[id].hint()));
        label->setSizePolicy(toQtSizePol(placement.align, QSizePolicy::Minimum), QSizePolicy::Minimum);
        //label->setMinimumSize(10,10);
        into->addWidget(label,placement.row,placement.column,placement.rowspan,placement.colspan,toQtAlignment(placement.align, Qt::AlignRight));
        what->setProperty("nextTitle",QVariant::fromValue(label));
    }
}

void dcontrols_ui::finalizeInner()
{
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    gridLayout->addWidget(spacer,cur_placement.row+1,0);
}

void dcontrols_ui::finalize()
{
    for(auto it=ui_windows.begin();it!=ui_windows.end();++it)
    {
        if(!(*it))
            continue;
        if(*it==this)
        {
            finalizeInner();
        }else
            (*it)->finalize();
    }
}

Qt::Alignment dcontrols_ui::toQtAlignment(const alignments from, const Qt::Alignment def)
{
    const alignments v = alignments(from & ALIGN_MASK);
    if(v == ALIGN_RIGHT)
        return Qt::AlignRight;
    if(v == ALIGN_LEFT)
        return Qt::AlignLeft;
    if(v == ALIGN_CENTER)
        return Qt::AlignCenter;
    return def;
}

QSizePolicy::Policy dcontrols_ui::toQtSizePol(const alignments from, const QSizePolicy::Policy def)
{
    const alignments v = alignments(from & SIZE_MASK);
    if(v == SIZE_MINIMUMEXPANDING)
        return QSizePolicy::MinimumExpanding;
    if(v == SIZE_MINIMUM)
        return QSizePolicy::Minimum;
    if(v == SIZE_MAXIMUM)
        return QSizePolicy::Maximum;
    if(v == SIZE_PREFERRED)
        return QSizePolicy::Preferred;
    if(v == SIZE_FIXED)
        return QSizePolicy::Fixed;
    if(v == SIZE_EXPANDING)
        return QSizePolicy::Expanding;
    if(v == SIZE_IGNORED)
        return QSizePolicy::Ignored;
    return def;
}

void dcontrols_ui_tabbed::add_control(const c_id id)
{
    const c_def & d = c_def::all()[id];
    dcontrols_ui *win = ui_windows[d.window()];
    if(win && win != this)
        win->add_control(id);
    else{
        QWidget * control = construct_widget(id);
        if(!control)
            return;
        if(d.tab())
        {
            const QString tabName(d.tab());
            if(!ui_tabs.contains(tabName))
            {
                QWidget * widget=obtainParent();
                if(!ui_tabWidget)
                {
                    ui_tabWidget = new QTabWidget(widget);
                    if(gridLayout)
                    {
                        set_placement(cur_placement,{PLACE_SAME,0});
                        gridLayout->addWidget(ui_tabWidget,cur_placement.row,cur_placement.column,cur_placement.rowspan,cur_placement.colspan);
                    }
                }
                ui_tabs[tabName]=new QWidget(widget);
                ui_tabs[tabName]->setLayout(new QGridLayout(ui_tabs[tabName]));
                ui_tabWidget->addTab(ui_tabs[tabName],tabName);
                ui_tabs_placements[tabName]=c_def::grid_placement(0,0);
            }
            QGridLayout * layout = dynamic_cast<QGridLayout *>(ui_tabs[tabName]->layout());
            grid_insert(layout, ui_tabs_placements[tabName], control, id);
        }else if(d.demod_specific())
        {
            throw std::runtime_error("dcontrols_ui::add_control: Use dcontrols_ui_stacked to create stacked widget with demod_specific settings");
        }else{
            if(gridLayout)
                grid_insert(gridLayout, cur_placement, control, id);
        }
    }
}

void dcontrols_ui_tabbed::finalizeInner()
{
    for(auto it=ui_tabs.begin();it!=ui_tabs.end();++it)
    {
        QWidget* spacer = new QWidget();
        QGridLayout * layout = dynamic_cast<QGridLayout *>((*it)->layout());
        spacer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        layout->addWidget(spacer,ui_tabs_placements[it.key()].row+1,0);
    }
}

void dcontrols_ui_stacked::setCurrentIndex(int n)
{
    if(ui_stackedWidget)
        ui_stackedWidget->setCurrentIndex(Modulations::modes[n].group);
}

void dcontrols_ui_stacked::add_control(const c_id id)
{
    const c_def & d = c_def::all()[id];
    dcontrols_ui *win = ui_windows[d.window()];
    if(win && win != this)
        win->add_control(id);
    else{
        QWidget * control = construct_widget(id);
        if(!control)
            return;
        if(d.tab())
        {
            throw std::runtime_error("dcontrols_ui::add_control: Use dcontrols_ui_tabbed to create tabbed widget");
        }else if(d.demod_specific())
        {
            const int idx(d.demodgroup());
            if(!ui_stackedWidget)
            {
                QWidget * widget=obtainParent();
                ui_stackedWidget = new QStackedWidget(widget);
                if(gridLayout)
                {
                    set_placement(cur_placement,{PLACE_SAME,0});
                    gridLayout->addWidget(ui_stackedWidget,cur_placement.row,cur_placement.column,cur_placement.rowspan,cur_placement.colspan);
                    ui_tabs.reserve(Modulations::GRP_COUNT);
                    ui_tabs_placements.reserve(Modulations::GRP_COUNT);
                    for(int k=0;k<Modulations::GRP_COUNT;k++)
                    {
                        ui_tabs[k]=new QWidget(widget);
                        ui_tabs[k]->setLayout(new QGridLayout(ui_tabs[k]));
                        ui_stackedWidget->addWidget(ui_tabs[k]);
                        ui_tabs_placements[k]=c_def::grid_placement(0,0);
                    }
                }
            }
            QGridLayout * layout = dynamic_cast<QGridLayout *>(ui_tabs[idx]->layout());
            grid_insert(layout, ui_tabs_placements[idx], control, id);
        }else{
            if(gridLayout)
                grid_insert(gridLayout, cur_placement, control, id);
        }
    }
}

void dcontrols_ui_stacked::finalizeInner()
{
    for(int k=0;k<Modulations::GRP_COUNT;k++)
    {
        QWidget* spacer = new QWidget();
        QGridLayout * layout = dynamic_cast<QGridLayout *>(ui_tabs[k]->layout());
        spacer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        layout->addWidget(spacer,ui_tabs_placements[k].row+1,0);
    }
}
