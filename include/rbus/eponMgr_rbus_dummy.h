/**
 * @file eponMgr_rbus_dummy.h
 * @brief Dummy RBUS implementation for local testing
 * 
 * This header provides dummy RBUS types and functions that match the real
 * RBUS API signatures for local development and testing without requiring
 * actual RBUS libraries.
 * 
 * To switch to real RBUS, simply remove the USE_DUMMY_RBUS define and
 * link with -lrbus.
 */

#ifndef EPON_MGR_RBUS_DUMMY_H
#define EPON_MGR_RBUS_DUMMY_H

#ifdef USE_DUMMY_RBUS

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * RBUS Types (matching real RBUS API)
 * ========================================================================= */

/**
 * @brief RBUS handle type
 */
struct _rbusHandle;
typedef struct _rbusHandle* rbusHandle_t;

/**
 * @brief RBUS error codes
 */
typedef enum _rbusError {
    RBUS_ERROR_SUCCESS = 0,
    RBUS_ERROR_BUS_ERROR = 1,
    RBUS_ERROR_INVALID_INPUT,
    RBUS_ERROR_NOT_INITIALIZED,
    RBUS_ERROR_OUT_OF_RESOURCES,
    RBUS_ERROR_DESTINATION_NOT_FOUND,
    RBUS_ERROR_ACCESS_NOT_ALLOWED,
    RBUS_ERROR_INVALID_OPERATION,
    RBUS_ERROR_INVALID_EVENT,
    RBUS_ERROR_ELEMENT_DOES_NOT_EXIST,
    RBUS_ERROR_ELEMENT_NAME_DUPLICATE,
    RBUS_ERROR_ELEMENT_NAME_MISSING,
    RBUS_ERROR_COMPONENT_NAME_DUPLICATE,
    RBUS_ERROR_COMPONENT_DOES_NOT_EXIST,
    RBUS_ERROR_COMPONENT_NAME_MISSING,
    RBUS_ERROR_INVALID_HANDLE,
    RBUS_ERROR_SESSION_ALREADY_EXIST,
    RBUS_ERROR_SUBSCRIPTION_ALREADY_EXIST,
    RBUS_ERROR_ASYNC_RESPONSE,
    RBUS_ERROR_INVALID_METHOD,
    RBUS_ERROR_NOSUBSCRIBERS,
    RBUS_ERROR_TIMEOUT
} rbusError_t;

/**
 * @brief RBUS element types
 */
typedef enum _rbusElementType {
    RBUS_ELEMENT_TYPE_PROPERTY,
    RBUS_ELEMENT_TYPE_TABLE,
    RBUS_ELEMENT_TYPE_EVENT,
    RBUS_ELEMENT_TYPE_METHOD
} rbusElementType_t;

/**
 * @brief Forward declarations for RBUS types
 */
typedef struct _rbusProperty* rbusProperty_t;
typedef struct _rbusValue* rbusValue_t;
typedef struct _rbusObject* rbusObject_t;

/**
 * @brief Event filter (opaque for dummy)
 */
typedef void* rbusFilter_t;

/**
 * @brief Async method handle (opaque for dummy)
 */
typedef void* rbusMethodAsyncHandle_t;

/**
 * @brief Set options (opaque for dummy)
 */
typedef void* rbusSetOptions_t;

/**
 * @brief Event subscription action
 */
typedef enum _rbusEventSubAction {
    RBUS_EVENT_ACTION_SUBSCRIBE = 0,
    RBUS_EVENT_ACTION_UNSUBSCRIBE
} rbusEventSubAction_t;

/**
 * @brief GET handler options
 */
typedef struct _rbusGetHandlerOptions {
    uint32_t timeout;
} rbusGetHandlerOptions_t;

/**
 * @brief SET handler options
 */
typedef struct _rbusSetHandlerOptions {
    bool commit;
} rbusSetHandlerOptions_t;

/**
 * @brief Callback function types
 */
typedef rbusError_t (*rbusGetHandler_t)(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
typedef rbusError_t (*rbusSetHandler_t)(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts);
typedef rbusError_t (*rbusTableAddRowHandler_t)(rbusHandle_t handle, char const* tableName, char const* aliasName, uint32_t* instNum);
typedef rbusError_t (*rbusTableRemoveRowHandler_t)(rbusHandle_t handle, char const* rowName);
typedef rbusError_t (*rbusEventSubHandler_t)(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish);
typedef rbusError_t (*rbusMethodHandler_t)(rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle);

/**
 * @brief Callback table for data elements
 */
typedef struct _rbusCallbackTable {
    rbusGetHandler_t getHandler;
    rbusSetHandler_t setHandler;
    rbusTableAddRowHandler_t tableAddRowHandler;
    rbusTableRemoveRowHandler_t tableRemoveRowHandler;
    rbusEventSubHandler_t eventSubHandler;
    rbusMethodHandler_t methodHandler;
} rbusCallbackTable_t;

/**
 * @brief Data element structure
 */
typedef struct _rbusDataElement {
    char* name;
    rbusElementType_t type;
    rbusCallbackTable_t cbTable;
} rbusDataElement_t;

/* ============================================================================
 * RBUS API Functions (dummy implementations)
 * ========================================================================= */

/**
 * @brief Open RBUS connection (dummy)
 */
rbusError_t rbus_open(rbusHandle_t* handle, char const* componentName);

/**
 * @brief Close RBUS connection (dummy)
 */
rbusError_t rbus_close(rbusHandle_t handle);

/**
 * @brief Register data elements (dummy)
 */
rbusError_t rbus_regDataElements(
    rbusHandle_t handle,
    int numDataElements,
    rbusDataElement_t* elements);

/**
 * @brief Unregister data elements (dummy)
 */
rbusError_t rbus_unregDataElements(
    rbusHandle_t handle,
    int numDataElements,
    rbusDataElement_t* elements);

/**
 * @brief Get parameter value (dummy)
 */
rbusError_t rbus_get(
    rbusHandle_t handle,
    char const* name,
    rbusValue_t* value);

/**
 * @brief Set parameter value (dummy)
 */
rbusError_t rbus_set(
    rbusHandle_t handle,
    char const* name,
    rbusValue_t value,
    rbusSetOptions_t* opts);

#ifdef __cplusplus
}
#endif

#endif /* USE_DUMMY_RBUS */
/* Property and value manipulation functions */
const char* rbusProperty_GetName(rbusProperty_t property);
void rbusProperty_SetValue(rbusProperty_t property, rbusValue_t value);
rbusValue_t rbusProperty_GetValue(rbusProperty_t property);

void rbusValue_Init(rbusValue_t* value);
void rbusValue_Release(rbusValue_t value);
void rbusValue_SetString(rbusValue_t value, const char* str);
const char* rbusValue_GetString(rbusValue_t value, int* length);
void rbusValue_SetBoolean(rbusValue_t value, bool b);
bool rbusValue_GetBoolean(rbusValue_t value);
void rbusValue_SetInt32(rbusValue_t value, int32_t i);
int32_t rbusValue_GetInt32(rbusValue_t value);
void rbusValue_SetUInt32(rbusValue_t value, uint32_t u);
uint32_t rbusValue_GetUInt32(rbusValue_t value);
void rbusValue_SetUInt64(rbusValue_t value, uint64_t u);
uint64_t rbusValue_GetUInt64(rbusValue_t value);
#endif /* EPON_MGR_RBUS_DUMMY_H */
