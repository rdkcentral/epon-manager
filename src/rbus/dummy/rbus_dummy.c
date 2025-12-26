/**
 * @file rbus_dummy.c
 * @brief Dummy RBUS implementation for local testing
 * 
 * This provides printf-based stubs that match real RBUS API signatures.
 * Allows building and testing EPON Manager locally without RBUS dependencies.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/rbus/eponMgr_rbus_dummy.h"

#ifdef USE_DUMMY_RBUS

/* Simple handle structure for dummy */
struct _rbusHandle {
    char* componentName;
    int dummy_fd;  /* Not used, just for structure */
};

rbusError_t rbus_open(rbusHandle_t* handle, char const* componentName) {
    if (!handle || !componentName) {
        printf("[DUMMY_RBUS] rbus_open: Invalid input\n");
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    *handle = (rbusHandle_t)malloc(sizeof(struct _rbusHandle));
    if (!*handle) {
        printf("[DUMMY_RBUS] rbus_open: Out of memory\n");
        return RBUS_ERROR_OUT_OF_RESOURCES;
    }
    
    (*handle)->componentName = strdup(componentName);
    (*handle)->dummy_fd = 1;  /* Fake FD */
    
    printf("[DUMMY_RBUS] ✓ rbus_open('%s') - SUCCESS\n", componentName);
    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbus_close(rbusHandle_t handle) {
    if (!handle) {
        printf("[DUMMY_RBUS] rbus_close: Invalid handle\n");
        return RBUS_ERROR_INVALID_HANDLE;
    }
    
    printf("[DUMMY_RBUS] ✓ rbus_close('%s') - SUCCESS\n", 
           handle->componentName ? handle->componentName : "unknown");
    
    if (handle->componentName) {
        free(handle->componentName);
    }
    free(handle);
    
    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbus_regDataElements(
    rbusHandle_t handle,
    int numDataElements,
    rbusDataElement_t* elements) 
{
    if (!handle) {
        printf("[DUMMY_RBUS] rbus_regDataElements: Invalid handle\n");
        return RBUS_ERROR_INVALID_HANDLE;
    }
    
    if (numDataElements <= 0 || !elements) {
        printf("[DUMMY_RBUS] rbus_regDataElements: Invalid input\n");
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    printf("[DUMMY_RBUS] ✓ rbus_regDataElements: Registering %d elements:\n", numDataElements);
    for (int i = 0; i < numDataElements; i++) {
        const char* type_str = "";
        switch (elements[i].type) {
            case RBUS_ELEMENT_TYPE_PROPERTY: type_str = "PROPERTY"; break;
            case RBUS_ELEMENT_TYPE_TABLE: type_str = "TABLE"; break;
            case RBUS_ELEMENT_TYPE_EVENT: type_str = "EVENT"; break;
            case RBUS_ELEMENT_TYPE_METHOD: type_str = "METHOD"; break;
            default: type_str = "UNKNOWN"; break;
        }
        printf("[DUMMY_RBUS]   [%d] %s (type=%s)\n", i+1, elements[i].name, type_str);
    }
    
    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbus_unregDataElements(
    rbusHandle_t handle,
    int numDataElements,
    rbusDataElement_t* elements)
{
    if (!handle) {
        printf("[DUMMY_RBUS] rbus_unregDataElements: Invalid handle\n");
        return RBUS_ERROR_INVALID_HANDLE;
    }
    
    printf("[DUMMY_RBUS] ✓ rbus_unregDataElements: Unregistering %d elements\n", numDataElements);
    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbus_get(
    rbusHandle_t handle,
    char const* name,
    rbusValue_t* value)
{
    if (!handle || !name || !value) {
        printf("[DUMMY_RBUS] rbus_get: Invalid input\n");
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    printf("[DUMMY_RBUS] ✓ rbus_get('%s') - returning dummy value\n", name);
    *value = NULL;  /* Dummy */
    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbus_set(
    rbusHandle_t handle,
    char const* name,
    rbusValue_t value,
    rbusSetOptions_t* opts)
{
    (void)value;
    (void)opts;
    
    if (!handle || !name) {
        printf("[DUMMY_RBUS] rbus_set: Invalid input\n");
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    printf("[DUMMY_RBUS] ✓ rbus_set('%s') - SUCCESS\n", name);
    return RBUS_ERROR_SUCCESS;
}

#endif /* USE_DUMMY_RBUS */
