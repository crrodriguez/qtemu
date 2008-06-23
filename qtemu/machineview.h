/****************************************************************************
**
** Copyright (C) 2008 Ben Klopfenstein <benklop @ gmail.com>
**
** This file is part of QtEmu.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Library General Public License
** along with this library; see the file COPYING.LIB.  If not, write to
** the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
** Boston, MA 02110-1301, USA.
**
****************************************************************************/

#ifndef MACHINEVIEW_H
#define MACHINEVIEW_H

#include "vnc/vncview.h"

#include <QScrollArea>
#include <QKeyEvent>
#include <QSvgWidget>


/**
	@author Ben Klopfenstein <benklop@gmail.com>
*/
class MachineView : public QScrollArea
{
Q_OBJECT
public:
    MachineView(QWidget *parent = 0, const QString defaultImage = QString());

    ~MachineView();

    void showSplash(bool show);
    void setSplash(QString location);
    void enableScaling(bool scale);
    bool isScaleable() const;
    void machineNumber(int machine);
    void initView();
    void fullscreen(bool enabled);
    void captureAllKeys(bool enabled);
    void sendKey(QKeyEvent *event);
        
public slots:
    void newViewSize();
    
private:
    void resizeEvent(QResizeEvent * event);
    
    void resizeView(int widgetWidth, int widgetHeight);
    
    VncView *view;
    QSvgWidget *splash;
    bool scaleable;
    bool splashShown;
    int port;
};



#endif
