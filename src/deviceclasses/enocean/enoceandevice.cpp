//
//  Copyright (c) 2013-2014 plan44.ch / Lukas Zeller, Zurich, Switzerland
//
//  Author: Lukas Zeller <luz@plan44.ch>
//
//  This file is part of vdcd.
//
//  vdcd is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  vdcd is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with vdcd. If not, see <http://www.gnu.org/licenses/>.
//

#include "enoceandevice.hpp"

#include "enoceandevicecontainer.hpp"

#include "buttonbehaviour.hpp"
#include "sensorbehaviour.hpp"
#include "outputbehaviour.hpp"

#include "enoceanrps.hpp"
#include "enocean1bs.hpp"
#include "enocean4bs.hpp"
//#include "enoceanVld.hpp"


using namespace p44;


#pragma mark - EnoceanChannelHandler

EnoceanChannelHandler::EnoceanChannelHandler(EnoceanDevice &aDevice) :
  device(aDevice),
  dsChannelIndex(0)
{
}


#pragma mark - EnoceanDevice

EnoceanDevice::EnoceanDevice(EnoceanDeviceContainer *aClassContainerP, EnoceanSubDevice aTotalSubdevices) :
  Device(aClassContainerP),
  eeProfile(eep_profile_unknown),
  eeManufacturer(manufacturer_unknown),
	totalSubdevices(aTotalSubdevices),
  alwaysUpdateable(false),
  pendingDeviceUpdate(false),
  subDevice(0)
{
}


EnoceanDeviceContainer &EnoceanDevice::getEnoceanDeviceContainer()
{
  return *(static_cast<EnoceanDeviceContainer *>(classContainerP));
}


EnoceanAddress EnoceanDevice::getAddress()
{
  return enoceanAddress;
}


EnoceanSubDevice EnoceanDevice::getSubDevice()
{
  return subDevice;
}


EnoceanSubDevice EnoceanDevice::getTotalSubDevices()
{
	return totalSubdevices;
}



void EnoceanDevice::setAddressingInfo(EnoceanAddress aAddress, EnoceanSubDevice aSubDevice)
{
  enoceanAddress = aAddress;
  subDevice = aSubDevice;
  deriveDsUid();
}


void EnoceanDevice::setEEPInfo(EnoceanProfile aEEProfile, EnoceanManufacturer aEEManufacturer)
{
  eeProfile = aEEProfile;
  eeManufacturer = aEEManufacturer;
}


EnoceanProfile EnoceanDevice::getEEProfile()
{
  return eeProfile;
}


EnoceanManufacturer EnoceanDevice::getEEManufacturer()
{
  return eeManufacturer;
}



void EnoceanDevice::deriveDsUid()
{
  // UUID in enOcean name space
  //   name = xxxxxxxx:s (x=8 digit enocean hex UPPERCASE address, s=decimal subdevice index, 0..n)
  DsUid enOceanNamespace(DSUID_ENOCEAN_NAMESPACE_UUID);
  string s = string_format("%08lX", getAddress()); // base address comes from
  dSUID.setNameInSpace(s, enOceanNamespace);
  dSUID.setSubdeviceIndex(getSubDevice()*idBlockSize()); // space subdevices according to idBlockSize (e.g. up/down-buttons will reserve a second subdevice to allow vdSM to expand it into 2 separate buttons)
}


string EnoceanDevice::hardwareGUID()
{
  // GTIN is 24bit company prefix + 20bit item reference, SGTIN adds a 48bit serial number as third element: urn:epc:id:sgtin:COMPANYPREFIX.ITEMREF.SERIALNO
  // TODO: create a GTIN if there is an official scheme for it
  return string_format("enoceanaddress:%08lX", getAddress());
}


string EnoceanDevice::modelName()
{
  return string_format("%s enOcean device (%02X-%02X-%02X)", manufacturerName().c_str(), EEP_RORG(eeProfile), EEP_FUNC(eeProfile), EEP_TYPE(eeProfile));
}


string EnoceanDevice::manufacturerName()
{
  return EnoceanComm::manufacturerName(eeManufacturer);
}


void EnoceanDevice::disconnect(bool aForgetParams, DisconnectCB aDisconnectResultHandler)
{
  // clear learn-in data from DB
  getEnoceanDeviceContainer().db.executef("DELETE FROM knownDevices WHERE enoceanAddress=%d AND subdevice=%d", getAddress(), getSubDevice());
  // disconnection is immediate, so we can call inherited right now
  inherited::disconnect(aForgetParams, aDisconnectResultHandler);
}



void EnoceanDevice::addChannelHandler(EnoceanChannelHandlerPtr aChannelHandler)
{
  // create channel number
  aChannelHandler->channel = channels.size();
  // add to my local list
  channels.push_back(aChannelHandler);
  // register behaviour of the channel with the device
  addBehaviour(aChannelHandler->behaviour);
}




EnoceanChannelHandlerPtr EnoceanDevice::channelForBehaviour(const DsBehaviour *aBehaviourP)
{
  EnoceanChannelHandlerPtr handler;
  for (EnoceanChannelHandlerVector::iterator pos = channels.begin(); pos!=channels.end(); ++pos) {
    if ((*pos)->behaviour.get()==static_cast<const DsBehaviour *>(aBehaviourP)) {
      handler = *pos;
      break;
    }
  }
  return handler;
}



