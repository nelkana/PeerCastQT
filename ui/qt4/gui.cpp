// --------------------------------------------------------------------------
// File: gui.cpp
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
#include "servmgr.h"

#include <QApplication>
#include <QSettings>
#include <QScrollBar>
#include <QTextFrame>
#include <QCloseEvent>

#include "main.h"
#include "gui.h"
#include "listwidget.h"

#define MAX_LOG_NUM 1024

MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    setupUi(this);

    listWidgetChannel->setItemDelegate(new ChannelListItemDelegate(listWidgetChannel));
    listWidgetChannel->setFocus();
    listWidgetConnection->setItemDelegate(new ConnectionListItemDelegate(listWidgetConnection));
    textEditLog->document()->setMaximumBlockCount(MAX_LOG_NUM);
    initTextEditLogMargin();

    iniFileName = qApp->applicationDirPath() + "/peercast_qt.ini";

    {
        QSettings ini(iniFileName, QSettings::IniFormat);
        QRect rect;

        restoreGeometry(ini.value("geometry").toByteArray());
        splitter->restoreState(ini.value("splitter").toByteArray());
    }

    remainPopup = -1;

#ifdef Q_OS_WIN32
    ico.addFile(":/peercast.ico");
#else
    ico.addFile(":/peercast.xpm");
#endif // Q_OS_WIN32
    setWindowIcon(ico);

    timerUpdate = new QTimer(this);
    timerUpdate->start(1000);

    timerLogUpdate = new QTimer(this);
    timerLogUpdate->start(100);

    connect(timerLogUpdate, SIGNAL(timeout()), this, SLOT(timerLogUpdate_timeout()));
    connect(timerUpdate, SIGNAL(timeout()), this, SLOT(timerUpdate_timeout()));

    connect(pushButtonEnabled, SIGNAL(toggled(bool)), this, SLOT(pushButtonEnabled_toggled(bool)));
    connect(pushButtonStop, SIGNAL(toggled(bool)), this, SLOT(pushButtonStop_toggled(bool)));
    connect(pushButtonDebug, SIGNAL(toggled(bool)), this, SLOT(pushButtonDebug_toggled(bool)));
    connect(pushButtonError, SIGNAL(toggled(bool)), this, SLOT(pushButtonError_toggled(bool)));
    connect(pushButtonNetwork, SIGNAL(toggled(bool)), this, SLOT(pushButtonNetwork_toggled(bool)));
    connect(pushButtonChannel, SIGNAL(toggled(bool)), this, SLOT(pushButtonChannel_toggled(bool)));
    connect(pushButtonClear, SIGNAL(clicked()), this, SLOT(pushButtonClear_clicked()));
    connect(pushButtonBump, SIGNAL(clicked()), this, SLOT(pushButtonBump_clicked()));
    connect(pushButtonDisconnect, SIGNAL(clicked()), this, SLOT(pushButtonDisconnect_clicked()));
    connect(pushButtonKeep, SIGNAL(clicked()), this, SLOT(pushButtonKeep_clicked()));
    connect(pushButtonPlay, SIGNAL(clicked()), this, SLOT(pushButtonPlay_clicked()));
    connect(pushButtonDisconnectConn, SIGNAL(clicked()), this, SLOT(pushButtonDisconnectConn_clicked()));
    connect(listWidgetChannel, SIGNAL(itemSelectionChanged()), this, SLOT(timerUpdate_timeout()));
    connect(listWidgetConnection, SIGNAL(itemSelectionChanged()), this, SLOT(timerUpdate_timeout()));

    actionExit = new QAction(this);
    connect(actionExit, SIGNAL(triggered()), qApp, SLOT(quit()));
    actionShow = new QAction(this);
    connect(actionShow, SIGNAL(triggered()), this, SLOT(showGui()));

    actionTracker = new QAction(this);
    actionTracker->setCheckable(true);
    connect(actionTracker, SIGNAL(triggered(bool)), this, SLOT(actionTracker_triggered(bool)));
    actionTrack = new QAction(this);
    actionTrack->setCheckable(true);
    connect(actionTrack, SIGNAL(triggered(bool)), this, SLOT(actionTrack_triggered(bool)));
    actionMsgPeerCast = new QAction(this);
    actionMsgPeerCast->setCheckable(true);
    connect(actionMsgPeerCast, SIGNAL(triggered(bool)), this, SLOT(actionMsgPeerCast_triggered(bool)));

