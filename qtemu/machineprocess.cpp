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

#include "machineprocess.h"
#include "networksystem.h"
#include "config.h"

#include <QCoreApplication>
#include <QSettings>
#include <QTime>
#include <QDir>

MachineProcess::MachineProcess(QObject *parent)
               :QProcess(parent),
                bootFromCdEnabled(false),
                bootFromFloppyEnabled(false),
                snapshotEnabled(false),
                networkEnabled(false),
                mouseEnabled(false),
                timeEnabled(false),
                additionalOptionsEnabled(false),
                memoryInt(-1),
                cpuInt(-1)
{
    paused=false;
    doResume=false;
    soundSystem(0);
    getVersion();
    networkSystem = new NetworkSystem(this);
    connect(this, SIGNAL(readyReadStandardOutput()), this, SLOT(readProcess()));
    connect(this, SIGNAL(readyReadStandardError()), this, SLOT(readProcessErrors()));
    connect(this, SIGNAL(stdout(const QString&)), this, SLOT(writeDebugInfo(const QString&)));
    connect(this, SIGNAL(stdin(const QString&)), this, SLOT(writeDebugInfo(const QString&)));
}


void MachineProcess::start()
{
    QSettings settings("QtEmu", "QtEmu");
    QStringList env = QProcess::systemEnvironment();
    QStringList arguments;

    // Append the additional parameters first so wrapper programs like vde
    // can be used to start the virtual machine, e.g. vdeq kvm or vdeq qemu
    // where vdeq is specified as the command and kvm/qemu as the additional
    // option or parameter.
    if (additionalOptionsEnabled && !additionalOptionsString.isEmpty())
      arguments << additionalOptionsString.split(" ", QString::SkipEmptyParts);

    if (snapshotEnabled)
        arguments << "-snapshot";
    
    if (networkEnabled)
    {   //TODO:implement new network stuff
    
        //load up saved network setup
        networkSystem->loadNics();
        
           if (networkSystem->numNics() != 0)
               arguments << networkSystem->getOptionList();

           if (!networkCustomOptionsString.isEmpty())
               arguments << networkCustomOptionsString.split(" ", QString::SkipEmptyParts);
    }
    else
        arguments << "-net" << "none";

    /*using the drive syntax, multiple drives can be added to a VM. drives can be specified as
     disconnected, allowing media to be inserted after bootup. the initialization can take the form
     of a loop, and the device settings can be put in an array of drive objects. this also would
     allow both IDE and SCSI drives to be specified, making more than 4 disks possible.
    in order to take advantage of this, the interface will need a custom drive widget that can be
     inserted multiple times easily.
     to save these complex settings, settings can be saved under <drives> with a specific <drive>
     in the .qte file.
   */

    if (!cdRomPathString.isEmpty())
    {
        cdRomPathString = cdRomPathString.replace(QRegExp(","),",,");
        arguments << "-drive" << "file=" + cdRomPathString + ",if=ide,bus=1,unit=0,media=cdrom";
        if (bootFromCdEnabled)
            arguments << "-boot" << "d";
    }
    else//allows the cdrom to exist if not specified
    {
        arguments << "-drive" << "if=ide,bus=1,unit=0,media=cdrom";//TODO:make the drive location configurable
    }
    
    //TODO: modify to support multiple floppies(index=0 and index=1)
    if (!floppyDiskPathString.isEmpty())
    {
        arguments << "-drive" << "file=" + floppyDiskPathString + ",index=0,if=floppy";
        if (bootFromFloppyEnabled)
            arguments << "-boot" << "a";
    }
    
    if (soundEnabled)
    {
    //FIXME: this does not work yet with alsa... no idea why. specifying oss, which is the default anyway works ok.
        arguments << "-soundhw" << "es1370";
        env << "QEMU_AUDIO_DRV=" + useSoundSystem;
    }
    
    if (memoryInt > 0)
      arguments << "-m" << QString::number(memoryInt);

    if (cpuInt > 1)
      arguments << "-smp" << QString::number(cpuInt);

    if (mouseEnabled)
        arguments << "-usb" << "-usbdevice" << "tablet";

    if (timeEnabled)
        arguments << "-localtime";

    if (!virtualizationEnabled&&settings.value("runsKVM").toBool())
        arguments << "-no-kvm";
    else if (!virtualizationEnabled&&!settings.value("runsKVM").toBool())
        arguments << "-no-kqemu";
    else if (virtualizationEnabled&&!settings.value("runsKVM").toBool())
        arguments << "-kernel-kqemu";

    if (doResume)
        arguments << "-loadvm" << snapshotNameString;

    //allow access to the monitor via stdio
    //FIXME: does this work in windows? if so, then most of the new features will work in
    // windows too. if not, the pipe:filename option might work, if we can make a named pipe...
    // http://en.wikipedia.org/wiki/Named_pipe
#ifndef Q_OS_WIN32
    arguments << "-monitor" << "stdio";
#endif

    if((versionMinor + versionBugfix*.1)>=9.1||kvmVersion>=60)
        arguments << "-name" << "\"" + machineNameString + "\"";

    //TODO: use new -drive syntax (man qemu for info)
    // Add the VM image name...
    arguments << pathString;

#ifdef DEVELOPER
    QString debugString = QString();
    for (int i = 0; i < arguments.size(); ++i)
        debugString = debugString + arguments.at(i).toLocal8Bit().constData() + ' ';
    qDebug(debugString.toLocal8Bit().constData());
#endif

    connect(this, SIGNAL(finished(int)), SLOT(afterExitExecute()));

    QString command = settings.value("beforeStart").toString();
    if (!command.isEmpty())
    {
        QStringList commandList;
        commandList = command.split("\n");
        for (int i = 0; i < commandList.size(); ++i)
        {
            QProcess::start(commandList.at(i).toLocal8Bit().constData());
            while (waitForFinished())
            {
                QTime sleepTime = QTime::currentTime().addMSecs(5);
                while (QTime::currentTime() < sleepTime)
                    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
            }
        }
    }

    setEnvironment(env);
#ifndef Q_OS_WIN32
    QProcess::start(settings.value("command", "qemu").toString(), arguments);
#elif defined(Q_OS_WIN32)
    arguments << "-L" << ".";
    QString qemuCommand = settings.value("command", QCoreApplication::applicationDirPath() + "/qemu/qemu.exe").toString();
    QDir *path = new QDir(qemuCommand);
    path->cdUp();
    setWorkingDirectory(path->path());
    QProcess::start(qemuCommand, arguments);
#endif
}

