#ifndef AGENT_H_
#define AGENT_H_

gboolean register_agent(GDBusConnection *conn, const gchar *capability);
gboolean deregister_agent ();

#endif // AGENT_H_
