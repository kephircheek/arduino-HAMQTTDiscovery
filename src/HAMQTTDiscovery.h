// HAMQTTDiscovery
// Copyright 2022 Ilia Lazarev
// Licensed under the Apache License, Version 2.0k

#ifndef HAMQTTDiscovery_h
#define HAMQTTDiscovery_h

#include "Arduino.h"
#include <ArduinoJson.h>

// mac2String is a simple tool to convert mac numbers to string
String mac2String(const uint8_t* mac) {
  String s;
  for (uint8_t i = 0; i < 6; ++i)
  {
    char buf[3];
    sprintf(buf, "%02X", mac[i]);
    s += buf;
    if (i < 5) s += ':';
  }
  return s;
}


static const char* ONLINE = "online";
static const char* ON = "ON";
static const char* OFF = "OFF";

/* Device registry
*/
class HADevice {
    public:
        HADevice(const char* name, const uint8_t* mac, const char* sw) {
            _name = name;
            _mac = mac;
            _sw = sw;
            _device = NULL;
        }
        HADevice(const char* name, HADevice* device) {
            _name = name;
            _device = device;
        }
        String id() {
            if (_device == NULL) {
                return String(_name);
            } else {
                String s;
                s += _device->id();
                s += "_";
                s += _name;
                return s;
            }
        }
        String baseTopic() {
            String s;
            s += "homeassistant/";
            s += id();
            return s;

        }
        String stateTopic() {
            if (_device != NULL) {
                return _device->stateTopic();
            }
            return baseTopic() + "/state";
        }
        String availabilityTopic() {
            if (_device != NULL) {
                return _device->availabilityTopic();
            }
            return baseTopic() + "/status";
        }
        void fillConfig(JsonObject config) {
            config["name"] = _name;
            if (_device == NULL) {
                config["connections"][0][0] = "mac";
                config["connections"][0][1] = mac2String(_mac);
                config["sw_version"] = _sw;
            } else {
                config["via_device"] = _device->id();
            }
            config["identifiers"][0] = id();
        }
    private:
        const char* _name;
        const uint8_t* _mac;
        const char* _sw;
        HADevice* _device;


};

/* Entity registry
*/
class HAEntity {
    public:
        virtual const char* kind();
        virtual String name() {
            return String(_name);
        }
        virtual String id() {
            String s;
            s += _device->id();
            s += "_";
            s += kind();
            s += "_";
            s += name();
            return s;
        };
        String baseTopic() {
            const char* entityKind = kind();
            String s;
            s += "homeassistant/";
            s += entityKind;
            s += "/";
            s += id();
            return s;
        }
        virtual String stateValueTemplate() {
            String uniqueID = id();
            String s;
            s += "{{ value_json.";
            s += id();
            s += " }}";
            return s;
        }
        String configTopic() {
            String s = baseTopic();
            s += "/config";
            return s;
        }
        void fillCommonConfig(JsonObject obj) {
            String uniqueID = id();
            obj["uniq_id"] = uniqueID;
            obj["object_id"] = uniqueID;
            obj["availability"]["topic"] = _device->availabilityTopic();
            obj["state_topic"] = _device->stateTopic();
            JsonObject deviceObj = obj.createNestedObject("device");
            _device->fillConfig(deviceObj);
        }
        virtual void fillConfig(JsonObject obj);

    protected:
        const char* _name;
        HADevice* _device;
};


class HALight : public HAEntity {
    public:
        HALight(const char* name, HADevice* device) {
            _name = name;
            _device = device;
        }
        const char* kind() {
            return "light";
        }
        String commandTopic() {
            String s = baseTopic();
            s += "/set";
            return s;
        }
        void fillConfig(JsonObject obj) {
            fillCommonConfig(obj);
            obj["command_topic"] = commandTopic();
            obj["state_value_template"] = stateValueTemplate();
        }
};


class HASensor : public HAEntity {
    public:
        enum Class {
            Temperature,
            Humidity
        };
        HASensor(Class cls, HADevice* device) {
            _name = NULL;
            _cls = cls;
            _device = device;
        }
        HASensor(const char* name, Class cls, HADevice* device) {
            _name = name;
            _cls = cls;
            _device = device;
        }
        const char* kind() {
            return "sensor";
        }
        const char* classStr() {
            switch (_cls) {
                case Class::Temperature: return "temperature";
                case Class::Humidity: return "humidity";
            }
        }
        String name() {
            String s;
            if (_name == NULL) {
                return classStr();
            } else {
                s += _name;
                s += "_";
                s += classStr();
            }
        }
        String id() {
            const char* cls = classStr();
            String s;
            s += _device->id();
            s += "_";
            s += _name;
            s += cls;
            return s;
        };
        String stateValueTemplate() {
            String uniqueID = id();
            String s;
            s += "{{ value_json.";
            s += id();
            s += " | round(";
            s += String(1);
            s += ")";
            s += " }}";
            return s;
        }
        void fillConfig(JsonObject obj) {
            fillCommonConfig(obj);
            obj["device_class"] = classStr();
            obj["value_template"] = stateValueTemplate();
        }

    private:
        Class _cls;
        String _valueTemplate;
};

#endif
