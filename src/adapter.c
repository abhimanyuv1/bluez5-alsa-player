#include <glib.h>
#include <gio/gio.h>

#include "utils.h"
#include "bluez-interface.h"
#include "adapter.h"

static GDBusProxy *fdo_properties = NULL;

static gboolean adapter_set_powered() {
    GError *error = NULL;

    GVariant *result = g_dbus_proxy_call_sync (fdo_properties,
                                    OBJECT_MANAGER_PROP_SET,
                                    g_variant_new ("(ssv)",
                                                   ADAPTER_INTERFACE,
                                                   "Powered",
                                                   g_variant_new_boolean (TRUE)),
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);
    if (NULL == result) {
        DEBUG_ERROR ("Error to set adapter powered on, error: %s\n", error->message);
        return FALSE;
    }
    g_clear_error (&error);

    DEBUG_INFO ("Adapter1 is powered on...\n");
    return TRUE;
}

static gboolean adapter_set_discoverable() {
    GError *error = NULL;

    GVariant *result = g_dbus_proxy_call_sync (fdo_properties,
                                    OBJECT_MANAGER_PROP_SET,
                                    g_variant_new ("(ssv)",
                                                   ADAPTER_INTERFACE,
                                                   "Discoverable",
                                                   g_variant_new_boolean (TRUE)),
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);
    if (NULL == result) {
        DEBUG_ERROR ("Error to set adapter discoverable, error: %s\n", error->message);
        return FALSE;
    }
    g_clear_error (&error);

    DEBUG_INFO("Adapter1 is now discoverable...\n");
    return TRUE;
}


gboolean adapter_added(const gchar *path) {
    GError *error = NULL;
    /* Support only one adapter */
    if (fdo_properties) {
        return TRUE;
    }

    fdo_properties = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                            G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
                            G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                            NULL,
                            BLUEZ_SERVICE,
                            path,
                            OBJECT_MANAGER,
                            NULL,
                            &error);
    if (NULL == fdo_properties) {
        DEBUG_ERROR("Failed to get freedesktop Properties proxy, error: %s\n", error->message);
        return FALSE;
    }
    g_clear_error (&error);

    if (FALSE == adapter_set_powered ()) {
        return FALSE;
    }
    if (FALSE == adapter_set_discoverable ()) {
        return FALSE;
    }
    return TRUE;
}

