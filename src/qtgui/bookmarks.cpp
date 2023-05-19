/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2013 Christian Lindner DL2VCL, Stefano Leucci.
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
#include <Qt>
#include <QSet>
#include <algorithm>
#include <iostream>
#include "bookmarks.h"
#include "qtgui/bookmarkstablemodel.h"

const QColor TagInfo::DefaultColor(Qt::lightGray);
const QString TagInfo::strUntagged("Untagged");

CommaSeparated::CommaSeparated(bool useCaptions, QChar quote, QChar fieldDelimiter, QChar lineDelimiter):
        m_UseCaptions(useCaptions), m_Quote(quote), m_FieldDelimiter(fieldDelimiter), m_LineDelimiter(lineDelimiter)
{
}

CommaSeparated::~CommaSeparated()
{
    m_TS.flush();
}

bool CommaSeparated::open(const QString filename, bool write)
{
    m_FD.setFileName(filename);
    if(write)
    {
        if(m_FD.open(QFile::WriteOnly | QFile::Truncate | QIODevice::Text))
        {
            m_TS.setDevice(&m_FD);
            return true;
        }
        return false;
    }else
        return m_FD.open(QIODevice::ReadOnly | QIODevice::Text);
}

void CommaSeparated::close()
{
    m_TS.flush();
    m_FD.close();
}

bool CommaSeparated::write(QStringList & row)
{
    QString tmp{""};
    for(auto it=row.begin();it!=row.end();++it)
    {
        bool needsQuoting=false;
        QString field;
        if(it->contains(m_Quote))
        {
            needsQuoting=true;
            field=it->replace(m_Quote,QString(m_Quote)+m_Quote);
        }else
            field=*it;
        if(it->contains(m_FieldDelimiter)||it->contains(m_LineDelimiter))
            needsQuoting=true;
        if(tmp.length()>0)
            tmp+=m_FieldDelimiter;
        if(needsQuoting)
            tmp+=m_Quote+field+m_Quote;
        else
            tmp+=field;
    }
    m_TS<<tmp<<m_LineDelimiter;
    return m_FD.error()==QFileDevice::NoError;
}

bool CommaSeparated::read()
{
    int ps=0;
    int pc=0;
    bool in_quotes=false;
    bool found_quote=false;
    QString buf{""};
    m_Row.clear();
    while(true)
    {
        buf+=QString::fromUtf8(m_FD.readLine());
        if(buf.length()==0)
            return false;
        if((buf.length()==1)&&(buf[0]==m_LineDelimiter))
            {
                m_Row.append("");
                return true;
            }
        while(true)
        {
            if(in_quotes)
            {
                if(pc>=buf.length())
                {
                    pc++;
                    in_quotes=false;
                }else{
                    if(found_quote)
                    {
                        if(buf[pc]!=m_Quote)
                            in_quotes=false;
                        found_quote=false;
                    }else
                        if(buf[pc]==m_Quote)
                            found_quote=true;
                    if(buf[pc]==m_LineDelimiter)
                        break;
                }
            }
            if(!in_quotes)
            {
                if(pc>=buf.length())
                {
                    if(buf[ps]==m_Quote)
                    {
                        m_Row.append(buf.mid(ps+1,pc-ps-2).replace(QString(m_Quote)+m_Quote,QString(m_Quote)));
                    }else{
                        m_Row.append(buf.mid(ps,pc-ps));
                    }
                    return true;
                }
                if(buf[pc]==m_FieldDelimiter)
                {
                    if(buf[ps]==m_Quote)
                    {
                        m_Row.append(buf.mid(ps+1,pc-ps-2).replace(QString(m_Quote)+m_Quote,QString(m_Quote)));
                    }else{
                        m_Row.append(buf.mid(ps,pc-ps));
                    }
                    ps=pc+1;
                }
                if(buf[pc]==m_LineDelimiter)
                {
                    if(buf[ps]==m_Quote)
                    {
                        m_Row.append(buf.mid(ps+1,pc-ps-2).replace(QString(m_Quote)+m_Quote,QString(m_Quote)));
                    }else{
                        m_Row.append(buf.mid(ps,pc-ps));
                    }
                    return true;
                }
                if(buf[pc]==m_Quote)
                    in_quotes=true;
            }
            pc++;
        }
    }
}

