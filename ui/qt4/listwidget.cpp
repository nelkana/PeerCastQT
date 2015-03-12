// --------------------------------------------------------------------------
// File: listwidget.cpp
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
#include "listwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QMenu>
#include <QDebug>

QRgb get_relay_color(tServentInfo &info)
{
    if( info.available ) {
        if( info.firewalled ) {
            if( info.relays )
                return qRgb(255,128,0);
            else
                return qRgb(255,0,0);
        }
        else {
            if( info.relay ) {
                return qRgb(0,255,0);
            }
            else {
                if( info.relays )
                    return qRgb(0,0,255);
                else
                    return qRgb(255,0,255);
            }
        }
    }
    else {
        return qRgb(0,0,0);
    }
}

// --------------------------------------------------------------------------
int ChannelConnectionListWidget::selectedCurrentRow() const
{
    int row = currentRow();

    if( row >= 0 ) {
        if( !currentItem()->isSelected() )
            row = -1;
    }

    return row;
}

bool ChannelConnectionListWidget::event(QEvent *e)
{
    // ウィンドウアクティブ時の不必要なカレント矩形線を表示しないようにする為の対応。
    // ウィンドウがアクティブになった時、フォーカスのあるQListWigetのカレント行が
    // 負の値を設定してるのに0に設定されるので、focusInイベントをブロックして回避する。
    if( e->type() == QEvent::WindowActivate ) {
        if( this == window()->focusWidget() )
            this->blockFocusIn = true;
    }
    else if( e->type() == QEvent::FocusIn ) {
        if( this->blockFocusIn ) {
            this->blockFocusIn = false;
            return true;
        }
    }

    return QListWidget::event(e);
}

void ChannelConnectionListWidget::mousePressEvent(QMouseEvent *e)
{
    this->mousePressPos = e->pos();

    if( !itemAt(e->pos()) )
        clearSelection();

    QListWidget::mousePressEvent(e);
}

// --------------------------------------------------------------------------
ChannelListWidget::ChannelListWidget(QWidget *parent) : ChannelConnectionListWidget(parent)
{
    this->menu = new QMenu(this);
}

void ChannelListWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if( e->button() == Qt::RightButton )
    {
        QListWidgetItem *item = itemAt(this->mousePressPos);

        if( visualItemRect(item).contains(e->pos()) ) {
            ChannelListItemData data = qvariant_cast<ChannelListItemData>(item->data(Qt::UserRole));

            chanMgr->lock.on();

            Channel *c = chanMgr->findChannelByID(data.id);
            if( c ) {
                QString str;

                menu->clear();
                menu->addAction(QString::fromUtf8(c->info.name.data));

                str = QString::fromUtf8(c->info.genre.data);
                if(!c->info.genre.isEmpty() && !c->info.genre.isEmpty())
                    str += " - ";
                str += QString::fromUtf8(c->info.desc.data);
                if(str != "") {
                    str = "[" + str + "]";
                    menu->addAction(str);
                }

                str = QString::fromUtf8(c->info.track.artist.data);
                if(!c->info.track.artist.isEmpty() && !c->info.track.title.isEmpty())
                    str += " - ";
                str += QString::fromUtf8(c->info.track.title.data);
                if(str != "") {
                    str = "Playing: " + str;
                    menu->addAction(str);
                }

                if(!c->info.comment.isEmpty()) {
                    str = "\"";
                    str += QString::fromUtf8(c->info.comment.data);
                    str +="\"";
                    menu->addAction(str);
                }

                chanMgr->lock.off();

                menu->addSeparator();
                QAction *act = menu->addAction(tr("Deselect"));
                connect(act, SIGNAL(triggered()), this, SLOT(clearSelection()));

                menu->popup(e->globalPos());
            }
            else {
                chanMgr->lock.off();
            }
        }
    }

    ChannelConnectionListWidget::mouseReleaseEvent(e);
}

// --------------------------------------------------------------------------
ChannelListItemData::ChannelListItemData()
{
    this->id.clear();
    this->info.available = false;
    this->info.relay = false;
    this->info.firewalled = false;
    this->info.relays = 0;
    this->info.vp_ver = 0;
    this->info.totalRelays = 0;
    this->info.totalListeners = 0;
    this->receive = false;
    this->tracker = false;
    this->broadcast = false;
}

ChannelListItemData::ChannelListItemData(Channel *ch)
{
    this->info.available = true;
    this->info.relay = ch->chDisp.relay;
    this->info.relays = ch->localRelays();
    this->info.firewalled = (servMgr->getFirewall() == ServMgr::FW_ON);

    this->receive = (ch->status==Channel::S_RECEIVING || ch->status==Channel::S_BROADCASTING);
    this->broadcast = (ch->status == Channel::S_BROADCASTING);
    this->tracker = ch->sourceHost.tracker;

    this->status.sprintf(" - %d kbps - %s - %d/%d - [%d/%d] - %s",
        ch->info.bitrate,
        ch->getStatusStr(),
        ch->totalListeners(),
        ch->totalRelays(),
        ch->localListeners(),
        ch->localRelays(),
        ch->stayConnected ? "YES" : "NO"
        );
    this->name = QString::fromUtf8(ch->info.name.data);

    this->id = ch->info.id;
}

