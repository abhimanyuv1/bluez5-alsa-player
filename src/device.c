#include <glib.h>
#include <gio/gio.h>

#include "utils.h"
#include "bluez-interface.h"
#include "device.h"

static GDBusProxy *fdo_properties = NULL;

static gboolean device_set_trusted() {
    GError *error = NULL;

    GVariant *result = g_dbus_proxy_call_sync (fdo_properties,
                                    OBJECT_MANAGER_PROP_SET,
                                    g_variant_new ("(ssv)",
                                                   DEVICE_INTERFACE,
                                                   "Trusted",
                                                   g_variant_new_boolean (TRUE)),
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);
    if (NULL == result) {
        DEBUG_ERROR ("Error to set device Trusted, error: %s\n", error->message);
        return FALSE;
    }
    g_clear_error (&error);

    DEBUG_INFO("Device1 is now Trusted...\n");
    return TRUE;
}

gboolean device_added(const gchar *path) {
    GError *error = NULL;
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

    if (FALSE == device_set_trusted ()) {
        return FALSE;
    }

    return TRUE;
}