#ifndef _APPLE
    trayMenuPopup = new QMenu(this);
    trayMenuPopup->addAction(actionMsgPeerCast);
    trayMenuPopup->addAction(actionTracker);
    trayMenuPopup->addAction(actionTrack);

    trayMenu = new QMenu(this);
    trayMenu->addAction(actionShow);
    trayMenu->addSeparator();
    trayMenu->addMenu(trayMenuPopup);
    trayMenu->addSeparator();
    trayMenu->addAction(actionExit);

    tray = new QSystemTrayIcon(this);
    tray->setIcon(ico);
    tray->setContextMenu(trayMenu);
    tray->show();

    connect(tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(tray_activated(QSystemTrayIcon::ActivationReason)));
    connect(tray, SIGNAL(messageClicked()), this, SLOT(tray_messageClicked()));
#endif

    reloadGui();
    pushButtonEnabled->setChecked(servMgr->autoServe);

    languageChange();
}

MainWindow::~MainWindow()
{
    {
        QSettings ini(iniFileName, QSettings::IniFormat);

        ini.setValue("geometry", saveGeometry());
        ini.setValue("splitter", splitter->saveState());
    }
}

void MainWindow::languageChange()
{
#ifndef _APPLE
    tray->setToolTip(tr("PeerCast"));
    trayMenuPopup->setTitle(tr("Popup message"));
#endif

    actionTracker->setText(tr("Broadcasters"));
    actionTrack->setText(tr("Track info"));
    actionMsgPeerCast->setText(tr("PeerCast"));

    actionExit->setText(tr("Exit"));
    actionShow->setText(tr("Show GUI"));
}

void MainWindow::reloadGui()
{
    char sztemp[256];

    sprintf(sztemp, "%d", (int)servMgr->serverHost.port);
    lineEditPort->setText(sztemp);
    lineEditPassword->setText(servMgr->password);
    sprintf(sztemp, "%d", (int)servMgr->maxRelays);
    lineEditMaxRelays->setText(sztemp);

    pushButtonDebug->setChecked(servMgr->showLog&(1<<LogBuffer::T_DEBUG));
    pushButtonError->setChecked(servMgr->showLog&(1<<LogBuffer::T_ERROR));
    pushButtonNetwork->setChecked(servMgr->showLog&(1<<LogBuffer::T_NETWORK));
    pushButtonChannel->setChecked(servMgr->showLog&(1<<LogBuffer::T_CHANNEL));
    pushButtonStop->setChecked(servMgr->pauseLog);

    int mask = peercastInst->getNotifyMask();

    actionTracker->setChecked(mask & ServMgr::NT_BROADCASTERS);
    actionTrack->setChecked(mask & ServMgr::NT_TRACKINFO);
    actionMsgPeerCast->setChecked(mask & ServMgr::NT_PEERCAST);
}

void MainWindow::showGui()
{
#ifdef Q_OS_LINUX
    setVisible(true);
#else
    showNormal();
#endif // Q_OS_LINUX
    activateWindow();
}

void MainWindow::showHideGui()
{
    if( isMinimized() || !isVisible() ) {
        setVisible(true);
        activateWindow();
    }
    else {
#ifdef Q_OS_LINUX
        if( isActiveWindow() )
            setVisible(false);
        else
            activateWindow();
#else
        setVisible(false);
#endif // Q_OS_LINUX
    }
}

#define NOTIFY_TIMEOUT 8

void MainWindow::timerLogUpdate_timeout()    // 100ms
{
    if(remainPopup < 0)
    {
        if(!g_qNotify.empty())
        {
            tNotifyInfo info = g_qNotify.front();
            g_qNotify.pop();
#ifndef _APPLE
            tray->showMessage(info.name, info.msg, QSystemTrayIcon::NoIcon, NOTIFY_TIMEOUT*1000);
#endif
            remainPopup = NOTIFY_TIMEOUT*10;
        }
    }
    else
    {
        remainPopup--;
    }

    while(!g_qLog.empty())
    {
        QString str = g_qLog.front();
        g_qLog.pop();

        textEditLog->append(str);
    }

    if(g_bChangeSettings)
    {
        g_bChangeSettings = false;
        reloadGui();
    }
}