void MachineProcess::afterExitExecute()
{
    QSettings settings("QtEmu", "QtEmu");

    QString command = settings.value("afterExit").toString();
    if (!command.isEmpty())
    {
        QStringList commandList;
        commandList = command.split("\n");
        for (int i = 0; i < commandList.size(); ++i)
        {
            QProcess::start(commandList.at(i).toLocal8Bit().constData());
            while (waitForFinished())
            {
                QTime sleepTime = QTime::currentTime().addMSecs(5);
                while (QTime::currentTime() < sleepTime)
                    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
            }
        }
    }

    doResume=false;
}

void MachineProcess::path(const QString &newPath)
{
    pathString = newPath;
}

void MachineProcess::cdRomPath(const QString &newPath)
{
    cdRomPathString = newPath;
}

void MachineProcess::cdBoot(int value)
{
    bootFromCdEnabled = (value == Qt::Checked);
}

void MachineProcess::floppyDiskPath(const QString &newPath)
{
    floppyDiskPathString = newPath;
}

void MachineProcess::floppyBoot(int value)
{
    bootFromFloppyEnabled = (value == Qt::Checked);
}

void MachineProcess::snapshot(int value)
{
    if(state() == QProcess::Running && snapshotEnabled == true)
    {
        write("commit\n");
    }

#ifdef DEVELOPER
    snapshotEnabled = true;
#else
    snapshotEnabled = (value == Qt::Checked);
#endif
}

void MachineProcess::network(int value)
{
    networkEnabled = (value == Qt::Checked);
}

void MachineProcess::sound(int value)
{
    soundEnabled = (value == Qt::Checked);
}

void MachineProcess::time(int value)
{
    timeEnabled = (value == Qt::Checked);
}

void MachineProcess::virtualization(int value)
{
    virtualizationEnabled = (value == Qt::Checked);
}

void MachineProcess::mouse(int value)
{
    mouseEnabled = (value == Qt::Checked);
}

void MachineProcess::useAdditionalOptions(int value)
{
    additionalOptionsEnabled = (value == Qt::Checked);
}

void MachineProcess::memory(int value)
{
    memoryInt = value;
}

void MachineProcess::cpu(int value)
{
    cpuInt = value;
}

void MachineProcess::networkCustomOptions(const QString& options)
{
    networkCustomOptionsString = options;
}

void MachineProcess::additionalOptions(const QString& options)
{
    additionalOptionsString = options;
}

void MachineProcess::name(const QString & name)
{
    machineNameString = name;
}

void MachineProcess::resume() {resume("Default");}
void MachineProcess::resume(const QString & snapshotName)
{
    snapshotNameString = snapshotName;
    if(state()==QProcess::Running)
    {
        write("loadvm " + snapshotName.toAscii() + '\n');
        emit resuming(snapshotName);
    }
    else
    {
        doResume=true;
        start();
        emit resuming(snapshotName);
        write("\n");
    }
    connect(this, SIGNAL(stdout(const QString&)),this,SLOT(resumeFinished(const QString&)));

}

