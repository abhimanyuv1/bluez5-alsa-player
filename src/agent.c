#include <glib.h>
#include <gio/gio.h>

#include "utils.h"
#include "bluez-interface.h"

#define AGENT_PATH                  "/player/agent"

static GDBusProxy *agent_manager1 = NULL;

static const gchar agent1_introspection_xml[] =
    "<node name='/'>"
    "  <interface name='org.bluez.Agent1'>"
    "    <method name='Release'/>"
    "    <method name='RequestPinCode'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='s' name='pincode' direction='out'/>"
    "    </method>"
    "    <method name='RequestPasskey'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='u' name='passkey' direction='out'/>"
    "    </method>"
    "    <method name='DisplayPasskey'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='u' name='passkey' direction='in'/>"
    "      <arg type='q' name='entered' direction='in'/>"
    "    </method>"
    "    <method name='DisplayPinCode'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='s' name='pincode' direction='in'/>"
    "    </method>"
    "    <method name='RequestConfirmation'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='u' name='passkey' direction='in'/>"
    "    </method>"
    "    <method name='RequestAuthorization'>"
    "      <arg type='o' name='device' direction='in'/>"
    "    </method>"
    "    <method name='AuthorizeService'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='s' name='uuid' direction='in'/>"
    "    </method>"
    "    <method name='Cancel'/>"
    "  </interface>"
    "</node>";

static void handle_method_call(GDBusConnection *connection,
			       const gchar *sender,
			       const gchar *object_path,
			       const gchar *interface_name,
			       const gchar *method_name,
			       GVariant *parameters,
			       GDBusMethodInvocation *invocation, gpointer user_data) {

    DEBUG_INFO("method_name: %s\n", method_name);

	if (g_strcmp0(method_name, "AuthorizeService") == 0) {
		const char *path;
		const char *uuid;

		g_variant_get(parameters, "(&o&s)", &path, &uuid);
        DEBUG_INFO("AuthorizeService (path: %s, uuid: %s)...\n", path, uuid);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
}


static const GDBusInterfaceVTable interface_vtable = {
    handle_method_call,             /* method callback */
    NULL,                           /* GetProperty */
    NULL,                           /* SetProperty */
};

gboolean register_agent(GDBusConnection *conn, const gchar *capability) {
    GError *error = NULL;
    GDBusNodeInfo *introspection_data;

    if (agent_manager1) {
        DEBUG_WARN("Agent already registered...\n");
        return FALSE;
    }

    agent_manager1 = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                            G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
                            G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                            NULL,
                            BLUEZ_SERVICE,
                            "/org/bluez",
                            AGENT_MANAGER,
                            NULL,
                            &error);
    if (NULL == agent_manager1) {
        DEBUG_ERROR("Failed to get AgentManager1 proxy, error: %s\n", error->message);
        return FALSE;
    }
    g_clear_error (&error);

    introspection_data = g_dbus_node_info_new_for_xml (agent1_introspection_xml,
                                                       NULL);
    gint reg_id = g_dbus_connection_register_object(conn,
						    AGENT_PATH,
						    introspection_data->interfaces[0],
						    &interface_vtable,
						    NULL,
						    NULL,
						    &error);

    if (0 == reg_id) {
        DEBUG_ERROR("Failed to register object: %s", error->message);
        return FALSE;
    }
    g_clear_error (&error);

    GVariant *result = g_dbus_proxy_call_sync (agent_manager1,
                                    "RegisterAgent",
                                    g_variant_new ("(os)", AGENT_PATH, capability),
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);
    g_clear_error (&error);
    result = g_dbus_proxy_call_sync (agent_manager1,
                                    "RequestDefaultAgent",
                                    g_variant_new ("(o)", AGENT_PATH),
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    DEBUG_INFO("Agent registered...\n");
    return TRUE;
}

gboolean deregister_agent() {
    if (agent_manager1) {
        g_object_unref (agent_manager1);
    }
    return TRUE;
}
