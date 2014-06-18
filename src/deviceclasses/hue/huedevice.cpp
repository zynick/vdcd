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

#include "huedevice.hpp"
#include "huedevicecontainer.hpp"

#include "fnv.hpp"

#include "lightbehaviour.hpp"

using namespace p44;


#pragma mark - HueLightScene


HueLightScene::HueLightScene(SceneDeviceSettings &aSceneDeviceSettings, SceneNo aSceneNo) :
  inherited(aSceneDeviceSettings, aSceneNo)
{
  colorMode = hueColorModeNone;
  XOrHueOrCt = 0;
  YOrSat = 0;
}


#pragma mark - HueLight Scene persistence

const char *HueLightScene::tableName()
{
  return "HueLightScenes";
}

// data field definitions

static const size_t numHueSceneFields = 3;

size_t HueLightScene::numFieldDefs()
{
  return inherited::numFieldDefs()+numHueSceneFields;
}


const FieldDefinition *HueLightScene::getFieldDef(size_t aIndex)
{
  static const FieldDefinition dataDefs[numHueSceneFields] = {
    { "colorMode", SQLITE_INTEGER },
    { "XOrHueOrCt", SQLITE_FLOAT },
    { "YOrSat", SQLITE_FLOAT }
  };
  if (aIndex<inherited::numFieldDefs())
    return inherited::getFieldDef(aIndex);
  aIndex -= inherited::numFieldDefs();
  if (aIndex<numHueSceneFields)
    return &dataDefs[aIndex];
  return NULL;
}



/// load values from passed row
void HueLightScene::loadFromRow(sqlite3pp::query::iterator &aRow, int &aIndex)
{
  inherited::loadFromRow(aRow, aIndex);
  // get the fields
  colorMode = (HueColorMode)aRow->get<int>(aIndex++);
  XOrHueOrCt = aRow->get<double>(aIndex++);
  YOrSat = aRow->get<double>(aIndex++);
}


/// bind values to passed statement
void HueLightScene::bindToStatement(sqlite3pp::statement &aStatement, int &aIndex, const char *aParentIdentifier)
{
  inherited::bindToStatement(aStatement, aIndex, aParentIdentifier);
  // bind the fields
  aStatement.bind(aIndex++, (int)colorMode);
  aStatement.bind(aIndex++, XOrHueOrCt);
  aStatement.bind(aIndex++, YOrSat);
}



#pragma mark - Light scene property access

static char huelightscene_key;

enum {
  colorMode_key,
  hue_key,
  saturation_key,
  X_key,
  Y_key,
  colorTemperature_key,
  numHueLightSceneProperties
};


int HueLightScene::numProps(int aDomain, PropertyDescriptorPtr aParentDescriptor)
{
  return inherited::numProps(aDomain, aParentDescriptor)+numHueLightSceneProperties;
}


PropertyDescriptorPtr HueLightScene::getDescriptorByIndex(int aPropIndex, int aDomain, PropertyDescriptorPtr aParentDescriptor)
{
  static const PropertyDescription properties[numHueLightSceneProperties] = {
    { "x-p44-colorMode", apivalue_uint64, colorMode_key, OKEY(huelightscene_key) },
    { "x-p44-hue", apivalue_double, hue_key, OKEY(huelightscene_key) },
    { "x-p44-saturation", apivalue_double, saturation_key, OKEY(huelightscene_key) },
    { "x-p44-X", apivalue_double, X_key, OKEY(huelightscene_key) },
    { "x-p44-Y", apivalue_double, Y_key, OKEY(huelightscene_key) },
    { "x-p44-colorTemperature", apivalue_double, colorTemperature_key, OKEY(huelightscene_key) },
  };
  int n = inherited::numProps(aDomain, aParentDescriptor);
  if (aPropIndex<n)
    return inherited::getDescriptorByIndex(aPropIndex, aDomain, aParentDescriptor); // base class' property
  aPropIndex -= n; // rebase to 0 for my own first property
  return PropertyDescriptorPtr(new StaticPropertyDescriptor(&properties[aPropIndex], aParentDescriptor));
}


