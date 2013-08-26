//
//  sensorbehaviour.hpp
//  vdcd
//
//  Created by Lukas Zeller on 23.08.13.
//  Copyright (c) 2013 plan44.ch. All rights reserved.
//

#ifndef __vdcd__sensorbehaviour__
#define __vdcd__sensorbehaviour__


#include "device.hpp"

using namespace std;

namespace p44 {

  class SensorBehaviour : public DsBehaviour
  {
    typedef DsBehaviour inherited;

  protected:

    /// @name behaviour description, constants or variables
    ///   set by device implementations when adding a Behaviour.
    /// @{
    DsSensorType sensorType; ///< type and physical unit of sensor
    double min; /// minimum value (corresponding to aEngineeringValue==0)
    double max; /// max value
    double resolution; /// change per LSB of sensor engineering value
    MLMicroSeconds updateInterval;
    /// @}

    /// @name persistent settings
    /// @{
    MLMicroSeconds minPushInterval; ///< minimum time between pushes
    /// @}


    /// @name internal volatile state
    /// @{
    double currentValue; ///< current sensor value
    MLMicroSeconds lastUpdate; ///< time of last update from hardware
    MLMicroSeconds lastPush; ///< time of last push
    /// @}


  public:

    /// constructor
    SensorBehaviour(Device &aDevice);

    /// initialisation of hardware-specific constants for this button input
    /// @note this must be called once before the device gets added to the device container. Implementation might
    ///   also derive default values for settings from this information.
    void setHardwareSensorConfig(DsSensorType aType, double aMin, double aMax, double aResolution, MLMicroSeconds aUpdateInterval);

    /// @name interface towards actual device hardware (or simulation)
    /// @{

    /// button action occurred
    /// @param aEngineeringValue the engineering value from the sensor.
    ///   The state value will be adjusted and scaled according to min/max/resolution
    void updateEngineeringValue(long aEngineeringValue);

    /// @}

    /// description of object, mainly for debug and logging
    /// @return textual description of object, may contain LFs
    virtual string description();

  protected:

    /// the behaviour type
    virtual BehaviourType getType() { return behaviour_sensor; };

    // property access implementation for descriptor/settings/states
    virtual int numDescProps();
    virtual const PropertyDescriptor *getDescDescriptor(int aPropIndex);
    virtual int numSettingsProps();
    virtual const PropertyDescriptor *getSettingsDescriptor(int aPropIndex);
    virtual int numStateProps();
    virtual const PropertyDescriptor *getStateDescriptor(int aPropIndex);
    // combined field access for all types of properties
    virtual bool accessField(bool aForWrite, JsonObjectPtr &aPropValue, const PropertyDescriptor &aPropertyDescriptor, int aIndex);

    // persistence implementation
    virtual const char *tableName();
    virtual size_t numFieldDefs();
    virtual const FieldDefinition *getFieldDef(size_t aIndex);
    virtual void loadFromRow(sqlite3pp::query::iterator &aRow, int &aIndex);
    virtual void bindToStatement(sqlite3pp::statement &aStatement, int &aIndex, const char *aParentIdentifier);

  };
  typedef boost::intrusive_ptr<SensorBehaviour> SensorBehaviourPtr;



} // namespace p44

#endif /* defined(__vdcd__sensorbehaviour__) */
