#include <glib.h>
#include <gio/gio.h>

#include "agent.h"
#include "utils.h"
#include "a2dp-endpoint.h"
#include "bluez-interface.h"

#define A2DP_SBC_SINK_ENDPOINT      "/MediaEndpoint/SBC/Sink"

static void interface_added(GDBusConnection *conn,
                            const char *path,
                            GVariant *variant) {
    //DEBUG_INFO("interface: %s, path: %s\n", g_variant_print(variant, TRUE), path);

    if (g_variant_lookup_value(variant, AGENT_MANAGER,
                   G_VARIANT_TYPE_DICTIONARY)) {
        DEBUG_INFO("AgentManager1 interface found\n");
        register_agent (conn, "NoInputNoOutput");
    }

    if (g_variant_lookup_value(variant, MEDIA_INTERFACE,
                   G_VARIANT_TYPE_DICTIONARY)) {
        DEBUG_INFO("Media1 interface found\n");
        register_a2dp_endpoint (conn, path, SINK, A2DP_SBC_SINK_ENDPOINT);
    }
}

static void interface_removed(GDBusConnection * conn,
			                  const char *path,
			                  GVariant *variant) {
	//DEBUG_INFO("interface_remove: %s, path: %s\n", g_variant_print(variant, TRUE), path);

    GVariantIter *iter;
    gchar *str;

    g_variant_get (variant, "as", &iter);
    while (g_variant_iter_loop (iter, "s", &str)) {
        if (0 == g_strcmp0(str, "AGENT_MANAGER")) {
		    DEBUG_INFO("AgentManager1 interface removed\n");
		    deregister_agent();
        }

        DEBUG_INFO ("%s\n", str);
    }
    g_variant_iter_free (iter);
}


static void
object_manager_g_signal(GDBusProxy * proxy,
            gchar * sender_name, gchar * signal_name, GVariant * parameters,
            GDBusConnection * conn)
{
    char *object_path;
    GVariant *variant;

    g_variant_get(parameters, "(o*)", &object_path, &variant);

    if (g_strcmp0(signal_name, "InterfacesAdded") == 0) {
        interface_added(conn, object_path, variant);
    } else if (g_strcmp0(signal_name, "InterfacesRemoved") == 0) {
        interface_removed(conn, object_path, variant);
    } else {
        g_assert_not_reached();
    }

    g_free(object_path);
}


static void bluez_appeared_cb(GDBusConnection * conn,
                            const gchar * name,
                            const gchar * name_owner,
                            void *data) {
    GVariant *objects;
    GError *error = NULL;

    DEBUG_INFO("Bluez appeared...\n");

    GDBusProxy *object_manager = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                   G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
                                   G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                   NULL,
                                   BLUEZ_SERVICE,
                                   "/",
                                   OBJECT_MANAGER,
                                   NULL,
                                   &error);

    g_signal_connect (G_OBJECT(object_manager), "g-signal",
                        G_CALLBACK(object_manager_g_signal), conn);

    objects = g_dbus_proxy_call_sync (object_manager,
                                    "GetManagedObjects",
                                    NULL,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);
    if (objects == NULL) {
        g_object_unref(object_manager);
        return;
    }

    GVariant *value;
    GVariantIter *iter;
    char *object_path;

    g_variant_get(objects, "(a{oa{sa{sv}}})", &iter);

    while ((value  = g_variant_iter_next_value (iter))) {
        GVariant *ifaces;
        char *key;

        g_variant_get(value, "{o*}", &key, &ifaces);
        interface_added(conn, key, ifaces);

        g_free(key);
    }

    g_variant_unref(objects);
}

static void bluez_vanished_cb(GDBusConnection *connection,
                            const gchar * name,
                            void *data) {
    DEBUG_INFO("Bluez disappear...\n");
    deregister_agent ();
}

int main() {
    guint owner_change_id = g_bus_watch_name(G_BUS_TYPE_SYSTEM,
                         BLUEZ_SERVICE,
                         G_BUS_NAME_WATCHER_FLAGS_NONE,
                         (GBusNameAppearedCallback)bluez_appeared_cb,
                         (GBusNameVanishedCallback)bluez_vanished_cb,
                         NULL, NULL);

    GMainLoop *mainloop = g_main_loop_new(NULL, FALSE);

    g_main_loop_run(mainloop);

    g_main_loop_unref(mainloop);

    return 0;
}