bool HueLightScene::accessField(PropertyAccessMode aMode, ApiValuePtr aPropValue, PropertyDescriptorPtr aPropertyDescriptor)
{
  if (aPropertyDescriptor->hasObjectKey(huelightscene_key)) {
    if (aMode==access_read) {
      // read properties
      switch (aPropertyDescriptor->fieldKey()) {
        case colorMode_key:
          aPropValue->setUint16Value(colorMode);
          return true;
        case hue_key:
          if (colorMode==hueColorModeHueSaturation) aPropValue->setDoubleValue(XOrHueOrCt); else aPropValue.reset();
          return true;
        case saturation_key:
          if (colorMode==hueColorModeHueSaturation) aPropValue->setDoubleValue(YOrSat); else aPropValue.reset();
          return true;
        case X_key:
          if (colorMode==hueColorModeXY) aPropValue->setDoubleValue(XOrHueOrCt); else aPropValue.reset();
          return true;
        case Y_key:
          if (colorMode==hueColorModeXY) aPropValue->setDoubleValue(YOrSat); else aPropValue.reset();
          return true;
        case colorTemperature_key:
          if (colorMode==hueColorModeCt) aPropValue->setDoubleValue(XOrHueOrCt); else aPropValue.reset();
          return true;
      }
    }
    else {
      // write properties
      switch (aPropertyDescriptor->fieldKey()) {
        case colorMode_key:
          colorMode = (HueColorMode)aPropValue->int32Value();
          markDirty();
          return true;
        case hue_key:
          colorMode = hueColorModeHueSaturation;
          goto setXOrHueOrCt;
        case X_key:
          colorMode = hueColorModeXY;
          goto setXOrHueOrCt;
        case colorTemperature_key:
          colorMode = hueColorModeCt;
        setXOrHueOrCt:
          XOrHueOrCt = aPropValue->doubleValue();
          markDirty();
          return true;
        case saturation_key:
          colorMode = hueColorModeHueSaturation;
          goto setYOrSat;
        case Y_key:
          colorMode = hueColorModeXY;
        setYOrSat:
          YOrSat = aPropValue->doubleValue();
          markDirty();
          return true;
      }
    }
  }
  return inherited::accessField(aMode, aPropValue, aPropertyDescriptor);
}


#pragma mark - default scene values


void HueLightScene::setDefaultSceneValues(SceneNo aSceneNo)
{
  // init default brightness
  inherited::setDefaultSceneValues(aSceneNo);
  // init hue specifics
  // TODO: maybe, more elaborated defaults
  colorMode = hueColorModeNone; // no stored color information
}



#pragma mark - HueDeviceSettings with default light scenes factory


HueDeviceSettings::HueDeviceSettings(Device &aDevice) :
  inherited(aDevice)
{
};


DsScenePtr HueDeviceSettings::newDefaultScene(SceneNo aSceneNo)
{
  HueLightScenePtr lightScene = HueLightScenePtr(new HueLightScene(*this, aSceneNo));
  lightScene->setDefaultSceneValues(aSceneNo);
  // return it
  return lightScene;
}


#pragma mark - HueLightBehaviour


HueLightBehaviour::HueLightBehaviour(Device &aDevice) :
  LightBehaviour(aDevice)
{
}


void HueLightBehaviour::recallScene(LightScenePtr aLightScene)
{
  HueLightScenePtr hueScene = boost::dynamic_pointer_cast<HueLightScene>(aLightScene);
  if (hueScene) {
    // prepare next color values in device
    HueDevice *devP = dynamic_cast<HueDevice *>(&device);
    if (devP) {
      devP->pendingColorScene = hueScene;
      // we need an output update, even if main output value (brightness) has not changed in new scene
      devP->getChannelByType(channeltype_brightness)->setChannelUpdatePending();
    }
  }
  // let base class update logical brightness, which will in turn update the output, which will then
  // catch the colors from pendingColorScene
  inherited::recallScene(aLightScene);
}



