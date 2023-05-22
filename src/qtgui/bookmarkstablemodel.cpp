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
#include <functional>
#include <QFile>
#include <QStringList>
#include "bookmarks.h"
#include "bookmarkstablemodel.h"
#include "dockrxopt.h"


BookmarksTableModel::BookmarksTableModel(QObject *parent) :
    QAbstractTableModel(parent),
    m_sortCol(0),
    m_sortDir(Qt::AscendingOrder)
{
}

int BookmarksTableModel::rowCount ( const QModelIndex & /*parent*/ ) const
{
    return m_Bookmarks.size();
}
int BookmarksTableModel::columnCount ( const QModelIndex & /*parent*/ ) const
{
    return Bookmarks::Get().columnCount();
}

QVariant BookmarksTableModel::headerData ( int section, Qt::Orientation orientation, int role ) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        return Bookmarks::Get().def(section).name;
    }
    if(orientation == Qt::Vertical && role == Qt::DisplayRole)
    {
        return section;
    }
    return QVariant();
}

QVariant BookmarksTableModel::dataFromBookmark(BookmarkInfo &info, int index)
{
    return Bookmarks::Get().def(index).toString(info);
}

QVariant BookmarksTableModel::data ( const QModelIndex & index, int role ) const
{
    BookmarkInfo &info = *getBookmarkAtRow(index.row());

    if(role==Qt::BackgroundRole)
    {
        QColor bg(info.GetColor());
        bg.setAlpha(0x60);
        return bg;
    }

    else if(role == Qt::DisplayRole || role==Qt::EditRole)
    {
        return BookmarksTableModel::dataFromBookmark(info, index.column());
    }
    return QVariant();
}

bool BookmarksTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role==Qt::EditRole)
    {
        BookmarkInfo &info = *getBookmarkAtRow(index.row());
        Bookmarks::Get().def(index.column()).fromString(info, value.toString());
        emit dataChanged(index, index);
        return true; // return true means success
    }
    return false;
}

Qt::ItemFlags BookmarksTableModel::flags ( const QModelIndex& index ) const
{
    Qt::ItemFlags flags = Qt::ItemFlags();

    switch(index.column())
    {
    case COL_TAGS:
        flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        break;
    default:
        flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
        break;
    }
    return flags;
}

void BookmarksTableModel::update()
{
    m_Bookmarks.clear();
    for(int iBookmark=0; iBookmark<Bookmarks::Get().size(); iBookmark++)
    {
        BookmarkInfo& info = Bookmarks::Get().getBookmark(iBookmark);

        for(int iTag=0; iTag<info.tags.size(); ++iTag)
        {
            TagInfo::sptr tag = info.tags[iTag];
            if(tag->active)
            {
                m_Bookmarks.append(iBookmark);
                break;
            }
        }
    }
    sort(m_sortCol, m_sortDir);
}

BookmarkInfo *BookmarksTableModel::getBookmarkAtRow(int row) const
{
    return & Bookmarks::Get().getBookmark(m_Bookmarks[row]);
}

int BookmarksTableModel::GetBookmarksIndexForRow(int iRow)
{
  return m_Bookmarks[iRow];
}

int BookmarksTableModel::GetRowForBookmarkIndex(int index)
{
    return m_Bookmarks.indexOf(index);
}

bool BookmarksTableModel::bmCompare(const int a, const int b, int column, int order)
{
    Bookmarks & bm(Bookmarks::Get());
    if (order)
        return bm.def(column).cmp(bm.getBookmark(a), bm.getBookmark(b)) >= 0;
    else
        return bm.def(column).cmp(bm.getBookmark(a), bm.getBookmark(b)) < 0;
}

void BookmarksTableModel::sort(int column, Qt::SortOrder order)
{
    if (column < 0)
        return;
    m_sortCol = column;
    m_sortDir = order;
    std::stable_sort(m_Bookmarks.begin(), m_Bookmarks.end(),
                     std::bind(bmCompare, std::placeholders::_1,
                               std::placeholders::_2, column, order));
    emit layoutChanged();
}
