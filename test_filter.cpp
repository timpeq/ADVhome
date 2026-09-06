#include <ArduinoJson.h>
void test() {
    JsonDocument filter;
    filter["type"] = true;
    filter["id"] = true;
    filter["success"] = true;
    filter["ha_version"] = true;
    
    // For result of get_states
    filter["result"][0]["entity_id"] = true;
    filter["result"][0]["state"] = true;
    filter["result"][0]["attributes"]["friendly_name"] = true;

    // For events
    filter["event"]["event_type"] = true;
    filter["event"]["data"]["entity_id"] = true;
    filter["event"]["data"]["new_state"]["state"] = true;
    filter["event"]["data"]["new_state"]["attributes"]["friendly_name"] = true;
}