Bookmarks::Bookmarks()
{
     TagInfo::sptr tag = TagInfo::make(TagInfo::strUntagged);
     m_TagList.append(tag);
     m_idx_struct.append({T:V_INT,     name:"Frequency",col:BookmarksTableModel::COL_FREQUENCY,ref:-1,
        genSetter(&BookmarkInfo::set_frequency), genGetter(&BookmarkInfo::get_frequency), genCmp(&BookmarkInfo::get_frequency)});
     m_idx_struct.append({T:V_STRING,  name:"Name",col:BookmarksTableModel::COL_NAME,ref:-1,
        genSetter(&BookmarkInfo::set_name), genGetter(&BookmarkInfo::get_name), genCmp(&BookmarkInfo::get_name)});
     m_idx_struct.append({T:V_STRING,  name:"Tags",col:BookmarksTableModel::COL_TAGS,ref:-1,
        genSetter(&BookmarkInfo::set_tags), genGetter(&BookmarkInfo::get_tags), genCmp(&BookmarkInfo::get_tags)});
     m_idx_struct.append({T:V_BOOLEAN, name:"Autostart",col:BookmarksTableModel::COL_LOCKED,ref:-1,
        genSetter(&BookmarkInfo::set_freq_lock), genGetter(&BookmarkInfo::get_freq_lock), genCmp(&BookmarkInfo::get_freq_lock)});
     m_idx_struct.append({T:V_INT,     name:"Modulation",col:BookmarksTableModel::COL_MODULATION,ref:-1,
        genSetter(&BookmarkInfo::set_demod), genGetter(&BookmarkInfo::get_demod), genCmp(&BookmarkInfo::get_demod)});
     m_idx_struct.append({T:V_INT,     name:"Filter Low",col:BookmarksTableModel::COL_FILTER_LOW,ref:-1,
        genSetter(&BookmarkInfo::set_filter_low), genGetter(&BookmarkInfo::get_filter_low), genCmp(&BookmarkInfo::get_filter_low)});
     m_idx_struct.append({T:V_INT,     name:"Filter High",col:BookmarksTableModel::COL_FILTER_HIGH,ref:-1,
        genSetter(&BookmarkInfo::set_filter_high), genGetter(&BookmarkInfo::get_filter_high), genCmp(&BookmarkInfo::get_filter_high)});
     m_idx_struct.append({T:V_INT,     name:"Filter TW",col:BookmarksTableModel::COL_FILTER_TW,ref:-1,
        genSetter(&BookmarkInfo::set_filter_tw), genGetter(&BookmarkInfo::get_filter_tw), genCmp(&BookmarkInfo::get_filter_tw)});
     m_idx_struct.append({T:V_BOOLEAN, name:"AGC On",col:BookmarksTableModel::COL_AGC_ON,ref:-1,
        genSetter(&BookmarkInfo::set_agc_on), genGetter(&BookmarkInfo::get_agc_on), genCmp(&BookmarkInfo::get_agc_on)});
     m_idx_struct.append({T:V_INT,     name:"AGC target level",col:BookmarksTableModel::COL_AGC_TARGET,ref:-1,
        genSetter(&BookmarkInfo::set_agc_target_level), genGetter(&BookmarkInfo::get_agc_target_level), genCmp(&BookmarkInfo::get_agc_target_level)});
     m_idx_struct.append({T:V_DOUBLE,  name:"AGC manual gain",col:BookmarksTableModel::COL_AGC_MANUAL,ref:-1,
        genSetter(&BookmarkInfo::set_agc_manual_gain), genGetter(&BookmarkInfo::get_agc_manual_gain), genCmp(&BookmarkInfo::get_agc_manual_gain)});
     m_idx_struct.append({T:V_INT,     name:"AGC max gain",col:BookmarksTableModel::COL_AGC_MAX,ref:-1,
        genSetter(&BookmarkInfo::set_agc_max_gain), genGetter(&BookmarkInfo::get_agc_max_gain), genCmp(&BookmarkInfo::get_agc_max_gain)});
     m_idx_struct.append({T:V_INT,     name:"AGC attack",col:BookmarksTableModel::COL_AGC_ATTACK,ref:-1,
        genSetter(&BookmarkInfo::set_agc_attack), genGetter(&BookmarkInfo::get_agc_attack), genCmp(&BookmarkInfo::get_agc_attack)});
     m_idx_struct.append({T:V_INT,     name:"AGC decay",col:BookmarksTableModel::COL_AGC_DECAY,ref:-1,
        genSetter(&BookmarkInfo::set_agc_decay), genGetter(&BookmarkInfo::get_agc_decay), genCmp(&BookmarkInfo::get_agc_decay)});
     m_idx_struct.append({T:V_INT,     name:"AGC hang",col:BookmarksTableModel::COL_AGC_HANG,ref:-1,
        genSetter(&BookmarkInfo::set_agc_hang), genGetter(&BookmarkInfo::get_agc_hang), genCmp(&BookmarkInfo::get_agc_hang)});
     m_idx_struct.append({T:V_INT,     name:"Panning",col:BookmarksTableModel::COL_AGC_PANNING,ref:-1,
        genSetter(&BookmarkInfo::set_agc_panning), genGetter(&BookmarkInfo::get_agc_panning), genCmp(&BookmarkInfo::get_agc_panning)});
     m_idx_struct.append({T:V_BOOLEAN, name:"Auto panning",col:BookmarksTableModel::COL_AGC_PANNING_AUTO,ref:-1,
        genSetter(&BookmarkInfo::set_agc_panning_auto), genGetter(&BookmarkInfo::get_agc_panning_auto), genCmp(&BookmarkInfo::get_agc_panning_auto)});
     m_idx_struct.append({T:V_INT,     name:"CW offset",col:BookmarksTableModel::COL_CW_OFFSET,ref:-1,
        genSetter(&BookmarkInfo::set_cw_offset), genGetter(&BookmarkInfo::get_cw_offset), genCmp(&BookmarkInfo::get_cw_offset)});
     m_idx_struct.append({T:V_DOUBLE,  name:"FM max deviation",col:BookmarksTableModel::COL_FM_MAXDEV,ref:-1,
        genSetter(&BookmarkInfo::set_fm_maxdev), genGetter(&BookmarkInfo::get_fm_maxdev), genCmp(&BookmarkInfo::get_fm_maxdev)});
     m_idx_struct.append({T:V_DOUBLE,  name:"FM deemphasis",col:BookmarksTableModel::COL_FM_DEEMPH,ref:-1,
        genSetter(&BookmarkInfo::set_fm_deemph), genGetter(&BookmarkInfo::get_fm_deemph), genCmp(&BookmarkInfo::get_fm_deemph)});
     m_idx_struct.append({T:V_BOOLEAN, name:"AM DCR",col:BookmarksTableModel::COL_AM_DCR,ref:-1,
        genSetter(&BookmarkInfo::set_am_dcr), genGetter(&BookmarkInfo::get_am_dcr), genCmp(&BookmarkInfo::get_am_dcr)});
     m_idx_struct.append({T:V_BOOLEAN, name:"AM SYNC DCR",col:BookmarksTableModel::COL_AMSYNC_DCR,ref:-1,
        genSetter(&BookmarkInfo::set_amsync_dcr), genGetter(&BookmarkInfo::get_amsync_dcr), genCmp(&BookmarkInfo::get_amsync_dcr)});
     m_idx_struct.append({T:V_DOUBLE,  name:"PLL BW",col:BookmarksTableModel::COL_PLL_BW,ref:-1,
        genSetter(&BookmarkInfo::set_pll_bw), genGetter(&BookmarkInfo::get_pll_bw), genCmp(&BookmarkInfo::get_pll_bw)});
     m_idx_struct.append({T:V_BOOLEAN, name:"NB1 ON",col:BookmarksTableModel::COL_NB1_ON,ref:-1,
        genSetterN(&BookmarkInfo::set_nb_on, 1), genGetterN(&BookmarkInfo::get_nb_on, 1), genCmpN(&BookmarkInfo::get_nb_on, 1)});
     m_idx_struct.append({T:V_DOUBLE,  name:"NB1 threshold",col:BookmarksTableModel::COL_NB1_THRESHOLD,ref:-1,
        genSetterN(&BookmarkInfo::set_nb_threshold, 1), genGetterN(&BookmarkInfo::get_nb_threshold, 1), genCmpN(&BookmarkInfo::get_nb_threshold, 1)});
     m_idx_struct.append({T:V_BOOLEAN, name:"NB2 ON",col:BookmarksTableModel::COL_NB2_ON,ref:-1,
        genSetterN(&BookmarkInfo::set_nb_on, 2), genGetterN(&BookmarkInfo::get_nb_on, 2), genCmpN(&BookmarkInfo::get_nb_on, 2)});
     m_idx_struct.append({T:V_DOUBLE,  name:"NB2 threshold",col:BookmarksTableModel::COL_NB2_THRESHOLD,ref:-1,
        genSetterN(&BookmarkInfo::set_nb_threshold, 2), genGetterN(&BookmarkInfo::get_nb_threshold, 2), genCmpN(&BookmarkInfo::get_nb_threshold, 2)});
     m_idx_struct.append({T:V_STRING,  name:"REC DIR",col:BookmarksTableModel::COL_REC_DIR,ref:-1,
        genSetterCS(&BookmarkInfo::set_audio_rec_dir), genGetterCS(&BookmarkInfo::get_audio_rec_dir), genCmp(&BookmarkInfo::get_audio_rec_dir)});
     m_idx_struct.append({T:V_BOOLEAN, name:"REC SQL trig",col:BookmarksTableModel::COL_REC_SQL_TRIGGERED,ref:-1,
        genSetter(&BookmarkInfo::set_audio_rec_sql_triggered), genGetter(&BookmarkInfo::get_audio_rec_sql_triggered), genCmp(&BookmarkInfo::get_audio_rec_sql_triggered)});
     m_idx_struct.append({T:V_INT,     name:"REC Min time",col:BookmarksTableModel::COL_REC_MIN_TIME,ref:-1,
        genSetter(&BookmarkInfo::set_audio_rec_min_time), genGetter(&BookmarkInfo::get_audio_rec_min_time), genCmp(&BookmarkInfo::get_audio_rec_min_time)});
     m_idx_struct.append({T:V_INT,     name:"REC Max gap",col:BookmarksTableModel::COL_REC_MAX_GAP,ref:-1,
        genSetter(&BookmarkInfo::set_audio_rec_max_gap), genGetter(&BookmarkInfo::get_audio_rec_max_gap), genCmp(&BookmarkInfo::get_audio_rec_max_gap)});

     m_col_struct.resize(m_idx_struct.length());
     for(int k=0;k<m_idx_struct.length();k++)
     {
            if(m_idx_struct[k].ref>=0)
                m_col_struct[m_idx_struct[k].ref]=&m_idx_struct[k];
            m_name_struct[m_idx_struct[k].name]=&m_idx_struct[k];
     }
}

