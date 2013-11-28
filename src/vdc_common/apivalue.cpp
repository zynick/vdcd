//
//  apivalue.cpp
//  vdcd
//
//  Created by Lukas Zeller on 27.11.13.
//  Copyright (c) 2013 plan44.ch. All rights reserved.
//

#include "apivalue.h"


using namespace p44;


#pragma mark - ApiValue


ApiValue::ApiValue() :
  objectType(apivalue_null)
{
}



bool ApiValue::isType(ApiValueType aObjectType)
{
  return (objectType==aObjectType);
}


ApiValueType ApiValue::getType()
{
  return objectType;
}


void ApiValue::setType(ApiValueType aType)
{
  // base class: just set type
  if (aType!=objectType) {
    objectType = aType;
    // type has changed, make sure internals are cleared
    clear();
  }
}


int ApiValue::arrayLength()
{
  return 0;
}


void ApiValue::clear()
{
  switch (objectType) {
    // "Zero" simple values
    case apivalue_bool:
      setBoolValue(false);
      break;
    case apivalue_int64:
    case apivalue_uint64:
      setUint64Value(0);
      break;
    case apivalue_double:
      setDoubleValue(0);
      break;
    case apivalue_string:
      setStringValue("");
      break;
    // stuctured values need to be handled in derived class
    default:
      break;
  }
}



// getting and setting as string (for all basic types)

string ApiValue::stringValue()
{
  switch (objectType) {
    case apivalue_bool:
      return boolValue() ? "true" : "false";
    case apivalue_int64:
      return string_format("%ld", int64Value());
    case apivalue_uint64:
      return string_format("%ud", uint64Value());
    case apivalue_double:
      return string_format("%f", doubleValue());
    case apivalue_object:
      return "<object>";
    case apivalue_array:
      return "<array>";
    case apivalue_null:
    case apivalue_string: // if actual type is string, derived class should have delivered it
    default:
      return "";
  }
}


bool ApiValue::setStringValue(const string &aString)
{
  int n;
  switch (objectType) {
    case apivalue_bool: {
      string s = lowerCase(aString);
      setBoolValue(s.length()>0 && s!="false" && s!="0" && s!="no");
      return true;
    }
    case apivalue_int64: {
      int64_t intVal;
      n = sscanf(aString.c_str(), "%lld", &intVal);
      if (n==1) setInt64Value(intVal);
      return n==1;
    }
    case apivalue_uint64: {
      uint64_t uintVal;
      n = sscanf(aString.c_str(), "%llu", &uintVal);
      if (n==1) setUint64Value(uintVal);
      return n==1;
    }
    case apivalue_double: {
      double doubleVal;
      n = sscanf(aString.c_str(), "%lf", &doubleVal);
      if (n==1) setDoubleValue(doubleVal);
      return n==1;
    }
    default:
      break;
  }
  // cannot set as string
  return false;
}


// factory methods
ApiValuePtr ApiValue::newInt64(int64_t aInt64)
{
  ApiValuePtr newVal = newValue(apivalue_int64);
  newVal->setInt64Value(aInt64);
  return newVal;
}


ApiValuePtr ApiValue::newUint64(uint64_t aUint64)
{
  ApiValuePtr newVal = newValue(apivalue_uint64);
  newVal->setUint64Value(aUint64);
  return newVal;
}


ApiValuePtr ApiValue::newDouble(double aDouble)
{
  ApiValuePtr newVal = newValue(apivalue_double);
  newVal->setDoubleValue(apivalue_double);
  return newVal;
}


ApiValuePtr ApiValue::newBool(bool aBool)
{
  ApiValuePtr newVal = newValue(apivalue_bool);
  newVal->setBoolValue(aBool);
  return newVal;
}


ApiValuePtr ApiValue::newString(const char *aString)
{
  if (!aString) aString = "";
  return newString(string(aString));
}


ApiValuePtr ApiValue::newString(const string &aString)
{
  ApiValuePtr newVal = newValue(apivalue_string);
  newVal->setStringValue(aString);
  return newVal;
}