void MachineProcess::resumeFinished(const QString& returnedText)
{
    if(returnedText == "(qemu)")
    {
        emit resumed(snapshotNameString);
        disconnect(this, SIGNAL(stdout(const QString&)),this,SLOT(resumeFinished(const QString&)));
    }
    //might need to reconnect the usb tablet here...
}

void MachineProcess::suspend() {suspend("Default");}
void MachineProcess::suspend(const QString & snapshotName)
{
    snapshotNameString = snapshotName;
    emit suspending(snapshotName);
    //usb is not properly resumed, so we need to disable it first in order to keep things working afterwords.
    //this also means that we need to dynamically get usb devices to unload and save them with the qtemu config
    //file for proper usb support with suspend. as it is we just unload the tablet, which is all we know about.
    if(mouseEnabled) 
    {
        write("usb_del 0.1\n");
        sleep(2);//wait for the guest OS to notice
    }
    write("stop\n");
    write("savevm " + snapshotName.toAscii() + '\n');
    connect(this, SIGNAL(stdout(const QString&)),this,SLOT(suspendFinished(const QString&)));
}

void MachineProcess::suspendFinished(const QString& returnedText)
{
    if(returnedText == "(qemu)")
    {
        write("cont\n");
        emit suspended(snapshotNameString);
        disconnect(this, SIGNAL(stdout(const QString&)),this,SLOT(suspendFinished(const QString&)));
    }
}

void MachineProcess::togglePause()
{
     paused ? write("cont\n") : write("stop\n");
     paused = !paused;
}

void MachineProcess::stop()
{
    write("system_powerdown\n");
}

void MachineProcess::forceStop()
{
    write("quit\n");
}

void MachineProcess::readProcess()
{
    QByteArray rawOutput = readAllStandardOutput();
    emit rawConsole(rawOutput); //for connection to a fully interactive console... eventually
    QString convOutput = rawOutput;
    QStringList splitOutput = convOutput.split("[K");
    if (splitOutput.last()==splitOutput.first())
    {
        emit stdout(convOutput.simplified());
    }
    else 
    {
        if(!splitOutput.last().isEmpty())
        {
            QString cleanOutput = splitOutput.last().remove(QRegExp("\[[KD]."));
            emit stdout(cleanOutput.simplified());
        }
    }
}

void MachineProcess::readProcessErrors()
{
    emit error(readAllStandardError());
}

qint64 MachineProcess::write ( const QByteArray & byteArray )
{
    emit stdin(((QString)byteArray).simplified());
    return QProcess::write(byteArray);
}

void MachineProcess::writeDebugInfo(const QString & debugText)
{
#ifdef DEVELOPER
    qDebug(debugText.toAscii());
#endif
}

void MachineProcess::getVersion()
{
    QSettings settings("QtEmu", "QtEmu");
    QString versionString;
    QString qemuCommand = settings.value("command", QCoreApplication::applicationDirPath() + "/qemu/qemu.exe").toString();
    QProcess *findVersion = new QProcess();
    findVersion->start(qemuCommand);
    findVersion->waitForFinished();
    QString infoString = findVersion->readLine();
    QStringList infoStringList = infoString.split(" ");
    
    versionString = infoStringList.at(4);
    QStringList versionStringList = versionString.split(".");
    versionMajor = versionStringList.at(0).toInt();
    versionMinor = versionStringList.at(1).toInt();
    versionBugfix = versionStringList.at(2).toInt();
    if(settings.value("runsKVM")!="0")
    {
        versionString = infoStringList.at(5);
        versionString.remove(QRegExp("[(),]"));
        kvmVersion = versionString.remove(QRegExp("kvm-")).toInt();
    }
    #ifdef DEVELOPER
    qDebug(("kvm: " + QString::number(kvmVersion) + " qemu: " + QString::number(versionMajor) + '.' + QString::number(versionMinor) + '.' + QString::number(versionBugfix)).toAscii());
    #endif
}

void MachineProcess::soundSystem(int value)
{
    (value == Qt::Checked) ? useSoundSystem="alsa" : useSoundSystem="oss";
}

//TODO: accept a drive assignment to eject/insert. this means multiple drives would be supported
void MachineProcess::changeCdrom()
{
    write("eject -f ide1-cd0\n");
    write("change ide1-cd0 " + cdRomPathString.toAscii() + '\n');
}

//TODO: accept a drive assignment to eject/insert.
void MachineProcess::changeFloppy()
{
    write("eject -f floppy0\n");
    write("change floppy0 " + floppyDiskPathString.toAscii() + '\n');
}

//TODO: all network stuff needs to be implemented. some of this will require use of sudo. (bridged modes)
//a wizard also needs to be made to set up sudo to work without manual intervention.
void MachineProcess::smbFolderPath(const QString & newPath)
{
}

void MachineProcess::toggleNetworkMode(bool userMode)
{
}

void MachineProcess::localBridgeModeNetwork(int value)
{
}

void MachineProcess::sharedVlanNetwork(int value)
{
}	