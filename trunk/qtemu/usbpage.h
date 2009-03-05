/****************************************************************************
**
** Copyright (C) 2008 Ben Klopfenstein <benklop@gmail.com>
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

#ifndef USBPAGE_H
#define USBPAGE_H

#include <QWidget>

#include "ui_usbpage.h"

class MachineConfigObject;

class UsbPage : public QWidget , public Ui::UsbPage
{
Q_OBJECT
public:
    UsbPage(MachineConfigObject *config, QWidget *parent = 0);

    ~UsbPage();

private:
    MachineConfigObject *config;
    void registerWidgets();
};

#endif // USBPAGE_H
