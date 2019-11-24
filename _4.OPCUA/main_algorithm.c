#include <open62541.h>
#include "main_server.h"

enum EventType {
    Success, Failure
};

static UA_StatusCode
create_event(UA_Server *server, char *message, enum EventType type, UA_NodeId *outId) {
    UA_StatusCode ret = UA_Server_createEvent(server, EventType_id, outId);
    if (ret != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "createEvent failed. StatusCode %s", UA_StatusCode_name(ret));
        return ret;
    }

    UA_DateTime event_time = UA_DateTime_now();
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Time"),
                                         &event_time, &UA_TYPES[UA_TYPES_DATETIME]);

    UA_LocalizedText event_message = UA_LOCALIZEDTEXT("ru-RU", message);
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Message"),
                                         &event_message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);

    UA_UInt16 event_type = type == Success ? 0 : 1;
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Type"),
                                         &event_type, &UA_TYPES[UA_TYPES_UINT16]);
    UA_String event_source_name = UA_STRING("Устройство");
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "SourceName"),
                                         &event_source_name, &UA_TYPES[UA_TYPES_STRING]);
    UA_UInt16 event_severity = type == Failure ? 100 : 10;
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Severity"),
                                         &event_severity, &UA_TYPES[UA_TYPES_UINT16]);
    return UA_STATUSCODE_GOOD;
}

//https://github.com/open62541/open62541/issues/3102
void write_detector_value(UA_Server *server, UA_UInt16 value) {
    UA_NodeId event_id = UA_NODEID_NULL;
    UA_StatusCode ret = create_event(server, "Ошибка состояния запроса", Failure, &event_id);
    if (ret == UA_STATUSCODE_GOOD) {
        ret = UA_Server_triggerEvent(server, event_id,
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                                     NULL, UA_TRUE);
        if (ret != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                           "Triggering event failed. StatusCode %s", UA_StatusCode_name(ret));
        } else {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Создано событие %lu ", (unsigned long) event_id.identifier.numeric);
        }
    }
}