Bookmarks& Bookmarks::Get()
{
    static Bookmarks intern;
    return intern;
}

void Bookmarks::setConfigDir(const QString& cfg_dir)
{
    m_bookmarksFile = cfg_dir + "/bookmarks.csv";
    std::cout << "BookmarksFile is " << m_bookmarksFile.toStdString() << std::endl;
}

void Bookmarks::add(BookmarkInfo &info)
{
    m_BookmarkList.append(info);
    std::stable_sort(m_BookmarkList.begin(),m_BookmarkList.end());
    save();
}

void Bookmarks::remove(int index)
{
    m_BookmarkList.removeAt(index);
    save();
}

void Bookmarks::remove(const BookmarkInfo &info)
{
    m_BookmarkList.removeOne(info);
    save();
}

bool Bookmarks::load()
{
    QFile file(m_bookmarksFile);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        m_BookmarkList.clear();
        m_TagList.clear();

        // always create the "Untagged" entry.
        findOrAddTag(TagInfo::strUntagged);

        // Read Tags, until first empty line.
        while (!file.atEnd())
        {
            QString line = QString::fromUtf8(file.readLine().trimmed());

            if(line.isEmpty())
                break;

            if(line.startsWith("#"))
                continue;

            QStringList strings = line.split(";");
            if(strings.count() == 2)
            {
                TagInfo::sptr info = findOrAddTag(strings[0]);
                info->color = QColor(strings[1].trimmed());
            }
            else
            {
                std::cout << "Bookmarks: Ignoring Line:" << std::endl;
                std::cout << "  " << line.toStdString() << std::endl;
            }
        }
        std::sort(m_TagList.begin(),m_TagList.end());

        // Read Bookmarks, after first empty line.
        while (!file.atEnd())
        {
            QString line = QString::fromUtf8(file.readLine().trimmed());
            if(line.isEmpty() || line.startsWith("#"))
                continue;

            QStringList strings = line.split(";");
            if(strings.count() == 5)
            {
                BookmarkInfo info;
                m_idx_struct[0].fromString(info, strings[0].trimmed());
                m_idx_struct[1].fromString(info, strings[1].trimmed());
                m_idx_struct[4].fromString(info, strings[2].trimmed());
                int bandwidth  = strings[3].toInt();
                switch(info.get_demod())
                {
                case Modulations::MODE_LSB:
                case Modulations::MODE_CWL:
                    info.set_filter(-100 - bandwidth, -100, bandwidth * 0.2);
                    break;
                case Modulations::MODE_USB:
                case Modulations::MODE_CWU:
                    info.set_filter(100, 100 + bandwidth, bandwidth * 0.2);
                    break;
                default:
                    info.set_filter(-bandwidth / 2, bandwidth / 2, bandwidth * 0.2);
                }
                m_idx_struct[2].fromString(info, strings[4].trimmed());

                m_BookmarkList.append(info);
            }
            else if (strings.count() > 5)
            {
                BookmarkInfo info;
                int i = 0;
                for(i=0;i<strings.count();i++)
                    if(i<m_idx_struct.length())
                        m_idx_struct[i].fromString(info, strings[i].trimmed());
                    else
                        std::cout << "Bookmarks: Ignoring Column "<<i<<" = " << strings[i].trimmed().toStdString() << std::endl;

                m_BookmarkList.append(info);
            }
            else
            {
                std::cout << "Bookmarks: Ignoring Line:" << std::endl;
                std::cout << "  " << line.toStdString() << std::endl;
            }
        }
        file.close();
        std::stable_sort(m_BookmarkList.begin(),m_BookmarkList.end());

        emit BookmarksChanged();
        return true;
    }
    return false;
}

