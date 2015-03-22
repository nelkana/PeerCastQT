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

    this->iniFileName = qApp->applicationDirPath() + "/peercast_qt.ini";

    this->listWidgetChannel->setItemDelegate(new ChannelListItemDelegate(this->listWidgetChannel));
    this->listWidgetChannel->setFocus();
    this->listWidgetConnection->setItemDelegate(new ConnectionListItemDelegate(this->listWidgetConnection));
    this->textEditLog->document()->setMaximumBlockCount(MAX_LOG_NUM);
    initTextEditLogMargin();

    this->remainPopup = -1;

#ifdef Q_OS_WIN32
    this->ico.addFile(":/peercast.ico");
#else
    this->ico.addFile(":/peercast.xpm");
#endif // Q_OS_WIN32
    setWindowIcon(this->ico);

    this->timerUpdate = new QTimer(this);
    this->timerUpdate->start(1000);

    this->timerLogUpdate = new QTimer(this);
    this->timerLogUpdate->start(100);

    connect(this->timerLogUpdate, SIGNAL(timeout()), this, SLOT(timerLogUpdate_timeout()));
    connect(this->timerUpdate, SIGNAL(timeout()), this, SLOT(timerUpdate_timeout()));

    connect(this->pushButtonEnabled, SIGNAL(toggled(bool)), this, SLOT(pushButtonEnabled_toggled(bool)));
    connect(this->pushButtonStop, SIGNAL(toggled(bool)), this, SLOT(pushButtonStop_toggled(bool)));
    connect(this->pushButtonDebug, SIGNAL(toggled(bool)), this, SLOT(pushButtonDebug_toggled(bool)));
    connect(this->pushButtonError, SIGNAL(toggled(bool)), this, SLOT(pushButtonError_toggled(bool)));
    connect(this->pushButtonNetwork, SIGNAL(toggled(bool)), this, SLOT(pushButtonNetwork_toggled(bool)));
    connect(this->pushButtonChannel, SIGNAL(toggled(bool)), this, SLOT(pushButtonChannel_toggled(bool)));
    connect(this->pushButtonClear, SIGNAL(clicked()), this, SLOT(pushButtonClear_clicked()));
    connect(this->pushButtonBump, SIGNAL(clicked()), this, SLOT(pushButtonBump_clicked()));
    connect(this->pushButtonDisconnect, SIGNAL(clicked()), this, SLOT(pushButtonDisconnect_clicked()));
    connect(this->pushButtonKeep, SIGNAL(clicked()), this, SLOT(pushButtonKeep_clicked()));
    connect(this->pushButtonPlay, SIGNAL(clicked()), this, SLOT(pushButtonPlay_clicked()));
    connect(this->pushButtonDisconnectConn, SIGNAL(clicked()), this, SLOT(pushButtonDisconnectConn_clicked()));
    connect(this->listWidgetChannel, SIGNAL(itemSelectionChanged()), this, SLOT(timerUpdate_timeout()));
    connect(this->listWidgetConnection, SIGNAL(itemSelectionChanged()), this, SLOT(timerUpdate_timeout()));

    this->actionShow = new QAction(this);
    connect(this->actionShow, SIGNAL(triggered()), this, SLOT(showGui()));
    this->actionExit = new QAction(this);
    connect(this->actionExit, SIGNAL(triggered()), qApp, SLOT(quit()));

    this->actionMsgPeerCast = new QAction(this);
    this->actionMsgPeerCast->setCheckable(true);
    connect(this->actionMsgPeerCast, SIGNAL(triggered(bool)), this, SLOT(actionMsgPeerCast_triggered(bool)));
    this->actionTracker = new QAction(this);
    this->actionTracker->setCheckable(true);
    connect(this->actionTracker, SIGNAL(triggered(bool)), this, SLOT(actionTracker_triggered(bool)));
    this->actionTrack = new QAction(this);
    this->actionTrack->setCheckable(true);
    connect(this->actionTrack, SIGNAL(triggered(bool)), this, SLOT(actionTrack_triggered(bool)));
    this->actionHideGuiOnLaunch = new QAction(this);
    this->actionHideGuiOnLaunch->setCheckable(true);
    connect(this->actionHideGuiOnLaunch, SIGNAL(triggered(bool)), this, SLOT(actionHideGuiOnLaunch_triggered(bool)));

