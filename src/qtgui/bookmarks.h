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
#ifndef BOOKMARKS_H
#define BOOKMARKS_H

#include <QtGlobal>
#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QString>
#include <QMap>
#include <QList>
#include <QVector>
#include <QStringList>
#include <QColor>
#include "receivers/vfo.h"

struct TagInfo
{
    using sptr = std::shared_ptr<TagInfo>;
    QString name;
    QColor color;
    bool active;

    static const QColor DefaultColor;
    static const QString strUntagged;

    TagInfo(QString name)
    {
        active=true;
        this->color=DefaultColor;
        this->name = name;
    }
    static sptr make(QString name = "")
    {
        return std::make_shared<TagInfo>(name);
    }
    bool operator<(const TagInfo &other) const
    {
        return name < other.name;
    }
};

class BookmarkInfo:public vfo_s
{
    public:
#if GNURADIO_VERSION < 0x030900
    typedef boost::shared_ptr<BookmarkInfo> sptr;
#else
    typedef std::shared_ptr<BookmarkInfo> sptr;
#endif
    static sptr make()
    {
        return sptr(new BookmarkInfo());
    }

    BookmarkInfo():vfo_s(),frequency(0)
    {
    }

/*    BookmarkInfo( qint64 frequency, QString name, qint64 bandwidth, QString modulation )
    {
        this->frequency = frequency;
        this->name = name;
        this->modulation = modulation;
        this->bandwidth = bandwidth;
    }
*/
    bool operator<(const BookmarkInfo &other) const
    {
        return frequency < other.frequency;
    }
    bool operator==(const BookmarkInfo &other) const
    {
        return frequency == other.frequency;
    }
    qint64 get_frequency() const {return frequency;}
    void set_frequency(qint64 newFrequency){frequency=newFrequency;}
    QString get_name() const {return name;}
    void set_name(QString newName){name=newName;}
    QString get_tags() const ;
    void set_tags(QString newTags);
/*
    void setTags(QString tagString);
    QString getTagString();
    bool hasTags(QString _tags);
    bool hasTags(QStringList _tags);
 */

    const QColor GetColor() const;
    bool IsActive() const;

    qint64  frequency;
    QString name;
    QList<TagInfo::sptr> tags;
};

class CommaSeparated
{
public:
    CommaSeparated(QChar quote='\"', QChar fieldDelimiter=';', QChar lineDelimiter='\n');
    ~CommaSeparated();
    bool open(const QString filename, bool write);
    void close();
    QStringList & getRowRef(){return m_Row;}
    bool write(const QStringList & row);
    bool read();
    bool read(QStringList & row);
    bool read(const QStringList & captions, QMap<QString,QString> & row);
private:
    QChar m_Quote;
    QChar m_FieldDelimiter;
    QChar m_LineDelimiter;
    QFile m_FD;
    QTextStream m_TS;
    QStringList m_Row;
};

class Bookmarks : public QObject
{
    Q_OBJECT
public:
    enum value_type
    {
        V_INT=0,
        V_DOUBLE,
        V_STRING,
        V_BOOLEAN
    };

    struct Def{
        value_type T;
        QString name;
        int /*BookmarksTableModel::EColumns*/ col;
        int ref;//TODO: replace with c_id
        std::function< bool (BookmarkInfo &, const QString &)> fromString;
        std::function< QString (BookmarkInfo &)> toString;
        std::function< int (const BookmarkInfo &, const BookmarkInfo &)> cmp;
    };

    // This is a Singleton Class now because you can not send qt-signals from static functions.
    static Bookmarks& Get();

    void add(BookmarkInfo& info);
    void remove(int index);
    void remove(const BookmarkInfo &info);
    bool load();
    bool save();
    int size() { return m_BookmarkList.size(); }
    BookmarkInfo& getBookmark(int i) { return m_BookmarkList[i]; }
    QList<BookmarkInfo> getBookmarksInRange(qint64 low, qint64 high, bool autoAdded = false);
    int find(const BookmarkInfo &info);
    //int lowerBound(qint64 low);
    //int upperBound(qint64 high);

    QList<TagInfo::sptr> getTagList() { return  QList<TagInfo::sptr>(m_TagList); }
    TagInfo::sptr findOrAddTag(QString tagName);
    int getTagIndex(QString tagName);
    bool removeTag(QString tagName);
    bool setTagChecked(QString tagName, bool bChecked);

    void setConfigDir(const QString&);