ApiValuePtr ApiValue::newObject()
{
  return newValue(apivalue_object);
}


ApiValuePtr ApiValue::newArray()
{
  return newValue(apivalue_array);
}




// get in different int types
uint8_t ApiValue::uint8Value()
{
  return uint64Value() & 0xFF;
}

uint16_t ApiValue::uint16Value()
{
  return uint64Value() & 0xFFFF;
}

uint32_t ApiValue::uint32Value()
{
  return uint64Value() & 0xFFFFFFFF;
}


int8_t ApiValue::int8Value()
{
  return (int8_t)int64Value();
}


int16_t ApiValue::int16Value()
{
  return (int16_t)int64Value();
}


int32_t ApiValue::int32Value()
{
  return (int32_t)int64Value();
}


void ApiValue::setUint8Value(uint8_t aUint8)
{
  setUint64Value(aUint8);
}

void ApiValue::setUint16Value(uint16_t aUint16)
{
  setUint64Value(aUint16);
}

void ApiValue::setUint32Value(uint32_t aUint32)
{
  setUint64Value(aUint32);
}

void ApiValue::setInt8Value(int8_t aInt8)
{
  setInt64Value(aInt8);
}

void ApiValue::setInt16Value(int16_t aInt16)
{
  setInt64Value(aInt16);
}

void ApiValue::setInt32Value(int32_t aInt32)
{
  setInt64Value(aInt32);
}




// convenience utilities

size_t ApiValue::stringLength()
{
  return stringValue().length();
}


bool ApiValue::setStringValue(const char *aCString)
{
  return setStringValue(aCString ? string(aCString) : "");
}


bool ApiValue::setStringValue(const char *aCStr, size_t aLen)
{
  string s;
  if (aCStr) s.assign(aCStr, aLen);
  return setStringValue(s);
}


const char *ApiValue::c_strValue()
{
  return stringValue().c_str();
}


bool ApiValue::isNull()
{
  return getType()==apivalue_null;
}


void ApiValue::setNull()
{
  setType(apivalue_null);
}


string ApiValue::lowercaseStringValue()
{
  return lowerCase(stringValue());
}



#pragma mark - JsonApiValue

JsonApiValue::JsonApiValue()
{
}


ApiValuePtr JsonApiValue::newValue(ApiValueType aObjectType)
{
  ApiValuePtr newVal = ApiValuePtr(new JsonApiValue);
  newVal->setType(aObjectType);
  return newVal;
}



void JsonApiValue::clear()
{
  switch (getType()) {
    case apivalue_object:
      // just assign new object an forget old one
      jsonObj = JsonObject::newObj();
      break;
    case apivalue_array:
      jsonObj = JsonObject::newArray();
      break;
    // for unstuctured values, the json obj will be created on assign, so clear it now
    default:
      jsonObj.reset();
      break;
  }
}


bool JsonApiValue::setStringValue(const string &aString)
{
  if (getType()==apivalue_string) {
    jsonObj = JsonObject::newString(aString, false);
    return true;
  }
  else
    return inherited::setStringValue(aString);
};




JsonApiValue::JsonApiValue(JsonObjectPtr aWithObject)
{
  jsonObj = aWithObject;
  // derive type
  if (!jsonObj) {
    setType(apivalue_null);
  }
  else {
    switch (jsonObj->type()) {
      case json_type_boolean: objectType = apivalue_bool; break;
      case json_type_double: objectType = apivalue_double; break;
      case json_type_int: objectType = apivalue_int64; break;
      case json_type_object: objectType = apivalue_object; break;
      case json_type_array: objectType = apivalue_array; break;
      case json_type_string: objectType = apivalue_string; break;
      case json_type_null:
      default:
        setType(apivalue_null);
        break;
    }
  }
}


ApiValuePtr JsonApiValue::newValueFromJson(JsonObjectPtr aJsonObject)
{
  return ApiValuePtr(new JsonApiValue(aJsonObject));
}




#pragma mark - StandaloneApiValue