void HueLightBehaviour::performSceneActions(DsScenePtr aScene)
{
  // we can only handle light scenes
  LightScenePtr lightScene = boost::dynamic_pointer_cast<LightScene>(aScene);
  if (lightScene && lightScene->effect==scene_effect_alert) {
    // alert
    HueDevice *devP = dynamic_cast<HueDevice *>(&device);
    if (devP) {
      // Three breathe cycles
      devP->alertHandler(3);
    }
  }
}





// capture scene
void HueLightBehaviour::captureScene(DsScenePtr aScene, DoneCB aDoneCB)
{
  HueLightScenePtr hueScene = boost::dynamic_pointer_cast<HueLightScene>(aScene);
  if (hueScene) {
    // query light attributes and state
    HueDevice *devP = dynamic_cast<HueDevice *>(&device);
    if (devP) {
      string url = string_format("/lights/%s", devP->lightID.c_str());
      devP->hueComm().apiQuery(url.c_str(), boost::bind(&HueLightBehaviour::sceneColorsReceived, this, hueScene, aDoneCB, _1, _2));
    }
  }
}


void HueLightBehaviour::sceneColorsReceived(HueLightScenePtr aHueScene, DoneCB aDoneCB, JsonObjectPtr aDeviceInfo, ErrorPtr aError)
{
  if (Error::isOK(aError)) {
    JsonObjectPtr o;
    // get current color settings
    JsonObjectPtr state = aDeviceInfo->get("state");
    HueColorMode newMode = hueColorModeNone;
    double newXOrHueOrCt = 0;
    double newYOrSat = 0;
    if (state) {
      o = state->get("colormode");
      if (o) {
        string mode = o->stringValue();
        if (mode=="hs") {
          newMode = hueColorModeHueSaturation;
          o = state->get("hue");
          if (o) newXOrHueOrCt = o->int32Value();
          o = state->get("sat");
          if (o) newYOrSat = o->int32Value();
        }
        else if (mode=="xy") {
          newMode = hueColorModeXY;
          o = state->get("xy");
          if (o) {
            JsonObjectPtr e = o->arrayGet(0);
            if (e) newXOrHueOrCt = e->doubleValue();
            e = o->arrayGet(1);
            if (e) newYOrSat = e->doubleValue();
          }
        }
        else if (mode=="ct") {
          newMode = hueColorModeCt;
          o = state->get("ct");
          if (o) newXOrHueOrCt = o->int32Value();
        }
      }
      // check color updates
      if (newMode!=aHueScene->colorMode) { aHueScene->colorMode = newMode; aHueScene->markDirty(); }
      if (newXOrHueOrCt!=aHueScene->XOrHueOrCt) { aHueScene->XOrHueOrCt = newXOrHueOrCt; aHueScene->markDirty(); }
      if (newXOrHueOrCt!=aHueScene->YOrSat) { aHueScene->YOrSat = newYOrSat; aHueScene->markDirty(); }
      // in any case, update current output value (cache in outputbehaviour) as well, so base class will capture a current level
      Brightness bri = 0; // assume off
      o = state->get("on");
      if (o && o->boolValue()) {
        // lamp is on, get brightness
        o = state->get("bri");
        if (o) {
          bri = (Brightness)o->int32Value();
        }
      }
      getChannelByType(channeltype_brightness)->initChannelValue(bri);
    }
  }
  // anyway, let base class capture brightness
  inherited::captureScene(aHueScene, aDoneCB);
}


#pragma mark - HueDevice


HueDevice::HueDevice(HueDeviceContainer *aClassContainerP, const string &aLightID) :
  inherited(aClassContainerP),
  lightID(aLightID)
{
  // hue devices are lights
  setPrimaryGroup(group_yellow_light);
  // derive the dSUID
  deriveDsUid();
  // use hue light settings, which include a extended scene table
  deviceSettings = DeviceSettingsPtr(new HueDeviceSettings(*this));
  // set the behaviour
  HueLightBehaviourPtr l = HueLightBehaviourPtr(new HueLightBehaviour(*this));
  l->setHardwareOutputConfig(outputFunction_dimmer, usage_undefined, true, 8.5); // hue lights are always dimmable, one hue = 8.5W
  l->setHardwareName(string_format("hue light #%s", lightID.c_str()));
  l->initBrightnessParams(1,255); // brightness range is 1..255
  addBehaviour(l);
}



