// --------------------------------------------------------------------------
// File: listwidget.h
// Copyright (C) 2006-2008 ◆e5bW6vDOJ.
// Copyright (C) 2015 nel
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// --------------------------------------------------------------------------
#ifndef LISTWIDGETH
#define LISTWIDGETH

#include "peercast.h"

#include <QMetaType>
#include <QListWidgetItem>
#include <QStyledItemDelegate>

typedef struct
{
    bool available;
    bool relay;
    bool firewalled;
    int relays;
    int vp_ver;
    int totalRelays, totalListeners;
} tServentInfo;

QRgb get_relay_color(tServentInfo &info);

class ChannelConnectionListWidget : public QListWidget
{
    Q_OBJECT

public:
    ChannelConnectionListWidget(QWidget *parent=0):QListWidget(parent) { this->blockFocusIn=false; }
    int selectedCurrentRow() const;

protected:
    bool event(QEvent *e);
    void mousePressEvent(QMouseEvent *);

    QPoint mousePressPos;

private:
    bool blockFocusIn;
};

class ChannelListWidget : public ChannelConnectionListWidget
{
    Q_OBJECT

public:
    ChannelListWidget(QWidget *parent=0);

protected:
    void mouseReleaseEvent(QMouseEvent *);

    QMenu *menu;
};

class ChannelListItemData
{
public:
    GnuID        id;
    tServentInfo info;
    bool         receive;
    bool         tracker;
    bool         broadcast;
    QString      name;
    QString      status;

    ChannelListItemData();
    ChannelListItemData(Channel *ch);
};
Q_DECLARE_METATYPE(ChannelListItemData)

class ChannelListItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    ChannelListItemDelegate(QListWidget *parent):QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class ConnectionListItemData
{
public:
    int          servent_id;
    tServentInfo info;
    bool         relaying;
    QString      text;

    ConnectionListItemData();
    ConnectionListItemData(Servent *sv, tServentInfo &si);

protected:
    QString timeToStr(unsigned sec);
};
Q_DECLARE_METATYPE(ConnectionListItemData)

class ConnectionListItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    ConnectionListItemDelegate(QListWidget *parent):QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // LISTWIDGETH