void MainWindow::timerUpdate_timeout()   // 1000ms
{
    GnuID sel_id;

    sel_id.clear();

    {
        bool block = listWidgetChannel->blockSignals(true);

        int n, y, count = 0;
        Channel *c;

        y = listWidgetChannel->verticalScrollBar()->value();
        n = listWidgetChannel->selectedCurrentRow();
        listWidgetChannel->clear();

        chanMgr->lock.on();

        c = chanMgr->channel;
        while(c)
        {
            if(n == count)
                sel_id = c->info.id;

            QListWidgetItem *item = new QListWidgetItem(listWidgetChannel);
            item->setData(Qt::UserRole, QVariant::fromValue(ChannelListItemData(c)));
            listWidgetChannel->addItem(item);

            c = c->next;
            count++;
        }

        chanMgr->lock.off();

        listWidgetChannel->setCurrentRow(n);
        listWidgetChannel->verticalScrollBar()->setValue(y);

        listWidgetChannel->blockSignals(block);
    }

    {
        bool block = listWidgetConnection->blockSignals(true);

        int n, y;
        Servent *s;

        y = listWidgetConnection->verticalScrollBar()->value();
        n = listWidgetConnection->selectedCurrentRow();
        listWidgetConnection->clear();

        servMgr->lock.on();

        s = servMgr->servents;
        while(s)
        {
            tServentInfo info;

            if(s->type == Servent::T_NONE)
            {
                s = s->next;
                continue;
            }

            if(sel_id.isSet() && !sel_id.isSame(s->chanID))
            {
                s = s->next;
                continue;
            }

            {
                ChanHitList *chl;

                info.available = false;
                info.totalRelays = 0;
                info.totalListeners = 0;
                info.vp_ver = 0;

                chanMgr->hitlistlock.on();

                chl = chanMgr->findHitListByID(s->chanID);
                if(chl)
                {
                    ChanHit *hit = chl->hit;
                    while(hit)
                    {
                        if(hit->servent_id == s->servent_id)
                        {
                            if((hit->numHops == 1) && (hit->host.ip == s->getHost().ip))
                            {
                                info.available = true;
                                info.relay = hit->relay;
                                info.firewalled = hit->firewalled;
                                info.relays = hit->numRelays;
                                info.vp_ver = hit->version_vp;
                            }

                            info.totalRelays += hit->numRelays;
                            info.totalListeners += hit->numListeners;
                        }

                        hit = hit->next;
                    }
                }

                chanMgr->hitlistlock.off();
            }

            QListWidgetItem *item = new QListWidgetItem(listWidgetConnection);
            item->setData(Qt::UserRole, QVariant::fromValue(ConnectionListItemData(s, info)));
            listWidgetConnection->addItem(item);

            s = s->next;
        }

        servMgr->lock.off();

        listWidgetConnection->setCurrentRow(n);
        listWidgetConnection->verticalScrollBar()->setValue(y);

        listWidgetConnection->blockSignals(block);
    }
}

void MainWindow::pushButtonBump_clicked()
{
    int n;
    QListWidgetItem *item;

    n = listWidgetChannel->selectedCurrentRow();
    item = listWidgetChannel->item(n);
    if(item)
    {
        ChannelListItemData data = qvariant_cast<ChannelListItemData>(item->data(Qt::UserRole));
        chanMgr->lock.on();

        Channel *c = chanMgr->findChannelByID(data.id);
        if(c)
        {
            c->bump = true;
        }

        chanMgr->lock.off();
    }
}

void MainWindow::pushButtonDisconnect_clicked()
{
    int n;
    QListWidgetItem *item;

    n = listWidgetChannel->selectedCurrentRow();
    item = listWidgetChannel->item(n);
    if(item)
    {
        ChannelListItemData data = qvariant_cast<ChannelListItemData>(item->data(Qt::UserRole));
        chanMgr->lock.on();

        Channel *c = chanMgr->findChannelByID(data.id);
        if(c)
        {
            c->thread.active = false;
            c->thread.finish = true;
        }

        chanMgr->lock.off();
    }
}

void MainWindow::pushButtonKeep_clicked()
{
    int n;
    QListWidgetItem *item;

    n = listWidgetChannel->selectedCurrentRow();
    item = listWidgetChannel->item(n);
    if(item)
    {
        ChannelListItemData data = qvariant_cast<ChannelListItemData>(item->data(Qt::UserRole));
        chanMgr->lock.on();

        Channel *c = chanMgr->findChannelByID(data.id);
        if(c)
        {
            c->stayConnected = !c->stayConnected;
        }

        chanMgr->lock.off();
    }
}

