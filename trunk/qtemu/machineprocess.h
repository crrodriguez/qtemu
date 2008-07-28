/****************************************************************************
**
** Copyright (C) 2006-2008 Urs Wolfer <uwolfer @ fwo.ch>
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

#ifndef MACHINEPROCESS_H
#define MACHINEPROCESS_H

#include <QProcess>
#include "networksystem.h"

class MachineProcess : public QProcess
{
    Q_OBJECT

public:
    MachineProcess(QObject *parent = 0);
    qint64 write(const QByteArray & byteArray);

public slots:
    void start();
    void resume(const QString& snapshotName);
    void resume();
    void suspend(const QString& snapshotName);
    void suspend();
    void stop();
    void forceStop();
    void togglePause();
    void supressError(QString errorText);
    void changeCdrom();
    void changeFloppy();
    void loadCdrom();

signals:
    void suspending(const QString & snapshotName);
    void suspended(const QString & snapshotName);
    void booting();
    void resuming(const QString & snapshotName);
    void resumed(const QString & snapshotName);
    void error(const QString & errorText);
    void stdout(const QString & stdoutText);
    void stdin(const QString & stdoutText);
    void rawConsole(const QString & consoleOutput);
    void cleanConsole(const QString & consoleOutput);

private:
    void getVersion();
    void commitTmp();
    void createTmp();
    QString snapshotNameString;
    QStringList supressedErrors;
    long versionMajor, versionMinor, versionBugfix, kvmVersion;
    bool paused;
    bool doResume;
    //NetworkSystem* networkSystem;

private slots:
    void afterExitExecute();
    void readProcess();
    void readProcessErrors();
    void writeDebugInfo(const QString& debugText);
    void resumeFinished(const QString& returnedText);
    void suspendFinished(const QString& returnedText);
    void startedBooting(const QString& text);
    void deleteTmp(int successfulCommit);
};

#endif