//FIXME: Commas in names
bool Bookmarks::save()
{
    QFile file(m_bookmarksFile);
    if(file.open(QFile::WriteOnly | QFile::Truncate | QIODevice::Text))
    {
        QString tmp;
        QTextStream stream(&file);

        stream << QString("# Tag name").leftJustified(20) + "; " +
                  QString(" color") << '\n';

        QMap<QString, TagInfo::sptr> usedTags;
        for (int iBookmark = 0; iBookmark < m_BookmarkList.size(); iBookmark++)
        {
            BookmarkInfo& info = m_BookmarkList[iBookmark];
            for (QList<TagInfo::sptr>::iterator iTag = info.tags.begin(); iTag < info.tags.end(); ++iTag)
            {
              usedTags.insert((*iTag)->name, *iTag);
            }
        }

        for (QMap<QString, TagInfo::sptr>::iterator i = usedTags.begin(); i != usedTags.end(); i++)
        {
            TagInfo::sptr info = *i;
            stream << info->name.leftJustified(20) + "; " + info->color.name() << '\n';
        }

        stream << '\n';

        tmp = "# ";
        for(auto it = m_idx_struct.begin(); it != m_idx_struct.end(); ++it)
            if(it == m_idx_struct.begin())
                tmp += it->name;
            else
                tmp += ";" + it->name;

        stream << tmp  << '\n';

        for (int i = 0; i < m_BookmarkList.size(); i++)
        {
            BookmarkInfo& info = m_BookmarkList[i];
            tmp = "";
            for(auto it = m_idx_struct.begin(); it != m_idx_struct.end(); ++it)
                if(it == m_idx_struct.begin())
                    tmp += it->toString(info);
                else
                    tmp += ";" + it->toString(info);

            stream << tmp  << '\n';
        }

        emit BookmarksChanged();
        file.close();
        return true;
    }
    return false;
}

