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

#include "ui_mainform.h"

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
#ifndef _APPLE
#include <QSystemTrayIcon>
#endif // _APPLE

class QMainForm : public QWidget, private Ui::MainFormBase
{
    Q_OBJECT

public:
    QMainForm(QWidget *parent = 0);
    virtual ~QMainForm();

    QIcon ico;
    QString iniFileName;

    int remainPopup;

#ifndef _APPLE
    QMenu* trayMenu;
    QMenu* trayMenuPopup;
    QSystemTrayIcon* tray;
#endif

    QAction* actionExit;
    QAction* actionShow;
    QAction* actionTracker;
    QAction* actionTrack;
    QAction* actionMsgPeerCast;

    QTimer* timerLogUpdate;
    QTimer* timerUpdate;

    void reloadGui();
    void setNotifyMask(ServMgr::NOTIFY_TYPE nt);

    void languageChange();

public slots:
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

#ifndef _APPLE
    virtual void tray_activated(QSystemTrayIcon::ActivationReason reason);
    virtual void tray_messageClicked();
#endif

    virtual void actionTracker_triggered(bool checked);
    virtual void actionTrack_triggered(bool checked);
    virtual void actionMsgPeerCast_triggered(bool checked);

protected:
    virtual void closeEvent(QCloseEvent *event);
};

#endif // GUIH

