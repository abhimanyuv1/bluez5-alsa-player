#include <glib.h>
#include <gio/gio.h>

#include "a2dp-endpoint.h"
#include "utils.h"
#include "a2dp-codecs.h"
#include "bluez-interface.h"

static const gchar mediaendpoint1_xml[] =
    "<node name='/'>"
    " <interface name='org.bluez.MediaEndpoint1'>"
    "  <method name='SetConfiguration'>"
    "   <arg name='transport' direction='in' type='o'/>"
    "   <arg name='configuration' direction='in' type='a{sv}'/>"
    "  </method>"
    "  <method name='SelectConfiguration'>"
    "   <arg name='capabilities' direction='in' type='ay'/>"
    "   <arg name='configuration' direction='out' type='ay'/>"
    "  </method>"
    "  <method name='ClearConfiguration'/>"
    "  <method name='Release'/>"
    " </interface>"
    "</node>";


static void handle_method_call (GDBusConnection * conn,
                                const gchar * sender,
                                const gchar * path,
                                const gchar * interface,
                                const gchar * method,
                                GVariant * params,
                                GDBusMethodInvocation * invocation,
                                void *userdata) {
	(void)conn;
	(void)sender;
	(void)interface;
	(void)params;

	DEBUG_INFO("Endpoint method call: %s:%s()\n", interface, method);

	if (0 == g_strcmp0(method, "SelectConfiguration")) {

        GVariant *params1 = g_dbus_method_invocation_get_parameters (invocation);
        g_print("v1: %s\n", g_variant_print(params1, TRUE));
        g_print("v2: %s\n", g_variant_print(params, TRUE));

        const uint8_t *data;
        uint8_t *capabilities;
        size_t size = 0;

        params = g_variant_get_child_value (params, 0);
        data = g_variant_get_fixed_array (params, &size, sizeof (uint8_t));
        capabilities = g_memdup (data, size);
        g_variant_unref (params);

        GVariantBuilder *caps = g_variant_builder_new (G_VARIANT_TYPE ("ay"));
        size_t i;

        for (i = 0; i < size; i++) {
            g_variant_builder_add (caps, "y", capabilities[i]);
        }

        g_dbus_method_invocation_return_value (invocation, g_variant_new ("(ay)", caps));

	} else if (g_strcmp0(method, "SetConfiguration") == 0) {

        GVariant *params = g_dbus_method_invocation_get_parameters (invocation);
        const char *transport;
        GVariantIter *properties;

        g_variant_get (params, "(&oa{sv})", &transport, &properties);
        DEBUG_INFO ("Transport: %s\n", transport);

        // TODO: Properties will contain capabilities
        //DEBUG_INFO ("Properties: %s\n", g_variant_print(params, TRUE));

        g_dbus_method_invocation_return_value (invocation, NULL);
    } else if (g_strcmp0(method, "ClearConfiguration") == 0) {

        g_dbus_method_invocation_return_value (invocation, NULL);

	} else if (g_strcmp0(method, "Release") == 0) {

        g_dbus_method_invocation_return_value (invocation, NULL);

    } else {
		DEBUG_WARN ("Unsupported endpoint method: %s", method);
    }
}


static const GDBusInterfaceVTable interface_vtable = {
    handle_method_call,         /* method callback */
    NULL,                       /* GetProperty */
    NULL,                       /* SetProperty */
};

gboolean register_a2dp_endpoint(GDBusConnection *conn,
                                const char *path,
                                MediaEndpoint_Type type,
                                const char *endpoint_path) {
    GError *error = NULL;
    GDBusNodeInfo *introspection_data;

    introspection_data = g_dbus_node_info_new_for_xml (mediaendpoint1_xml,
                                                       &error);
    if (NULL == introspection_data) {
        DEBUG_ERROR ("Failed to get info from mediaendpoint1_xml, error: %s\n",
                     error->message);
        return FALSE;
    }
    g_clear_error(&error);

	gint reg_id = g_dbus_connection_register_object(conn,
						    endpoint_path,
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

	GDBusMessage *msg = g_dbus_message_new_method_call("org.bluez",
                                         path,
					                     MEDIA_INTERFACE,
                                         "RegisterEndpoint");

    a2dp_sbc_t sbc_configuration = {
        .frequency = SBC_SAMPLING_FREQ_16000 | SBC_SAMPLING_FREQ_32000 |
                     SBC_SAMPLING_FREQ_44100 | SBC_SAMPLING_FREQ_48000,
        .channel_mode = SBC_CHANNEL_MODE_MONO | SBC_CHANNEL_MODE_DUAL_CHANNEL |
                        SBC_CHANNEL_MODE_STEREO | SBC_CHANNEL_MODE_JOINT_STEREO,
        .block_length = SBC_BLOCK_LENGTH_4 | SBC_BLOCK_LENGTH_8 |
                        SBC_BLOCK_LENGTH_12 | SBC_BLOCK_LENGTH_16,
        .subbands = SBC_SUBBANDS_4 | SBC_SUBBANDS_8,
        .allocation_method = SBC_ALLOCATION_SNR | SBC_ALLOCATION_LOUDNESS,
        .min_bitpool = MIN_BITPOOL,
        .max_bitpool = MAX_BITPOOL
    };

    GVariantBuilder *properties = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));
    GVariantBuilder *caps = g_variant_builder_new (G_VARIANT_TYPE ("ay"));

    for (int i = 0; i < sizeof(sbc_configuration); i++) {
        g_variant_builder_add (caps, "y", ((uint8_t *) &sbc_configuration)[i]);
    }

    const char *uuid;
    if (SINK == type) {
        uuid = UUID_A2DP_SINK;
    }
    g_variant_builder_add (properties, "{sv}", "UUID", g_variant_new_string (uuid));
    g_variant_builder_add (properties, "{sv}", "Codec", g_variant_new_byte (A2DP_CODEC_SBC));
    g_variant_builder_add (properties, "{sv}", "Capabilities", g_variant_new ("ay", caps));

    g_dbus_message_set_body (msg, g_variant_new ("(oa{sv})", endpoint_path, properties));

    GDBusMessage *resp = g_dbus_connection_send_message_with_reply_sync (conn, msg,
                                                          G_DBUS_SEND_MESSAGE_FLAGS_NONE,
                                                          -1,
                                                          NULL,
                                                          NULL,
                                                          &error);
    if (resp == NULL) {
        DEBUG_ERROR ("Failed to register (type: %d) endpoint: %s\n", type, error->message);
        return FALSE;
    }

    DEBUG_INFO ("Endpoint (type: %d) is registered...\n", type);

    return TRUE;
}

gboolean deregister_a2dp_endpoint() {
    // Call UnRegisterEndpoint
    return TRUE;
}