// --------------------------------------------------------------------------
void ChannelListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
    const QModelIndex &index) const
{
    QPen old_pen = painter->pen();
    QBrush old_brush = painter->brush();

    QStyledItemDelegate::paint(painter, option, index);

    ChannelListItemData data = qvariant_cast<ChannelListItemData>(index.data(Qt::UserRole));

    if( data.receive ) {
        QListWidget *listWidget = (QListWidget *)parent();

        if( data.broadcast ) {
            if( listWidget->selectionModel()->isSelected(index) )
                painter->setPen(qRgb(255,255,0));
            else
                painter->setPen(qRgb(128,128,0));
        }
        else if( data.tracker ) {
            if( listWidget->selectionModel()->isSelected(index) )
                painter->setPen(qRgb(0,255,0));
            else
                painter->setPen(qRgb(0,160,0));
        }
    }

    QFontMetrics fm = option.fontMetrics;
    int color_area = fm.height();

    QRect rect = option.rect;
    rect.setX(rect.right()+1 - fm.width(data.status) - 4);
    painter->drawText(rect, Qt::AlignVCenter, data.status);
    rect.setRight(rect.x() - 1);
    rect.setX(2 + color_area + 2);
    painter->drawText(rect, Qt::AlignVCenter, data.name);

    QBrush brush;
    if( data.receive ) {
        QRgb color = get_relay_color(data.info);

        brush.setStyle(Qt::SolidPattern);
        brush.setColor(color);
        painter->setPen(color);
    }
    else {
        brush.setStyle(Qt::NoBrush);
    }

    painter->setBrush(brush);
    painter->drawRect(2 + 1, rect.y()+1, color_area-3, color_area-3);

    painter->setPen(old_pen);
    painter->setBrush(old_brush);
}

// --------------------------------------------------------------------------
ConnectionListItemData::ConnectionListItemData()
{
    this->servent_id = 0;
    this->info.available = false;
    this->info.relay = false;
    this->info.firewalled = false;
    this->info.relays = 0;
    this->info.vp_ver = 0;
    this->info.totalRelays = 0;
    this->info.totalListeners = 0;
    this->relaying = false;
}

ConnectionListItemData::ConnectionListItemData(Servent *sv, tServentInfo &si)
{
    char vp_ver[32], host_name[32];

    this->servent_id = sv->servent_id;
    this->info = si;

    if(this->info.vp_ver)
        sprintf(vp_ver, "(VP%04d)", this->info.vp_ver);
    else
        strcpy(vp_ver, "");

    sv->getHost().toStr(host_name);

    QString time;
    unsigned sec = (sv->lastConnect) ? (sys->getTime() - sv->lastConnect) : 0;
    time = timeToStr(sec);

    this->relaying = (sv->type == Servent::T_RELAY);

    if(sv->type == Servent::T_RELAY)
    {
        if(sv->status == Servent::S_CONNECTED)
        {
            this->text.sprintf("RELAYING - %s - %d/%d - %s - %s%s",
                time.toUtf8().constData(),
                this->info.totalListeners,
                this->info.totalRelays,
                host_name,
                sv->agent.cstr(),
                vp_ver
                );
        }
        else
        {
            this->text.sprintf("%s-%s - %s - %d/%d - %s - %s%s",
                sv->getTypeStr(),
                sv->getStatusStr(),
                time.toUtf8().constData(),
                this->info.totalListeners,
                this->info.totalRelays,
                host_name,
                sv->agent.cstr(),
                vp_ver
                );
        }
    }
    else if(sv->type == Servent::T_DIRECT)
    {
        this->text.sprintf("%s-%s - %s - %s - %s",
            sv->getTypeStr(),
            sv->getStatusStr(),
            time.toUtf8().constData(),
            host_name,
            sv->agent.cstr()
            );
    }
    else
    {
        if(sv->status == Servent::S_CONNECTED)
        {
            this->text.sprintf("%s-%s - %s - %s - %s",
                sv->getTypeStr(),
                sv->getStatusStr(),
                time.toUtf8().constData(),
                host_name,
                sv->agent.cstr()
                );
        }
        else
        {
            this->text.sprintf("%s-%s - %s - %s",
                sv->getTypeStr(),
                sv->getStatusStr(),
                time.toUtf8().constData(),
                host_name
                );
        }
    }
}

QString ConnectionListItemData::timeToStr(unsigned sec)
{
    unsigned h, m, s;
    h = sec / 3600;
    sec %= 3600;
    m = sec / 60;
    s = sec % 60;

    QString time;
    if( h )
        time.sprintf("%02d:%02d:%02d", h, m, s);
    else
        time.sprintf("%02d:%02d", m, s);

    return time;
}

// --------------------------------------------------------------------------
void ConnectionListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
    const QModelIndex &index) const
{
    QPen old_pen = painter->pen();
    QBrush old_brush = painter->brush();

    QStyledItemDelegate::paint(painter, option, index);

    ConnectionListItemData data = qvariant_cast<ConnectionListItemData>(index.data(Qt::UserRole));

    int color_area = option.fontMetrics.height();

    QRect rect = option.rect;
    rect.setX(rect.x() + 2 + color_area + 2);
    painter->drawText(rect, Qt::AlignVCenter, data.text);

    QBrush brush;
    if( data.relaying ) {
        QRgb color = get_relay_color(data.info);

        brush.setStyle(Qt::SolidPattern);
        brush.setColor(color);
        painter->setPen(color);
    }
    else {
        brush.setStyle(Qt::NoBrush);
    }

    painter->setBrush(brush);
    painter->drawRect(2 + 1, rect.y()+1, color_area-3, color_area-3);

    painter->setPen(old_pen);
    painter->setBrush(old_brush);
}

