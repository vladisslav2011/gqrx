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
#include <clocale>
#include "qtgui/bookmarkstablemodel.h"

const QColor TagInfo::DefaultColor(Qt::lightGray);
const QString TagInfo::strUntagged("Untagged");

CommaSeparated::CommaSeparated(QChar quote, QChar fieldDelimiter, QChar lineDelimiter):
        m_Quote(quote), m_FieldDelimiter(fieldDelimiter), m_LineDelimiter(lineDelimiter)
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

bool CommaSeparated::write(const QStringList & row)
{
    QString tmp{""};
    for(auto it=row.begin();it!=row.end();++it)
    {
        bool needsQuoting=false;
        QString field=*it;
        if(it->contains(m_Quote))
        {
            needsQuoting=true;
            field=field.replace(m_Quote,QString(m_Quote)+m_Quote);
        }
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

bool CommaSeparated::read(QStringList & row)
{
    int ps=0;
    int pc=0;
    bool in_quotes=false;
    bool found_quote=false;
    QString buf{""};
    row.clear();
    while(true)
    {
        buf+=QString::fromUtf8(m_FD.readLine());
        if(buf.length()==0)
            return false;
        if((buf.length()==1)&&(buf[0]==m_LineDelimiter))
            {
                row.append("");
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
                        row.append(buf.mid(ps+1,pc-ps-2).replace(QString(m_Quote)+m_Quote,QString(m_Quote)));
                    }else{
                        row.append(buf.mid(ps,pc-ps));
                    }
                    return true;
                }
                if(buf[pc]==m_FieldDelimiter)
                {
                    if(buf[ps]==m_Quote)
                    {
                        row.append(buf.mid(ps+1,pc-ps-2).replace(QString(m_Quote)+m_Quote,QString(m_Quote)));
                    }else{
                        row.append(buf.mid(ps,pc-ps));
                    }
                    ps=pc+1;
                }
                if(buf[pc]==m_LineDelimiter)
                {
                    if(buf[ps]==m_Quote)
                    {
                        row.append(buf.mid(ps+1,pc-ps-2).replace(QString(m_Quote)+m_Quote,QString(m_Quote)));
                    }else{
                        row.append(buf.mid(ps,pc-ps));
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

bool CommaSeparated::read(const QStringList & captions, QMap<QString,QString> & row)
{
    if(!read(m_Row))
        return false;
    row.clear();
    const auto rowCount=m_Row.length();
    const auto captCount=captions.length();
    for(int k=0;k<captCount;k++)
        if(k<rowCount)
            row[captions[k]]=m_Row[k];
    return true;
}

bool CommaSeparated::read()
{
    return read(m_Row);
}

Bookmarks::Bookmarks()
{
     TagInfo::sptr tag = TagInfo::make(TagInfo::strUntagged);
     m_TagList.append(tag);
     m_idx_struct.append({V_INT,     "Frequency",//0
        genSetter(&BookmarkInfo::set_frequency), genGetter(&BookmarkInfo::get_frequency), genCmp(&BookmarkInfo::get_frequency)});
     m_idx_struct.append({V_STRING,  "Name",//1
        genSetter(&BookmarkInfo::set_name), genGetter(&BookmarkInfo::get_name), genCmp(&BookmarkInfo::get_name)});
     m_idx_struct.append({V_STRING,  "Tags",//2
        genSetter(&BookmarkInfo::set_tags), genGetter(&BookmarkInfo::get_tags), genCmp(&BookmarkInfo::get_tags)});
     m_idx_struct.append({V_BOOLEAN, "Autostart",//3
        genSetter(&BookmarkInfo::set_autostart), genGetter(&BookmarkInfo::get_autostart), genCmp(&BookmarkInfo::get_autostart)});
     m_idx_struct.append({V_INT,"",nullptr,nullptr,nullptr}); //4 Modulation
     m_idx_struct.append({V_INT,"",nullptr,nullptr,nullptr}); //5 Filter Low
     m_idx_struct.append({V_INT,"",nullptr,nullptr,nullptr}); //6 Filter High
     m_idx_struct.append({V_INT,"",nullptr,nullptr,nullptr}); //7 Filter Shape

     m_idx_struct.append({V_BOOLEAN,"",nullptr,nullptr,nullptr}); //8 AGC On
     m_idx_struct.append({V_INT,"",nullptr,nullptr,nullptr}); //9 AGC Target Level
     m_idx_struct.append({V_DOUBLE,"",nullptr,nullptr,nullptr}); //10 AGC Manual Gain
     m_idx_struct.append({V_INT,"",nullptr,nullptr,nullptr}); //11 AGC Max Gain
     m_idx_struct.append({V_INT,"",nullptr,nullptr,nullptr}); //12 AGC Attack
     m_idx_struct.append({V_INT,"",nullptr,nullptr,nullptr}); //13 AGC Decay
     m_idx_struct.append({V_INT,"",nullptr,nullptr,nullptr}); //14 AGC Hang
     m_idx_struct.append({V_INT,"",nullptr,nullptr,nullptr}); //15 Panning
     m_idx_struct.append({V_BOOLEAN,"",nullptr,nullptr,nullptr}); //16 Auto Panning

     m_idx_struct.append({V_INT,"",nullptr,nullptr,nullptr}); //17 CW Offset
     m_idx_struct.append({V_DOUBLE,"",nullptr,nullptr,nullptr}); //18 FM Max Deviation
     m_idx_struct.append({V_DOUBLE,"",nullptr,nullptr,nullptr}); //19 FM Deemphasis
     m_idx_struct.append({V_BOOLEAN,"",nullptr,nullptr,nullptr}); //20 AM DCR
     m_idx_struct.append({V_BOOLEAN,"",nullptr,nullptr,nullptr}); //21 AM SYNC DCR
     m_idx_struct.append({V_DOUBLE,"",nullptr,nullptr,nullptr}); //22 PLL BW
     m_idx_struct.append({V_BOOLEAN,"",nullptr,nullptr,nullptr}); //23 NB1 ON
     m_idx_struct.append({V_DOUBLE,"",nullptr,nullptr,nullptr}); //24 NB1 Threshold
     m_idx_struct.append({V_BOOLEAN,"",nullptr,nullptr,nullptr}); //25 NB2 ON
     m_idx_struct.append({V_DOUBLE,"",nullptr,nullptr,nullptr}); //26 NB2 Threshold
     m_idx_struct.append({V_STRING,"",nullptr,nullptr,nullptr}); // 27 REC DIR
     m_idx_struct.append({V_BOOLEAN,"",nullptr,nullptr,nullptr}); //28 REC SQL Trig
     m_idx_struct.append({V_INT,"",nullptr,nullptr,nullptr}); //29 REC Min Time
     m_idx_struct.append({V_INT,"",nullptr,nullptr,nullptr}); //30 REC Max Gap

    int maxIdx=-1;
    const auto & defs = c_def::all();
    for(int k=0;k<C_COUNT;k++)
        maxIdx = std::max(defs[k].bookmarks_column(), maxIdx);
    if(maxIdx >= m_idx_struct.length())
        m_idx_struct.insert(m_idx_struct.end(), maxIdx - m_idx_struct.length() + 1, {V_INT,"",nullptr,nullptr,nullptr});

    for(int k=0;k<C_COUNT;k++)
    {
        const int bcol=defs[k].bookmarks_column();
        if(bcol>0)
        {
            if(!defs[k].bookmarks_key())
                continue;
            if(m_idx_struct[bcol].fromString)
            {
                std::cerr<<"Conflicting bookmark definitions:\nlocal["<<
                    bcol<<"]:"<<m_idx_struct[bcol].name.toStdString()<<" and c_def["<<
                    k<<"]: "<<defs[k].name()<<" "<<defs[k].bookmarks_key()<<
                    "\nRemove local or set bookmarks_column to "<<m_idx_struct.count()<<" to fix this error\n";
                exit(1);
            }
            m_idx_struct[bcol].name=QString::fromStdString(defs[k].bookmarks_key());
            m_idx_struct[bcol].fromString=[=](BookmarkInfo & to, const QString & from) -> bool {
                //lookup preset
                auto it = defs[k].presets().find(from.toStdString());
                if(it != defs[k].presets().end())
                    return to.set_value(c_id(k),it->value);
                //no preset
                c_def::v_union tmp(defs[k].v_type(),from.toStdString());
                //TODO: validate tmp
                defs[k].clip(tmp);
                return to.set_value(c_id(k), tmp);
            };
            m_idx_struct[bcol].toString=[=](BookmarkInfo & from) -> QString {
                c_def::v_union tmp;
                from.get_value(c_id(k), tmp);
                //lookup preset
                auto it = defs[k].presets().find(tmp);
                if(it != defs[k].presets().end())
                    return QString(it->key);
                //no preset
                switch(defs[k].v_type())
                {
                case V_BOOLEAN:
                    return bool(tmp)?"true":"false";
                case V_INT:
                    return QString::number(qint64(tmp));
                case V_DOUBLE:
                    return QString::number(double(tmp),'f',defs[k].frac_digits());
                case V_STRING:
                    return QString::fromStdString(tmp);
                }
                return QString::fromStdString(tmp.to_string());
            };
            m_idx_struct[bcol].cmp=[=](const BookmarkInfo & a, const BookmarkInfo & b) -> int {
                c_def::v_union v_a, v_b;
                a.get_value(c_id(k), v_a);
                b.get_value(c_id(k), v_b);
                return (v_a>v_b)?1:((v_a<v_b)?-1:0);
            };
        }
    }

    for(int k=0;k<m_idx_struct.length();k++)
    {
        if(!m_idx_struct[k].fromString)
        {
            std::cerr<<"Malformed bookmark definition["<<k<<"]: "<<
            m_idx_struct[k].name.toStdString()<<"\nMissing index?\n";
            exit(1);
        }
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
    m_bookmarksFileOld = cfg_dir + "/bookmarks.csv";
    m_bookmarksFile = cfg_dir + "/bookmarks2.csv";
    m_tagsFile = cfg_dir + "/tags2.csv";
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
    CommaSeparated csv;
    if(csv.open(m_tagsFile, false))
    {
        std::string prev_loc = std::setlocale(LC_ALL, nullptr);
        std::setlocale(LC_ALL, "C");
        m_TagList.clear();
        // always create the "Untagged" entry.
        findOrAddTag(TagInfo::strUntagged);

        QStringList firstRow;
        if(csv.read(firstRow))
        {
            int nameIdx=-1;
            int colorIdx=-1;
            for(int k=0;k<firstRow.count();k++)
            {
                if(firstRow[k]=="name")
                    nameIdx=k;
                if(firstRow[k]=="color")
                    colorIdx=k;
            }
            if((nameIdx>=0) && (colorIdx>=0))
                while (1)
                {
                    QStringList & row=csv.getRowRef();
                    if(!csv.read())
                        break;
                    if(row.count()<1)
                        break;
                    TagInfo::sptr info = findOrAddTag(row[nameIdx]);
                    info->color = QColor(row[colorIdx].trimmed());
                }
        }
        csv.close();
        std::sort(m_TagList.begin(),m_TagList.end());
        if(csv.open(m_bookmarksFile, false))
        {
            m_BookmarkList.clear();
            QStringList firstRow;
            if(csv.read(firstRow))
            {
                QVector<Def*> kv;
                kv.resize(firstRow.count());
                for(int k=0;k<firstRow.count();k++)
                {
                    if(m_name_struct.contains(firstRow[k]))
                        kv[k]=m_name_struct[firstRow[k]];
                }
                while (1)
                {
                    QStringList & row = csv.getRowRef();
                    if(!csv.read())
                        break;
                    if(row.count()<1)
                        break;
                    if(row.count()==firstRow.count())
                    {
                        BookmarkInfo info;
                        int i = 0;
                        for(i=0;i<row.count();i++)
                            if(kv[i])
                                kv[i]->fromString(info,row[i]);
                            else
                                std::cout << "Bookmarks: Ignoring Column \""<< firstRow[i].toStdString() <<"\" #"<<i<<" = " << row[i].toStdString() << std::endl;
                        m_BookmarkList.append(info);
                    }else{
                        std::cout << "Bookmarks: Ignoring Line:" << std::endl;
                        std::cout << "  " << row.join(";").toStdString() << std::endl;
                    }
                }
            }
            csv.close();
            std::stable_sort(m_BookmarkList.begin(),m_BookmarkList.end());

            emit BookmarksChanged();
            std::setlocale(LC_ALL, prev_loc.c_str());
            return true;
        }
    }
    if(csv.open(m_bookmarksFileOld, false))
    {
        m_BookmarkList.clear();
        m_TagList.clear();

        // always create the "Untagged" entry.
        findOrAddTag(TagInfo::strUntagged);

        // Read Tags, until first empty line.
        while (1)
        {
            QStringList & row = csv.getRowRef();
            if(!csv.read())
                break;

            if(row.count()<1)
                break;
            if((row.count()==1) && (row[0]==""))
                break;

            if(row[0].startsWith("#"))
                continue;

            if(row.count() == 2)
            {
                TagInfo::sptr info = findOrAddTag(row[0]);
                info->color = QColor(row[1].trimmed());
            }
            else
            {
                std::cout << "Bookmarks: Ignoring Line:" << std::endl;
                std::cout << "  " << row.join(";").toStdString() << std::endl;
            }
        }
        std::sort(m_TagList.begin(),m_TagList.end());

        // Read Bookmarks, after first empty line.
        while (1)
        {
            QStringList & row = csv.getRowRef();
            if(!csv.read())
                break;

            if(row.count()<1)
                break;
            if((row.count()==1) && (row[0]==""))
                break;

            if(row[0].startsWith("#"))
                continue;
            if(row.count() == 5)
            {
                BookmarkInfo info;
                m_idx_struct[0].fromString(info, row[0].trimmed());
                m_idx_struct[1].fromString(info, row[1].trimmed());
                m_idx_struct[4].fromString(info, row[2].trimmed());
                int bandwidth  = row[3].toInt();
                switch(info.get_demod())
                {
                case Modulations::MODE_LSB:
                case Modulations::MODE_CWL:
                    info.set_filter(-100 - bandwidth, -100, Modulations::FILTER_SHAPE_NORMAL);
                    break;
                case Modulations::MODE_USB:
                case Modulations::MODE_CWU:
                    info.set_filter(100, 100 + bandwidth, Modulations::FILTER_SHAPE_NORMAL);
                    break;
                default:
                    info.set_filter(-bandwidth / 2, bandwidth / 2, Modulations::FILTER_SHAPE_NORMAL);
                }
                m_idx_struct[2].fromString( info, row[4].trimmed());

                m_BookmarkList.append(info);
            }
            else if (row.count() > 5)
            {
                BookmarkInfo info;
                int i = 0;
                for(i=0;i<row.count();i++)
                    if(i<m_idx_struct.length())
                        m_idx_struct[i].fromString(info, row[i].trimmed());
                    else
                        std::cout << "Bookmarks: Ignoring Column "<<i<<" = " << row[i].trimmed().toStdString() << std::endl;

                m_BookmarkList.append(info);
            }
            else
            {
                std::cout << "Bookmarks: Ignoring Line:" << std::endl;
                std::cout << "  " << row.join(";").toStdString() << std::endl;
            }
        }
        csv.close();
        std::stable_sort(m_BookmarkList.begin(),m_BookmarkList.end());

        emit BookmarksChanged();
        return true;
    }
    return false;
}

bool Bookmarks::save()
{
    CommaSeparated csv;
    if(csv.open(m_tagsFile, true))
    {
        std::string prev_loc = std::setlocale(LC_ALL, nullptr);
        std::setlocale(LC_ALL, "C");
        QString tmp;
        csv.write(QStringList({"name","color"}));
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
            csv.write(QStringList({info->name,info->color.name()}));
        }
        csv.close();
        if(csv.open(m_bookmarksFile, true))
        {
            QString tmp;

            QStringList lst;
            for(auto it = m_idx_struct.begin(); it != m_idx_struct.end(); ++it)
                lst.append(it->name);

            csv.write(lst);
            for (int i = 0; i < m_BookmarkList.size(); i++)
            {
                BookmarkInfo& info = m_BookmarkList[i];
                lst.clear();
                for(auto it = m_idx_struct.begin(); it != m_idx_struct.end(); ++it)
                    lst.append(it->toString(info));
                csv.write(lst);
            }

            csv.close();
            emit BookmarksChanged();
            std::setlocale(LC_ALL, prev_loc.c_str());
            return true;
        }
        std::setlocale(LC_ALL, prev_loc.c_str());
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
        if ((!autoAdded || lb->get_autostart()) && info.IsActive())
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
