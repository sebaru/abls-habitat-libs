/******************************************************************************************************************************/
/* include/mqtt.h      Declarations de la couche MQTT partagee — abls-libs                                                   */
/* Projet Abls-Libs version 1.0       Gestion d'habitat                                                     13.06.2026       */
/* Auteur: LEFEVRE Sebastien                                                                                                  */
/******************************************************************************************************************************/
/*
 * mqtt.h
 * This file is part of Abls-Libs
 *
 * Copyright (C) 1988-2026 - Sébastien LEFÈVRE
 *
 * Abls-Libs is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Abls-Libs is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Abls-Libs; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef _ABLS_MQTT_H_
 #define _ABLS_MQTT_H_

 #include <glib.h>
 #include <json-glib/json-glib.h>
 #include <mosquitto.h>

 #define ABLS_MQTT_RECONNECT_DELAY 5                                                                            /* en seconde */

 struct ABLS_MQTT
  { struct mosquitto *MOSQ_session;
    gchar *log_facility;
    gchar *log_prefixe;
    gchar *client_id;
    gchar *hostname;
    gchar *username;
    gchar *password;
    gint   port;
    gboolean connected;
    gint next_top_connect;
    gint qos;
    GAsyncQueue *queue;
    GRWLock subscribed_topics_lock;
    GSList *subscribed_topics;
  };

/*-- API publique MQTT -------------------------------------------------------------------------------------------------------*/
 extern struct ABLS_MQTT *Mqtt_init ( const gchar *log_facility, const gchar *log_prefixe, const gchar *client_id,
                                           gboolean is_ssl, const gchar *ca_file, const gchar *ca_path,
                                           const gchar *username, const gchar *password,
                                           const gchar *hostname, gint port, gint qos );
 extern gboolean Mqtt_start ( struct ABLS_MQTT *mqtt );
 extern void Mqtt_stop      ( struct ABLS_MQTT *mqtt );
 extern gboolean Mqtt_is_connected ( struct ABLS_MQTT *mqtt );
 extern void Mqtt_last_will ( struct ABLS_MQTT *mqtt, const gchar *will, gchar *format, ... );
 extern void Mqtt_subscribe ( struct ABLS_MQTT *mqtt, gchar *format, ... );
 extern void Mqtt_unsubscribe ( struct ABLS_MQTT *mqtt, gchar *format, ... );
 /* Mqtt_topic_is: compare mqtt_topic_lvlX a une liste de niveaux; un niveau attendu NULL est ignore */
 extern gboolean Mqtt_topic_is ( JsonNode *request, gint level_count, ... );
 /* Mqtt_get_topic_lvl: renvoie la valeur du niveau mqtt_topic_lvlX demande, ou NULL si absente */
 extern const gchar *Mqtt_get_topic_lvl ( JsonNode *request, gint level );
 extern void Mqtt_send_message     ( struct ABLS_MQTT *mqtt, JsonNode *node, gboolean retain, gchar *topic, ... );
 extern JsonNode *Mqtt_get_message ( struct ABLS_MQTT *mqtt );

#endif /* _ABLS_MQTT_H_ */
/*----------------------------------------------------------------------------------------------------------------------------*/
