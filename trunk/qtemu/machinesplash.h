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

/****************************************************************************
**
** C++ Interface: machinesplash
**
** Description:  builds the splashscreen each machine has with an embedded
** preview
**
****************************************************************************/

#ifndef MACHINESPLASH_H
#define MACHINESPLASH_H

#include <QWidget>

class QImage;
class QLabel;
class QSvgWidget;
class QStackedLayout;
class QRectF;
class QGridLayout;
class QFrame;
class QSettings;
class QPixmap;
class QString;

/**
	@author Ben Klopfenstein <benklop@gmail.com>
*/
class MachineSplash : public QWidget
{
Q_OBJECT
public:
    MachineSplash(QWidget *parent = 0);

    ~MachineSplash();
    
    void setPreview(const QString previewLocation = QString());
    
private:

    void resizeEvent(QResizeEvent * event);
    QSvgWidget *splashImage;
    QLabel *previewImage;
    QStackedLayout *layout;
    QRectF *previewBounds;
    QGridLayout *previewLayout;
    QPixmap alpha;
    QString previewLoc;
    void doResize();
    void getPreviewRect();


};

#endif
