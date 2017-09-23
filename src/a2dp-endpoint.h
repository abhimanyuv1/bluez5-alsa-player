#ifndef A2DP_ENDPOINT_H_
#define A2DP_ENDPOINT_H_

typedef enum {
    SINK,
    SOURCE
} MediaEndpoint_Type;

gboolean register_a2dp_endpoint (GDBusConnection *conn,
                                const char *path,
                                MediaEndpoint_Type type,
                                const char *endpoint_path);
gboolean deregister_a2dp_endpoint ();

#endif /* A2DP_ENDPOINT_H_ */