#ifndef Q_OS_MAC
    this->trayMenuPopup = new QMenu(this);
    this->trayMenuPopup->addAction(this->actionMsgPeerCast);
    this->trayMenuPopup->addAction(this->actionTracker);
    this->trayMenuPopup->addAction(this->actionTrack);

    this->trayMenuConfig = new QMenu(this);
    this->trayMenuConfig->addAction(this->actionHideGuiOnLaunch);

    this->trayMenu = new QMenu(this);
    this->trayMenu->addAction(this->actionShow);
    this->trayMenu->addSeparator();
    this->trayMenu->addMenu(this->trayMenuPopup);
    this->trayMenu->addMenu(this->trayMenuConfig);
    this->trayMenu->addSeparator();
    this->trayMenu->addAction(this->actionExit);

    this->tray = new QSystemTrayIcon(this);
    this->tray->setIcon(this->ico);
    this->tray->setContextMenu(this->trayMenu);
    this->tray->show();

    connect(this->tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(tray_activated(QSystemTrayIcon::ActivationReason)));
    connect(this->tray, SIGNAL(messageClicked()), this, SLOT(tray_messageClicked()));
#endif // Q_OS_MAC

    {
        QSettings s(this->iniFileName, QSettings::IniFormat);

        restoreGeometry(s.value("geometry").toByteArray());
        this->splitter->restoreState(s.value("splitter").toByteArray());
        this->actionHideGuiOnLaunch->setChecked(s.value("hideGuiOnLaunch", false).toBool());
    }

    reloadGui();
    pushButtonEnabled->setChecked(servMgr->autoServe);

    languageChange();
}

MainWindow::~MainWindow()
{
    QSettings s(this->iniFileName, QSettings::IniFormat);

    s.setValue("geometry", saveGeometry());
    s.setValue("splitter", this->splitter->saveState());
}

bool MainWindow::isHideGuiOnLaunch()
{
    return this->actionHideGuiOnLaunch->isChecked();
}

void MainWindow::languageChange()
{
#ifndef Q_OS_MAC
    this->tray->setToolTip(tr("PeerCastQt"));
    this->trayMenuPopup->setTitle(tr("Popup message"));
    this->trayMenuConfig->setTitle(tr("Config"));
#endif // Q_OS_MAC

    this->actionMsgPeerCast->setText(tr("PeerCast"));
    this->actionTracker->setText(tr("Broadcasters"));
    this->actionTrack->setText(tr("Track info"));
    this->actionHideGuiOnLaunch->setText(tr("Hide GUI on launch"));

    this->actionShow->setText(tr("Show GUI"));
    this->actionExit->setText(tr("Exit"));
}

void MainWindow::reloadGui()
{
    char sztemp[256];

    sprintf(sztemp, "%d", (int)servMgr->serverHost.port);
    this->lineEditPort->setText(sztemp);
    this->lineEditPassword->setText(servMgr->password);
    sprintf(sztemp, "%d", (int)servMgr->maxRelays);
    this->lineEditMaxRelays->setText(sztemp);

    this->pushButtonDebug->setChecked(servMgr->showLog&(1<<LogBuffer::T_DEBUG));
    this->pushButtonError->setChecked(servMgr->showLog&(1<<LogBuffer::T_ERROR));
    this->pushButtonNetwork->setChecked(servMgr->showLog&(1<<LogBuffer::T_NETWORK));
    this->pushButtonChannel->setChecked(servMgr->showLog&(1<<LogBuffer::T_CHANNEL));
    this->pushButtonStop->setChecked(servMgr->pauseLog);

    int mask = peercastInst->getNotifyMask();

    this->actionTracker->setChecked(mask & ServMgr::NT_BROADCASTERS);
    this->actionTrack->setChecked(mask & ServMgr::NT_TRACKINFO);
    this->actionMsgPeerCast->setChecked(mask & ServMgr::NT_PEERCAST);
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
    if(this->remainPopup < 0)
    {
        if(!g_qNotify.empty())
        {
            tNotifyInfo info = g_qNotify.front();
            g_qNotify.pop();
#ifndef Q_OS_MAC
            this->tray->showMessage(info.name, info.msg, QSystemTrayIcon::NoIcon, NOTIFY_TIMEOUT*1000);
#endif // Q_OS_MAC
            this->remainPopup = NOTIFY_TIMEOUT*10;
        }
    }
    else
    {
        this->remainPopup--;
    }

    if( !g_qLog.empty() ) {
        QString out;
        while( 1 ) {
            out += g_qLog.front();
            g_qLog.pop();
            if( g_qLog.empty() )
                break;
            else
                out += "\n";
        }

        this->textEditLog->append(out);
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
        bool block = this->listWidgetChannel->blockSignals(true);

        int n, y, count = 0;
        Channel *c;

        y = this->listWidgetChannel->verticalScrollBar()->value();
        n = this->listWidgetChannel->selectedCurrentRow();
        this->listWidgetChannel->clear();

        chanMgr->lock.on();

        c = chanMgr->channel;
        while(c)
        {
            if(n == count)
                sel_id = c->info.id;

            QListWidgetItem *item = new QListWidgetItem(this->listWidgetChannel);
            item->setData(Qt::UserRole, QVariant::fromValue(ChannelListItemData(c)));
            this->listWidgetChannel->addItem(item);

            c = c->next;
            count++;
        }

        chanMgr->lock.off();

        this->listWidgetChannel->setCurrentRow(n);
        this->listWidgetChannel->verticalScrollBar()->setValue(y);

        this->listWidgetChannel->blockSignals(block);
    }

    {
        bool block = this->listWidgetConnection->blockSignals(true);

        int n, y;
        Servent *s;

        y = this->listWidgetConnection->verticalScrollBar()->value();
        n = this->listWidgetConnection->selectedCurrentRow();
        this->listWidgetConnection->clear();

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

            QListWidgetItem *item = new QListWidgetItem(this->listWidgetConnection);
            item->setData(Qt::UserRole, QVariant::fromValue(ConnectionListItemData(s, info)));
            this->listWidgetConnection->addItem(item);

            s = s->next;
        }

        servMgr->lock.off();

        this->listWidgetConnection->setCurrentRow(n);
        this->listWidgetConnection->verticalScrollBar()->setValue(y);

        this->listWidgetConnection->blockSignals(block);
    }
}