QList<BookmarkInfo> Bookmarks::getBookmarksInRange(qint64 low, qint64 high, bool autoAdded)
{
    BookmarkInfo info;
    info.frequency = low;
    QList<BookmarkInfo>::const_iterator lb = std::lower_bound(m_BookmarkList.begin(), m_BookmarkList.end(), info);
    info.frequency=high;
    QList<BookmarkInfo>::const_iterator ub = std::upper_bound(m_BookmarkList.begin(), m_BookmarkList.end(), info);

    QList<BookmarkInfo> found;

    while (lb != ub)
    {
        const BookmarkInfo& info = *lb;
        if ((!autoAdded || lb->get_freq_lock()) && info.IsActive())
            found.append(info);
        lb++;
    }

    return found;

}

int Bookmarks::find(const BookmarkInfo &info)
{
    return m_BookmarkList.indexOf(info);
}


TagInfo::sptr Bookmarks::findOrAddTag(QString tagName)
{
    tagName = tagName.trimmed();

    if (tagName.isEmpty())
        tagName=TagInfo::strUntagged;

    int idx = getTagIndex(tagName);

    if (idx != -1)
        return m_TagList[idx];

    TagInfo::sptr info = TagInfo::make(tagName);
    m_TagList.append(info);
    emit TagListChanged();
    return m_TagList.last();
}

