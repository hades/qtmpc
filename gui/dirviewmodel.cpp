/*
 * Copyright (c) 2008 Sander Knopper (sander AT knopper DOT tk) and
 *                    Roeland Douma (roeland AT rullzer DOT com)
 *
 * This file is part of QtMPC.
 *
 * QtMPC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * QtMPC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QtMPC.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QCommonStyle>
#include <QModelIndex>
#include <QString>
#include <QVariant>

#include "dirviewmodel.h"
#include "dirviewitem.h"

dirViewModel::dirViewModel(QObject *parent)
		: QAbstractItemModel(parent),
		  rootItem(new DirViewItemRoot(""))
{
}

dirViewModel::~dirViewModel()
{
	delete rootItem;
}

QModelIndex dirViewModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	const DirViewItem * parentItem;

	if(!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<DirViewItem *>(parent.internalPointer());

	DirViewItem * const childItem = parentItem->child(row);
	if(childItem)
		return createIndex(row, column, childItem);

	return QModelIndex();
}

QModelIndex dirViewModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	const DirViewItem * const childItem = static_cast<DirViewItem *>(index.internalPointer());
	DirViewItem * const parentItem = childItem->parent();

	if (parentItem == rootItem)
		return QModelIndex();

	return createIndex(parentItem->row(), 0, parentItem);
}

QVariant dirViewModel::headerData(int /*section*/, Qt::Orientation /*orientation*/, int /*role*/) const
{
	return QVariant();
}

int dirViewModel::rowCount(const QModelIndex &parent) const
{
	if(parent.column() > 0)
		return 0;

	const DirViewItem *parentItem;

	if(!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<DirViewItem *>(parent.internalPointer());

	return parentItem->childCount();
}

int dirViewModel::columnCount(const QModelIndex &parent) const
{
	if(parent.isValid())
		return static_cast<DirViewItem *>(parent.internalPointer())->columnCount();
	else
		return rootItem->columnCount();
}

QVariant dirViewModel::data(const QModelIndex &index, int role) const
{
	if(!index.isValid())
		return QVariant();

	if(role != Qt::DisplayRole && role != Qt::DecorationRole)
		return QVariant();

	DirViewItem *item = static_cast<DirViewItem *>(index.internalPointer());

	if(role == Qt::DecorationRole) {
		QCommonStyle style;
		if(item->type() == DirViewItem::Type_Dir) {
			return style.standardIcon(QStyle::SP_DirIcon);
		} else if(item->type() == DirViewItem::Type_File) {
			return style.standardIcon(QStyle::SP_FileIcon);
		}
	} else {
		return item->data(index.column());
	}

	return QVariant();
}

void dirViewModel::dirViewUpdated(DirViewItemRoot *newroot)
{
	const DirViewItemRoot *oldRoot = rootItem;
	rootItem = newroot;
	delete oldRoot;

	reset();
}