void MainWindow::pushButtonBump_clicked()
{
    int n;
    QListWidgetItem *item;

    n = this->listWidgetChannel->selectedCurrentRow();
    item = this->listWidgetChannel->item(n);
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

    n = this->listWidgetChannel->selectedCurrentRow();
    item = this->listWidgetChannel->item(n);
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

    n = this->listWidgetChannel->selectedCurrentRow();
    item = this->listWidgetChannel->item(n);
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

    n = this->listWidgetChannel->selectedCurrentRow();
    item = this->listWidgetChannel->item(n);
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

    n = this->listWidgetConnection->selectedCurrentRow();
    item = this->listWidgetConnection->item(n);
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
    this->lineEditPort->setEnabled(state == 0);
    this->lineEditPassword->setEnabled(state == 0);
    this->lineEditMaxRelays->setEnabled(state == 0);

    if(state != 0)
    {
        if(!servMgr->autoServe)
        {
            QString str;
            unsigned short temp;
            bool success = false;

            str = this->lineEditPassword->text();;
            strcpy(servMgr->password, str.toUtf8().data());

            str = this->lineEditPort->text();
            temp = str.toUShort(&success);
            if(success)
                servMgr->serverHost.port = temp;

            str = this->lineEditMaxRelays->text();
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
    servMgr->pauseLog = (state != 0);
}

void MainWindow::pushButtonDebug_toggled(bool state)
{
    servMgr->showLog = (state != 0) ? servMgr->showLog|(1<<LogBuffer::T_DEBUG) : servMgr->showLog&~(1<<LogBuffer::T_DEBUG);
}

void MainWindow::pushButtonError_toggled(bool state)
{
    servMgr->showLog = (state != 0) ? servMgr->showLog|(1<<LogBuffer::T_ERROR) : servMgr->showLog&~(1<<LogBuffer::T_ERROR);
}

void MainWindow::pushButtonNetwork_toggled(bool state)
{
    servMgr->showLog = (state != 0) ? servMgr->showLog|(1<<LogBuffer::T_NETWORK) : servMgr->showLog&~(1<<LogBuffer::T_NETWORK);
}

void MainWindow::pushButtonChannel_toggled(bool state)
{
    servMgr->showLog = (state != 0) ? servMgr->showLog|(1<<LogBuffer::T_CHANNEL) : servMgr->showLog&~(1<<LogBuffer::T_CHANNEL);
}

void MainWindow::pushButtonClear_clicked()
{
    sys->logBuf->clear();
    this->textEditLog->clear();
    initTextEditLogMargin();
}

#ifndef Q_OS_MAC

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
    this->remainPopup = 0;
}

#endif // Q_OS_MAC

void MainWindow::actionMsgPeerCast_triggered(bool checked)
{
    setNotifyMask(ServMgr::NT_PEERCAST);
}

void MainWindow::actionTracker_triggered(bool checked)
{
    setNotifyMask(ServMgr::NT_BROADCASTERS);
}

void MainWindow::actionTrack_triggered(bool checked)
{
    setNotifyMask(ServMgr::NT_TRACKINFO);
}

void MainWindow::actionHideGuiOnLaunch_triggered(bool checked)
{
    QSettings s(this->iniFileName, QSettings::IniFormat);
    s.setValue("hideGuiOnLaunch", checked);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

void MainWindow::initTextEditLogMargin()
{
    QTextFrame *tf = this->textEditLog->document()->rootFrame();
    QTextFrameFormat tff = tf->frameFormat();
    tff.setTopMargin(1);
    tff.setBottomMargin(1);
    tff.setLeftMargin(2);
    tff.setRightMargin(2);
    tf->setFrameFormat(tff);
}

void MainWindow::setNotifyMask(ServMgr::NOTIFY_TYPE nt)
{
    int mask = peercastInst->getNotifyMask();
    mask ^= nt;
    peercastInst->setNotifyMask(mask);
    peercastInst->saveSettings();
}

