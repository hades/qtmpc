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

#include "dirviewitem.h"
#include "dirviewproxymodel.h"

DirViewProxyModel::DirViewProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{

}

bool DirViewProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
	const DirViewItem * const leftItem = static_cast<DirViewItem *>(left.internalPointer());
	const DirViewItem * const rightItem = static_cast<DirViewItem *>(right.internalPointer());

	if (leftItem->type() == DirViewItem::Type_Dir &&
	    rightItem->type() == DirViewItem::Type_File)
		return true;

	if (leftItem->type() == DirViewItem::Type_File &&
		rightItem->type() == DirViewItem::Type_Dir)
		return false;

	return QSortFilterProxyModel::lessThan(left, right);
}
