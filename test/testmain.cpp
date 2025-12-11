#include "unity.h"
#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * We're going to design a JSON packet class that will be used to send/receive
 * JSON packets, but with the ability to accumulate multiple packets into a single
 * JSON object, so that the client can send/receive data in amounts larger than
 * the limit of the transport layer (ie BLE or WiFi).
 * 
 * We want the following features:
 * - Ability to receive a JSON packet that is just 1, it contains everything and is not a multi-part transmission.
 * - Ability to receive a JSON packet that is part of a multi-part transmission.
 * 
 * The packet will be a JSON object with the following fields:
 * "_s": sequence number - valid values are 1-<inf>
 * "_n": total number of packets in the multi-part transmission - valid values are 1-<inf>
 * "_e": end of transmission - valid values are true or false
 * "_p": payload - the payload of the packet
 */

/**
 * Example of a multi-part transmission, trying to send a large JSON command
 * Example command to send:
 * {
 *     "type": "effect",
 * }
 * 
 * 
 * 
 * {
 *     "_s": 1,
 *     "_n": 3,
 *     "_e": false,
 *     "_p": "{\"type\":\"effect\",\"name\":\"test\",\"params\":{\"color\":\"red\",\"brightness\":100}}"
 * }
 * {
 *     "_s": 2,
 *     "_e": false,
 *     "_p": "{\"type\":\"effect\",\"name\":\"test\",\"params\":{\"color\":\"green\",\"brightness\":100}}"
 * }
 * {
 *     "_s": 3,
 *     "_e": true,
 *     "_p": "{\"type\":\"effect\",\"name\":\"test\",\"params\":{\"color\":\"blue\",\"brightness\":100}}"
 * }
 */

class JsonPacket {

};



void setUp(void)
{
    // set stuff up here
}

void tearDown(void)
{
    // clean stuff up here
}

void test_function_should_doBlahAndBlah(void)
{
    // test stuff
    // Serial.println("test_function_should_doBlahAndBlah");
    TEST_ASSERT_EQUAL(1, 2);
}

void test_function_should_doAlsoDoBlah(void)
{
    // more test stuff
    // Serial.println("test_function_should_doAlsoDoBlah");
    TEST_ASSERT_NOT_EQUAL(2, 1);
}

int runUnityTests(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_function_should_doBlahAndBlah);
    RUN_TEST(test_function_should_doAlsoDoBlah);
    UNITY_END();
    return 0;
}

// WARNING!!! PLEASE REMOVE UNNECESSARY MAIN IMPLEMENTATIONS //

/**
  * For native dev-platform or for some embedded frameworks
  */
int main(void)
{
    return runUnityTests();
}

/**
  * For Arduino framework
  */
void setup()
{
    // Wait ~2 seconds before the Unity test runner
    // establishes connection with a board Serial interface

    runUnityTests();
}
void loop() {}

/**
  * For ESP-IDF framework
  */
void app_main()
{
    runUnityTests();
}