    const Def & def(const int i)
    {
        return m_idx_struct[i];
    }

    const Def & def(const QString & k)
    {
        return *m_name_struct[k];
    }

    int columnCount() const
    {
        return m_idx_struct.count();
    }

private:
    Bookmarks(); // Singleton Constructor is private.
    bool fromString(bool & to, const QString & from){to=(from=="true"); return true;}
    bool fromString(int & to, const QString & from){bool ok{false}; to=from.toInt(&ok); return ok;}
    bool fromString(Modulations::idx & to, const QString & from){to=Modulations::GetEnumForModulationString(from); return true;}
    bool fromString(long int & to, const QString & from){bool ok{false}; to=from.toLong(&ok); return ok;}
    bool fromString(long long int & to, const QString & from){bool ok{false}; to=from.toLongLong(&ok); return ok;}
    bool fromString(float & to, const QString & from){bool ok{false}; to=from.toFloat(&ok); return ok;}
    bool fromString(double & to, const QString & from){bool ok{false}; to=from.toDouble(&ok); return ok;}
    bool fromString(std::string & to, const QString & from){to=from.toStdString(); return true;}
    bool fromString(QString & to, const QString & from){to=from; return true;}
    QString toString(const bool & from){return QString(from?"true":"false");}
    QString toString(const int & from){return QString::number(from);}
    QString toString(const Modulations::idx & from){return Modulations::GetStringForModulationIndex(from);}
    QString toString(const long int & from){return QString::number(from);}
    QString toString(const long long int & from){return QString::number(from);}
    QString toString(const float & from){return QString::number(from);}
    QString toString(const double & from){return QString::number(from);}
    QString toString(const std::string & from){return QString::fromStdString(from);}
    QString toString(const QString & from){return from;}
    template< class A, typename B> std::function< bool (A & to, const QString & from)> genSetter(void (A::* psetter)(B))
    {
        return [=] (A & to, const QString & from) -> bool { B tmp; bool ok = fromString(tmp, from); if(ok) (to.*psetter)(tmp); return ok;};
    }
    template< class A, typename B> std::function< QString (A & from)> genGetter(B (A::* pgetter)() const)
    {
        return [=] (A & from) -> QString { return toString((from.*pgetter)());};
    }
    template< class A, typename B> std::function< int (const A &, const A &)> genCmp(B (A::* pgetter)() const)
    {
        return [=] (const A & a, const A & b) -> int
            {
                B va = (a.*pgetter)();
                B vb = (b.*pgetter)();
                return (va==vb)?0:((va>vb)?1:-1);
            };
    }
    template< class A, typename B> std::function< bool (A & to, const QString & from)> genSetterN(void (A::* psetter)(int, B), int n)
    {
        return [=] (A & to, const QString & from) -> bool { B tmp; bool ok = fromString(tmp, from); if(ok) (to.*psetter)(n, tmp); return ok;};
    }
    template< class A, typename B> std::function< QString (A & from)> genGetterN(B (A::* pgetter)(int) const, int n)
    {
        return [=] (A & from) -> QString { return toString((from.*pgetter)(n));};
    }
    template< class A, typename B> std::function< int (const A &, const A &)> genCmpN(B (A::* pgetter)(int) const, int n)
    {
        return [=] (const A & a, const A & b) -> int
            {
                B va = (a.*pgetter)(n);
                B vb = (b.*pgetter)(n);
                return (va==vb)?0:((va>vb)?1:-1);
            };
    }
    template< class A> std::function< bool (A & to, const QString & from)> genSetterCS(void (A::* psetter)(const std::string &))
    {
        return [=] (A & to, const QString & from) -> bool { std::string tmp; bool ok = fromString(tmp, from); if(ok) (to.*psetter)(tmp); return ok;};
    }
    template< class A> std::function< QString (A & from)> genGetterCS(const std::string & (A::* pgetter)() const)
    {
        return [=] (A & from) -> QString { return toString((from.*pgetter)());};
    }

    QList<BookmarkInfo>  m_BookmarkList;
    QList<TagInfo::sptr> m_TagList;
    QString              m_bookmarksFileOld;
    QString              m_bookmarksFile;
    QString              m_tagsFile;
    QVector<Def>         m_idx_struct;
    QVector<Def*>        m_col_struct;
    QMap<QString,Def*>   m_name_struct;

signals:
    void BookmarksChanged(void);
    void TagListChanged(void);
};

#endif // BOOKMARKS_H
