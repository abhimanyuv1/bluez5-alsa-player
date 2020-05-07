#include <glib.h>
#include <gio/gio.h>
#include <gst/gst.h>

#include "utils.h"
#include "bluez-interface.h"
#include "media-transport.h"

static GstElement *pipeline = NULL;

static gboolean bus_message(GstBus *bus, GstMessage *message, gpointer data) {
	GstElement *pipeline = (GstElement*)data;

	switch (GST_MESSAGE_TYPE(message)) {
	case GST_MESSAGE_EOS:
	{
		g_print("Eos");
		gst_element_set_state(pipeline, GST_STATE_NULL);
		break;
	}

	case GST_MESSAGE_ERROR:
	{
		//GstPlayError gst_err = GST_ERROR_UNKNOWN;
		GError *err = NULL;
		gchar *dbg_info = NULL;
		gst_message_parse_error(message, &err, &dbg_info);
		g_print("GST_MESSAGE_ERROR src: %s message: [%s] debugging info: [%s]",
			GST_MESSAGE_SRC_NAME(message), err->message,
			(dbg_info) ? dbg_info : "none");

		g_error_free(err);
		g_free(dbg_info);

		break;
	}

	default:
		break;
	}
}

static void on_signal(GDBusProxy *      proxy,
		      gchar *           sender_name,
		      gchar *           signal_name,
		      GVariant *        parameters,
		      gpointer user_data) {
	char *object_path;
	GVariant *variant;
	GVariantIter *iter;

	//DEBUG_INFO ("%s\n", g_variant_print(parameters, TRUE));

	g_variant_get(parameters, "(sa{sv}as)", &object_path, &iter);

	while ((variant = g_variant_iter_next_value(iter))) {
		gchar *key, *state;
		gsize len;
		GVariant *v;
		g_variant_get(variant, "{sv}", &key, &v);
		state = g_variant_dup_string(v, &len);
		//g_print("%s\n", state);
		if (0 == g_strcmp0(state, "idle"))
			gst_element_set_state(pipeline, GST_STATE_PAUSED);
		else if (0 == g_strcmp0(state, "active"))
			gst_element_set_state(pipeline, GST_STATE_PLAYING);

		g_free(state);
	}
}

gboolean media_transport_added(const gchar *path) {
	GError *error = NULL;

	DEBUG_INFO("Transport path: %s\n", path);

	GDBusProxy *mediatransport1 = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
								    G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
								    NULL,
								    BLUEZ_SERVICE,
								    path,
								    OBJECT_MANAGER_PROP,
								    NULL,
								    &error);

	g_signal_connect(G_OBJECT(mediatransport1), "g-signal", G_CALLBACK(on_signal),
			 NULL);

	gchar *str = g_strdup_printf("avdtpsrc transport=%s ! rtpsbcdepay ! sbcparse ! sbcdec ! queue ! autoaudiosink", path);
	g_print("%s\n", str);
	pipeline = gst_parse_launch(str, NULL);
	GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	gst_bus_add_watch(bus, bus_message, pipeline);
	gst_object_unref(bus);
	g_free(str);

	return TRUE;
}