bool Bookmarks::removeTag(QString tagName)
{
    tagName = tagName.trimmed();

    // Do not delete "Untagged" tag.
    if(tagName.compare(TagInfo::strUntagged, tagName)==0)
        return false;

    int idx = getTagIndex(tagName);
    if (idx == -1)
        return false;

    // Delete Tag from all Bookmarks that use it.
    TagInfo::sptr pTagToDelete = m_TagList[idx];
    for(int i=0; i < m_BookmarkList.size(); ++i)
    {
        BookmarkInfo& bmi = m_BookmarkList[i];
        for(int t=0; t<bmi.tags.size(); ++t)
        {
            TagInfo::sptr pTag = bmi.tags[t];
            if(pTag.get() == pTagToDelete.get())
            {
                if(bmi.tags.size()>1) bmi.tags.removeAt(t);
                else bmi.tags[0] = findOrAddTag(TagInfo::strUntagged);
            }
        }
    }

    // Delete Tag.
    m_TagList.removeAt(idx);

    emit TagListChanged();
    save();

    return true;
}

bool Bookmarks::setTagChecked(QString tagName, bool bChecked)
{
    int idx = getTagIndex(tagName);
    if (idx == -1) return false;
    m_TagList[idx]->active = bChecked;
    emit BookmarksChanged();
    emit TagListChanged();
    return true;
}

int Bookmarks::getTagIndex(QString tagName)
{
    tagName = tagName.trimmed();
    for (int i = 0; i < m_TagList.size(); i++)
    {
        if (m_TagList[i]->name == tagName)
            return i;
    }

    return -1;
}

const QColor BookmarkInfo::GetColor() const
{
    for(int iTag=0; iTag<tags.size(); ++iTag)
    {
        TagInfo::sptr tag = tags[iTag];
        if(tag->active)
        {
            return tag->color;
        }
    }
    return TagInfo::DefaultColor;
}

bool BookmarkInfo::IsActive() const
{
    bool bActive = false;
    for(int iTag=0; iTag<tags.size(); ++iTag)
    {
        TagInfo::sptr tag = tags[iTag];
        if(tag->active)
        {
            bActive = true;
            break;
        }
    }
    return bActive;
}

QString BookmarkInfo::get_tags() const
{
    QString line{""};
    for (int iTag = 0; iTag < tags.size(); ++iTag)
        {
            TagInfo::sptr tag = tags[iTag];
            if (iTag!=0)
                line.append(",");
            line.append(tag->name);
        }
    return line;
}

void BookmarkInfo::set_tags(QString newTags)
{
    QStringList TagList = newTags.split(",");
    tags.clear();
    for(int iTag=0; iTag<TagList.size(); ++iTag)
        tags.append(Bookmarks::Get().findOrAddTag(TagList[iTag].trimmed()));
}
