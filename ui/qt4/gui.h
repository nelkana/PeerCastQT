// --------------------------------------------------------------------------
// File: gui.h
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
#ifndef GUIH
#define GUIH

#include "ui_mainwindow.h"

#include "servmgr.h"

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QFrame>
#include <QTimer>
#include <QIcon>
#include <QMenu>
#include <QEvent>
#ifndef Q_OS_MAC
#include <QSystemTrayIcon>
#endif // Q_OS_MAC

class MainWindow : public QWidget, protected Ui::MainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    virtual ~MainWindow();

    virtual bool isHideGuiOnLaunch();
    virtual void languageChange();
    virtual void reloadGui();

public slots:
    virtual void showGui();
    virtual void showHideGui();

    virtual void timerLogUpdate_timeout();
    virtual void timerUpdate_timeout();

    virtual void pushButtonBump_clicked();
    virtual void pushButtonDisconnect_clicked();
    virtual void pushButtonKeep_clicked();
    virtual void pushButtonPlay_clicked();
    virtual void pushButtonDisconnectConn_clicked();
    virtual void pushButtonEnabled_toggled(bool state);
    virtual void pushButtonStop_toggled(bool state);
    virtual void pushButtonDebug_toggled(bool state);
    virtual void pushButtonError_toggled(bool state);
    virtual void pushButtonNetwork_toggled(bool state);
    virtual void pushButtonChannel_toggled(bool state);
    virtual void pushButtonClear_clicked();

#ifndef Q_OS_MAC
    virtual void tray_activated(QSystemTrayIcon::ActivationReason reason);
    virtual void tray_messageClicked();
#endif // Q_OS_MAC

    virtual void actionMsgPeerCast_triggered(bool checked);
    virtual void actionTracker_triggered(bool checked);
    virtual void actionTrack_triggered(bool checked);
    virtual void actionHideGuiOnLaunch_triggered(bool checked);

protected:
    virtual void closeEvent(QCloseEvent *event);
    virtual void initTextEditLogMargin();
    virtual void setNotifyMask(ServMgr::NOTIFY_TYPE nt);

    QIcon ico;
    QString iniFileName;

    int remainPopup;

#ifndef Q_OS_MAC
    QMenu *trayMenu;
    QMenu *trayMenuPopup;
    QMenu *trayMenuConfig;
    QSystemTrayIcon *tray;
#endif // Q_OS_MAC

    QAction *actionShow;
    QAction *actionExit;
    QAction *actionMsgPeerCast;
    QAction *actionTracker;
    QAction *actionTrack;
    QAction *actionHideGuiOnLaunch;

    QTimer *timerLogUpdate;
    QTimer *timerUpdate;
};

#endif // GUIH