void MainWindow::pushButtonPlay_clicked()
{
    int n;
    QListWidgetItem *item;

    n = listWidgetChannel->selectedCurrentRow();
    item = listWidgetChannel->item(n);
    if(item)
    {
        ChannelListItemData data = qvariant_cast<ChannelListItemData>(item->data(Qt::UserRole));
        chanMgr->lock.on();

        Channel *c = chanMgr->findChannelByID(data.id);
        if(c)
        {
            chanMgr->playChannel(c->info);
        }

        chanMgr->lock.off();
    }
}

void MainWindow::pushButtonDisconnectConn_clicked()
{
    int n;
    QListWidgetItem *item;

    n = listWidgetConnection->selectedCurrentRow();
    item = listWidgetConnection->item(n);
    if(item)
    {
        ConnectionListItemData data = qvariant_cast<ConnectionListItemData>(item->data(Qt::UserRole));

        servMgr->lock.on();

        Servent *s = servMgr->findServentByServentID(data.servent_id);
        if(s)
        {
            s->thread.active = false;
        }

        servMgr->lock.off();
    }
}

void MainWindow::pushButtonEnabled_toggled(bool state)
{
    lineEditPort->setEnabled(state == 0);
    lineEditPassword->setEnabled(state == 0);
    lineEditMaxRelays->setEnabled(state == 0);

    if(state != 0)
    {
        if(!servMgr->autoServe)
        {
            QString str;
            unsigned short temp;
            bool success = false;

            str = lineEditPassword->text();;
            strcpy(servMgr->password, str.toUtf8().data());

            str = lineEditPort->text();
            temp = str.toUShort(&success);
            if(success)
                servMgr->serverHost.port = temp;

            str = lineEditMaxRelays->text();
            temp = str.toUShort(&success);
            if(success)
                servMgr->setMaxRelays(temp);

            servMgr->autoServe = true;

            peercastInst->saveSettings();
        }
    }
    else
    {
        servMgr->autoServe = false;
    }
}

void MainWindow::pushButtonStop_toggled(bool state)
{
    servMgr->pauseLog = state != 0;
}

void MainWindow::pushButtonDebug_toggled(bool state)
{
    servMgr->showLog = state != 0 ? servMgr->showLog|(1<<LogBuffer::T_DEBUG) : servMgr->showLog&~(1<<LogBuffer::T_DEBUG);
}

void MainWindow::pushButtonError_toggled(bool state)
{
    servMgr->showLog = state != 0 ? servMgr->showLog|(1<<LogBuffer::T_ERROR) : servMgr->showLog&~(1<<LogBuffer::T_ERROR);
}

void MainWindow::pushButtonNetwork_toggled(bool state)
{
    servMgr->showLog = state != 0 ? servMgr->showLog|(1<<LogBuffer::T_NETWORK) : servMgr->showLog&~(1<<LogBuffer::T_NETWORK);
}

void MainWindow::pushButtonChannel_toggled(bool state)
{
    servMgr->showLog = state != 0 ? servMgr->showLog|(1<<LogBuffer::T_CHANNEL) : servMgr->showLog&~(1<<LogBuffer::T_CHANNEL);
}

void MainWindow::pushButtonClear_clicked()
{
    sys->logBuf->clear();
    textEditLog->clear();
    initTextEditLogMargin();
}

#ifndef _APPLE

void MainWindow::tray_activated(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason)
    {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        showHideGui();
        break;

//  case QSystemTrayIcon::MiddleClick:
//  case QSystemTrayIcon::Context:
//  case QSystemTrayIcon::Unknown:
    }
}

void MainWindow::tray_messageClicked()
{
    remainPopup = 0;
}

#endif

void MainWindow::setNotifyMask(ServMgr::NOTIFY_TYPE nt)
{
    int mask = peercastInst->getNotifyMask();
    mask ^= nt;
    peercastInst->setNotifyMask(mask);
    peercastInst->saveSettings();
}

void MainWindow::actionTracker_triggered(bool checked)
{
    setNotifyMask(ServMgr::NT_BROADCASTERS);
}

void MainWindow::actionTrack_triggered(bool checked)
{
    setNotifyMask(ServMgr::NT_TRACKINFO);
}

void MainWindow::actionMsgPeerCast_triggered(bool checked)
{
    setNotifyMask(ServMgr::NT_PEERCAST);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

void MainWindow::initTextEditLogMargin()
{
    QTextFrame *tf = textEditLog->document()->rootFrame();
    QTextFrameFormat tff = tf->frameFormat();
    tff.setTopMargin(1);
    tff.setBottomMargin(1);
    tff.setLeftMargin(2);
    tff.setRightMargin(2);
    tf->setFrameFormat(tff);
}