HueDeviceContainer &HueDevice::hueDeviceContainer()
{
  return *(static_cast<HueDeviceContainer *>(classContainerP));
}


HueComm &HueDevice::hueComm()
{
  return (static_cast<HueDeviceContainer *>(classContainerP))->hueComm;
}



void HueDevice::setName(const string &aName)
{
  string oldname = getName();
  inherited::setName(aName);
  if (getName()!=oldname) {
    // really changed, propagate to hue
    JsonObjectPtr params = JsonObject::newObj();
    params->add("name", JsonObject::newString(getName()));
    string url = string_format("/lights/%s", lightID.c_str());
    hueComm().apiAction(httpMethodPUT, url.c_str(), params, NULL);
  }
}




void HueDevice::initializeDevice(CompletedCB aCompletedCB, bool aFactoryReset)
{
  // query light attributes and state
  string url = string_format("/lights/%s", lightID.c_str());
  hueComm().apiQuery(url.c_str(), boost::bind(&HueDevice::deviceStateReceived, this, aCompletedCB, aFactoryReset, _1, _2));
}


void HueDevice::deviceStateReceived(CompletedCB aCompletedCB, bool aFactoryReset, JsonObjectPtr aDeviceInfo, ErrorPtr aError)
{
  if (Error::isOK(aError) && aDeviceInfo) {
    JsonObjectPtr o;
    // get model name from device
    hueModel.clear();
    o = aDeviceInfo->get("type");
    if (o) {
      hueModel = o->stringValue();
    }
    o = aDeviceInfo->get("modelid");
    if (o) {
      hueModel += ": " + o->stringValue();
    }
    // get current brightness
    JsonObjectPtr state = aDeviceInfo->get("state");
    Brightness bri = 0;
    if (state) {
      o = state->get("on");
      if (o && o->boolValue()) {
        // lamp is on
        bri = 255; // default to full brightness
        o = state->get("bri");
        if (o) {
          bri = o->int32Value();
        }
        // set current brightness
        output->getChannelByType(channeltype_brightness)->initChannelValue(bri);
      }
    }
  }
  // let superclass initialize as well
  inherited::initializeDevice(aCompletedCB, aFactoryReset);
}



void HueDevice::checkPresence(PresenceCB aPresenceResultHandler)
{
  // query the device
  string url = string_format("/lights/%s", lightID.c_str());
  hueComm().apiQuery(url.c_str(), boost::bind(&HueDevice::presenceStateReceived, this, aPresenceResultHandler, _1, _2));
}



void HueDevice::presenceStateReceived(PresenceCB aPresenceResultHandler, JsonObjectPtr aDeviceInfo, ErrorPtr aError)
{
  bool reachable = false;
  if (Error::isOK(aError) && aDeviceInfo) {
    JsonObjectPtr state = aDeviceInfo->get("state");
    if (state) {
      // Note: 2012 hue bridge firmware always returns 1 for this.
      JsonObjectPtr o = state->get("reachable");
      reachable = o && o->boolValue();
    }
  }
  aPresenceResultHandler(reachable);
}



void HueDevice::identifyToUser()
{
  // Four breathe cycles
  alertHandler(4);
}


void HueDevice::alertHandler(int aLeftCycles)
{
  // do one alert
  string url = string_format("/lights/%s/state", lightID.c_str());
  JsonObjectPtr newState = JsonObject::newObj();
  newState->add("alert", JsonObject::newString("select"));
  hueComm().apiAction(httpMethodPUT, url.c_str(), newState, NULL);
  // schedule next if any left
  if (--aLeftCycles>0) {
    MainLoop::currentMainLoop().executeOnce(boost::bind(&HueDevice::alertHandler, this, aLeftCycles), 1*Second);
  }
}



void HueDevice::disconnect(bool aForgetParams, DisconnectCB aDisconnectResultHandler)
{
  checkPresence(boost::bind(&HueDevice::disconnectableHandler, this, aForgetParams, aDisconnectResultHandler, _1));
}


