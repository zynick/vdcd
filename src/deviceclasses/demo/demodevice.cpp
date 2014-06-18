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

#include "demodevice.hpp"

#include "fnv.hpp"

#include "buttonbehaviour.hpp"
#include "lightbehaviour.hpp"

using namespace p44;


DemoDevice::DemoDevice(DemoDeviceContainer *aClassContainerP) :
  Device((DeviceClassContainer *)aClassContainerP)
{
  // a demo device is a light which shows its dimming value as a string of 0..50 hashes on the console
  // - is a light device
  primaryGroup = group_yellow_light;
  // - use light settings, which include a fully functional scene table
  deviceSettings = DeviceSettingsPtr(new LightDeviceSettings(*this));
  // - create one output with light behaviour
  LightBehaviourPtr l = LightBehaviourPtr(new LightBehaviour(*this));
  // - set default config to act as dimmer with variable ramps
  l->setHardwareOutputConfig(outputFunction_dimmer, usage_undefined, true, -1);
  addBehaviour(l);
  // - hardware is defined, now derive dSUID
	deriveDsUid();
}




void DemoDevice::applyChannelValues(CompletedCB aCompletedCB)
{
  // single channel device, get primary channel (only if it has a changed value)
  ChannelBehaviourPtr ch = getChannelByType(channeltype_default, true);
  if (ch) {
    // This would be the place to implement sending the output value to the hardware
    // For the demo device, we show the output as a bar of 0..64 '#' chars
    // - read the new channel value from the behaviour
    int hwValue = ch->getChannelValue();
    // - display as a bar of hash chars
    string bar;
    while (hwValue>0) {
      // one hash character per 4 output value steps (0..255 = 0..64 hashes)
      bar += '#';
      hwValue -= 4;
    }
    printf("Demo Device Output: %s\n", bar.c_str());
    ch->channelValueApplied(); // confirm having applied the value
  }
  inherited::applyChannelValues(aCompletedCB);
}


void DemoDevice::deriveDsUid()
{
  Fnv64 hash;

  // vDC implementation specific UUID:
  //   UUIDv5 with name = classcontainerinstanceid::SingularDemoDevice
  DsUid vdcNamespace(DSUID_P44VDC_NAMESPACE_UUID);
  string s = classContainerP->deviceClassContainerInstanceIdentifier();
  s += "::SingularDemoDevice";
  dSUID.setNameInSpace(s, vdcNamespace);
}


string DemoDevice::modelName()
{
  return "Demo Output";
}


string DemoDevice::description()
{
  string s = inherited::description();
  string_format_append(s, "- Demo output to console\n");
  return s;
}
