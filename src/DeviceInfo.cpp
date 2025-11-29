#include "DeviceInfo.h"

String DeviceInfo::_deviceName = DEVICE_NAME;
String DeviceInfo::_firmwareVersion = FIRMWARE_VERSION;
String DeviceInfo::_buildDate = BUILD_DATE;
String DeviceInfo::_buildTime = BUILD_TIME;
String DeviceInfo::_buildTimestamp = BUILD_TIMESTAMP;
String DeviceInfo::_deviceVersion = DEVICE_VERSION;
std::vector<String> DeviceInfo::_capabilities = {};