void EnoceanDevice::sendOutgoingUpdate()
{
  pendingDeviceUpdate = true; // always send, for now (but nothing will be sent for devices without outputs)
  if (pendingDeviceUpdate) {
    // collect data from all channels to compose an outgoing message
    Esp3PacketPtr outgoingEsp3Packet;
    for (EnoceanChannelHandlerVector::iterator pos = channels.begin(); pos!=channels.end(); ++pos) {
      (*pos)->collectOutgoingMessageData(outgoingEsp3Packet);
    }
    if (outgoingEsp3Packet) {
      // set destination
      outgoingEsp3Packet->setRadioDestination(enoceanAddress); // the target is the device I manage
      outgoingEsp3Packet->finalize();
      LOG(LOG_INFO, "enOcean device %s: sending outgoing packet:\n%s", shortDesc().c_str(), outgoingEsp3Packet->description().c_str());
      // send it
      getEnoceanDeviceContainer().enoceanComm.sendPacket(outgoingEsp3Packet);
    }
    pendingDeviceUpdate = false; // done
  }
}


void EnoceanDevice::applyChannelValues(CompletedCB aCompletedCB, bool aForDimming)
{
  // trigger updating all device outputs
  for (int i=0; i<numChannels(); i++) {
    if (getChannelByIndex(i, true)) {
      // channel needs update
      pendingDeviceUpdate = true;
      break; // no more checking needed, need device level update anyway
    }
  }
  if (pendingDeviceUpdate && alwaysUpdateable) {
    // send immediately
    sendOutgoingUpdate();
  }
  inherited::applyChannelValues(aCompletedCB, aForDimming);
}


void EnoceanDevice::handleRadioPacket(Esp3PacketPtr aEsp3PacketPtr)
{
  LOG(LOG_INFO, "enOcean device %s: received packet:\n%s", shortDesc().c_str(), aEsp3PacketPtr->description().c_str());
  // pass to every channel
  for (EnoceanChannelHandlerVector::iterator pos = channels.begin(); pos!=channels.end(); ++pos) {
    (*pos)->handleRadioPacket(aEsp3PacketPtr);
  }
  // if device cannot be update whenever output value change is requested, send updates after receiving a message
  if (!alwaysUpdateable) {
    // send updates, if any
    sendOutgoingUpdate();
  }
}


string EnoceanDevice::description()
{
  string s = inherited::description();
  string_format_append(s, "- Enocean Address = 0x%08lX, subDevice=%d\n", enoceanAddress, subDevice);
  string_format_append(s,
    "- EEP RORG/FUNC/TYPE: %02X %02X %02X, Manufacturer = %s (%03X)\n",
    (eeProfile>>16) & 0xFF,
    (eeProfile>>8) & 0xFF,
    eeProfile & 0xFF,
    manufacturerName().c_str(),
    eeManufacturer
  );
  // show channels
  for (EnoceanChannelHandlerVector::iterator pos = channels.begin(); pos!=channels.end(); ++pos) {
    string_format_append(s, "- EnOcean device channel #%d: %s\n", (*pos)->channel, (*pos)->shortDesc().c_str());
  }
  return s;
}


#pragma mark - device factory


EnoceanDevicePtr EnoceanDevice::newDevice(
  EnoceanDeviceContainer *aClassContainerP,
  EnoceanAddress aAddress, EnoceanSubDevice aSubDevice,
  EnoceanProfile aEEProfile, EnoceanManufacturer aEEManufacturer,
  EnoceanSubDevice &aNumSubdevices,
  bool aSendTeachInResponse
) {
  EnoceanDevicePtr newDev;
  RadioOrg rorg = EEP_RORG(aEEProfile);
  // dispatch to factory according to RORG
  switch (rorg) {
    case rorg_RPS:
      newDev = EnoceanRpsHandler::newDevice(aClassContainerP, aAddress, aSubDevice, aEEProfile, aEEManufacturer, aNumSubdevices, aSendTeachInResponse);
      break;
    case rorg_1BS:
      newDev = Enocean1bsHandler::newDevice(aClassContainerP, aAddress, aSubDevice, aEEProfile, aEEManufacturer, aNumSubdevices, aSendTeachInResponse);
      break;
    case rorg_4BS:
      newDev = Enocean4bsHandler::newDevice(aClassContainerP, aAddress, aSubDevice, aEEProfile, aEEManufacturer, aNumSubdevices, aSendTeachInResponse);
      break;
//    case rorg_VLD:
//      newDev = EnoceanVldHandler::newDevice(aClassContainerP, aAddress, aSubDevice, aEEProfile, aEEManufacturer, aNumSubdevices, aSendTeachInResponse);
//      break;
    default:
      LOG(LOG_WARNING,"EnoceanDevice::newDevice: unknown RORG = 0x%02X\n", rorg);
      break;
  }
  // return device (or empty if none created)
  return newDev;
}


int EnoceanDevice::createDevicesFromEEP(EnoceanDeviceContainer *aClassContainerP, Esp3PacketPtr aLearnInPacket)
{
  EnoceanSubDevice totalSubDevices = 1; // at least one
  EnoceanSubDevice subDevice = 0;
  while (subDevice<totalSubDevices) {
    EnoceanDevicePtr newDev = newDevice(
      aClassContainerP,
      aLearnInPacket->radioSender(), subDevice,
      aLearnInPacket->eepProfile(), aLearnInPacket->eepManufacturer(),
      totalSubDevices, // possibly update total
      subDevice==0 // allow sending teach-in response for first subdevice only
    );
    if (!newDev) {
      // could not create a device
      break;
    }
    // created device
    // - add it to the container
    aClassContainerP->addAndRemeberDevice(newDev);
    // - count it
    subDevice++;
  }
  // return number of devices created
  return subDevice;
}

