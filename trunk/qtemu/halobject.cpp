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

#include "halobject.h"

#include <QDebug>

/***************************************************************************
**
** HalObject is supposed to be used ONCE by qtemu, and each VM talks to it.
** HalObject or another class should assure that only one machine uses each
** device, but this is not done yet.
**
***************************************************************************/
HalObject::HalObject()
{
   hal = new QDBusInterface("org.freedesktop.Hal",
                                               "/org/freedesktop/Hal/Manager",
                                               "org.freedesktop.Hal.Manager",
                                               QDBusConnection::systemBus(),
                                               this);

   connect(hal, SIGNAL(DeviceAdded(QString)), this, SLOT(halDeviceAdded(QString)));
   connect(hal, SIGNAL(DeviceRemoved(QString)), this, SLOT(halDeviceRemoved(QString)));

   QDBusReply<QStringList> deviceList = hal->call("GetAllDevices");
   if (deviceList.isValid())
   {
       for(int i = 0; i < deviceList.value().size(); i++)
           halDeviceAdded(deviceList.value().at(i));
   }
   else
   {
       //hal is probably not installed or running
   }


}

void HalObject::halDeviceAdded(QString name)
{
    QDBusInterface * tempInterface = new QDBusInterface("org.freedesktop.Hal",
                                               name,
                                               "org.freedesktop.Hal.Device",
                                               QDBusConnection::systemBus(),
                                               this);
    //generic device signal
    emit deviceAdded(name);

#ifdef DEVELOPER
    qDebug(name.toAscii());
#endif

    //USB device that is not a hub added...
    if(tempInterface->call("GetProperty", "info.subsystem").arguments().at(0).toString() == "usb_device" &&
       tempInterface->call("GetProperty", "usb_device.num_ports").arguments().at(0).toInt() == 0 )
    {
        UsbDevice device;
        device.address = tempInterface->call("GetProperty", "usb_device.bus_number").arguments().at(0).toString() + "." +
                         tempInterface->call("GetProperty", "usb_device.linux.device_number").arguments().at(0).toString();
        device.id = name;
        device.product = tempInterface->call("GetProperty", "info.product").arguments().at(0).toString();
        device.vendor = tempInterface->call("GetProperty", "info.vendor").arguments().at(0).toString();
        usbDeviceHash.insert(name, device);
        emit usbAdded(name, device);
    }
    else if(tempInterface->call("GetProperty", "info.capabilities").arguments().contains("volume.disc"))
    {
        emit opticalAdded(tempInterface->call("GetProperty", "volume.label").arguments().at(0).toString(), tempInterface->call("GetProperty", "block.device").arguments().at(0).toString());

    }
}

void HalObject::halDeviceRemoved(QString name)
{
    //generic device signal
    emit(deviceRemoved(name));

#ifdef DEVELOPER
    qDebug(name.toAscii());
#endif

    //USB device that is not a hub deleted...
    if(usbDeviceHash.contains(name))
    {
        emit usbRemoved(name, usbDeviceHash.value(name));
        usbDeviceHash.remove(name);
    }

}

const QList<UsbDevice> HalObject::usbList()
{
    return usbDeviceHash.values();
}