void HueDevice::disconnectableHandler(bool aForgetParams, DisconnectCB aDisconnectResultHandler, bool aPresent)
{
  if (!aPresent) {
    // call inherited disconnect
    inherited::disconnect(aForgetParams, aDisconnectResultHandler);
  }
  else {
    // not disconnectable
    if (aDisconnectResultHandler) {
      aDisconnectResultHandler(false);
    }
  }
}



void HueDevice::applyChannelValues(CompletedCB aCompletedCB)
{
  // check if any channel has changed at all
  bool needsUpdate = false;
  for (int i=0; i<numChannels(); i++) {
    if (getChannelByIndex(i, true)) {
      // channel needs update
      needsUpdate = true;
      break; // no more checking needed, need device level update anyway
    }
  }
  if (!needsUpdate) {
    // NOP
    aCompletedCB(ErrorPtr());
    return;
  }
  // single channel device, get primary channel
  ChannelBehaviourPtr ch = getChannelByType(channeltype_brightness, false); // %%% for now, always update, even if brightness has not changed
  if (ch) {
    string url = string_format("/lights/%s/state", lightID.c_str());
    JsonObjectPtr newState = JsonObject::newObj();
    Brightness b = ch->getChannelValue();
    if (b==0) {
      // light off
      newState->add("on", JsonObject::newBool(false));
    }
    else {
      // light on
      newState->add("on", JsonObject::newBool(true));
      newState->add("bri", JsonObject::newInt32(b)); // 0..255
      // add color in case it was set (by scene call)
      if (pendingColorScene) {
        const char *cm;
        switch (pendingColorScene->colorMode) {
          case hueColorModeHueSaturation: {
            newState->add("hue", JsonObject::newInt32(pendingColorScene->XOrHueOrCt));
            newState->add("sat", JsonObject::newInt32(pendingColorScene->YOrSat));
            cm = "hs";
            break;
          }
          case hueColorModeXY: {
            JsonObjectPtr xyArr = JsonObject::newArray();
            xyArr->arrayAppend(JsonObject::newDouble(pendingColorScene->XOrHueOrCt));
            xyArr->arrayAppend(JsonObject::newDouble(pendingColorScene->YOrSat));
            newState->add("xy", xyArr);
            cm = "xy";
            break;
          }
          case hueColorModeCt: {
            newState->add("ct", JsonObject::newInt32(pendingColorScene->XOrHueOrCt));
            cm = "ct"; break;
          }
          default:
            cm = NULL;
        }
        if (cm) {
          // colormode is read-only, bridge derives it from presence of ct/xy/hue+sat
          //newState->add("colormode", JsonObject::newString(cm));
        }
        // done
        pendingColorScene.reset();
      }
    }
    // for on and off, set transition time (1/10 second resolution)
    newState->add("transitiontime", JsonObject::newInt64(ch->transitionTimeToNewValue()/(100*MilliSecond)));
    LOG(LOG_INFO, "hue device %s: setting new brightness = %0.0f\n", shortDesc().c_str(), b);
    hueComm().apiAction(httpMethodPUT, url.c_str(), newState, boost::bind(&HueDevice::outputChangeSent, this, aCompletedCB, ch, _2));
  }
}



void HueDevice::outputChangeSent(CompletedCB aCompletedCB, ChannelBehaviourPtr aChannelBehaviour, ErrorPtr aError)
{
  if (Error::isOK(aError)) {
    aChannelBehaviour->channelValueApplied(); // confirm having applied the value
  }
  // confirm
  aCompletedCB(aError);
}



void HueDevice::deriveDsUid()
{
  // NOTE: lightID is not exactly a stable ID. But the hue API does not provide anything better at this time
  // vDC implementation specific UUID:
  //   UUIDv5 with name = classcontainerinstanceid::huelightid
  DsUid vdcNamespace(DSUID_P44VDC_NAMESPACE_UUID);
  string s = classContainerP->deviceClassContainerInstanceIdentifier();
  s += "::" + lightID;
  dSUID.setNameInSpace(s, vdcNamespace);
}


string HueDevice::description()
{
  string s = inherited::description();
  return s;
}
