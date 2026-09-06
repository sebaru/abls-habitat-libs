/******************************************************************************************************************************/
/* src/mqtt.c      Couche MQTT partagee pour Abls-Libs                                                                        */
/* Projet Abls-Libs version 1.0       Gestion d'habitat                                                      13.06.2026       */
/* Auteur: LEFEVRE Sebastien                                                                                                  */
/******************************************************************************************************************************/
/*
 * mqtt.c
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

 #include <glib.h>
 #include <string.h>
 #include <syslog.h>
 #include <time.h>

 #include "abls-libs.h"

/******************************************************************************************************************************/
/* Mqtt_default_ca_file: Recherche le fichier CA systeme par defaut pour la validation TLS MQTT                               */
/* Entrées: néant                                                                                                             */
/* Sortie : le chemin du fichier CA, ou NULL si aucun fichier n'est disponible                                                */
/******************************************************************************************************************************/
 static const gchar *Mqtt_default_ca_file ( void )
  { if ( g_file_test ( "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem", G_FILE_TEST_IS_REGULAR ) )
     { return("/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem"); }

   if ( g_file_test ( "/etc/ssl/certs/ca-certificates.crt", G_FILE_TEST_IS_REGULAR ) )
     { return("/etc/ssl/certs/ca-certificates.crt"); }

   if ( g_file_test ( "/etc/ssl/certs/ca-bundle.crt", G_FILE_TEST_IS_REGULAR ) )
     { return("/etc/ssl/certs/ca-bundle.crt"); }

    return(NULL);
  }

/******************************************************************************************************************************/
/* Mqtt_default_ca_path: Recherche le repertoire CA systeme par defaut pour la validation TLS MQTT                            */
/* Entrées: néant                                                                                                             */
/* Sortie : le chemin du repertoire CA, ou NULL si aucun repertoire n'est disponible                                          */
/******************************************************************************************************************************/
 static const gchar *Mqtt_default_ca_path ( void )
  { if ( g_file_test ( "/etc/pki/tls/certs", G_FILE_TEST_IS_DIR ) )
     { return("/etc/pki/tls/certs"); }

    if ( g_file_test ( "/etc/ssl/certs", G_FILE_TEST_IS_DIR ) )
     { return("/etc/ssl/certs"); }

    return(NULL);
  }
/******************************************************************************************************************************/
/* Mqtt_topic_is: Compare les niveaux mqtt_topic_lvlX d'un JsonNode avec une liste variadique                                 */
/* Entrées: request, le nombre de niveaux attendus, puis les niveaux attendus (NULL = wildcard/ignore ce niveau)              */
/* Sortie : TRUE si tous les niveaux non-NULL correspondent, FALSE sinon                                                      */
/******************************************************************************************************************************/
 gboolean Mqtt_topic_is ( JsonNode *request, gint level_count, ... )
  { va_list ap;

    if (!request || level_count <= 0) return(FALSE);

    va_start(ap, level_count);
    for (gint i = 0; i < level_count; i++)
     { const gchar *expected = va_arg(ap, const gchar *);
       if (!expected || g_strcmp0(expected, "+") == 0) continue;    /* Si attendu est NUL ou "+", on ne controle pas ce champ */

       gchar name[32];
       g_snprintf ( name, sizeof(name), "mqtt_topic_lvl%d", i );
       const gchar *received = Json_get_string ( request, name );                         /* Récupération du level i du topic */

       if (!received || g_strcmp0 ( received, expected ) )                    /* Si erreur ou différent de l'attendu, on sort */
        { va_end(ap);
          return(FALSE);
        }
     }
    va_end(ap);
    return(TRUE);
  }
/******************************************************************************************************************************/
/* Mqtt_get_topic_lvl: Recupere le niveau demande du topic mqtt_topic_lvlX d'un JsonNode                                      */
/* Entrées: request, le niveau demande                                                                                        */
/* Sortie : la chaine correspondante, ou NULL si absente                                                                      */
/******************************************************************************************************************************/
 const gchar *Mqtt_get_topic_lvl ( JsonNode *request, gint level )
  { gchar name[32];

    if (!request || level < 0) return(NULL);

    g_snprintf ( name, sizeof(name), "mqtt_topic_lvl%d", level );
    return( Json_get_string ( request, name ) );
  }
/******************************************************************************************************************************/
/* Mqtt_last_will: Configure le Last Will MQTT via un topic formate en variadique                                             */
/* Entrées: client MQTT, payload du will (chaine C terminee par NUL), format/topic variadique                                 */
/* Sortie : Néant                                                                                                             */
/******************************************************************************************************************************/
 void Mqtt_last_will ( struct ABLS_MQTT *mqtt, const gchar *will, gchar *format, ... )
  { gchar topic_full[256];
    va_list ap;
    gint retour;

    if (!mqtt || !mqtt->MOSQ_session || !will || !format) return;

    va_start( ap, format );
    g_vsnprintf ( topic_full, sizeof(topic_full), format, ap );
    va_end ( ap );

    retour = mosquitto_will_set ( mqtt->MOSQ_session, topic_full, strlen(will), will, mqtt->qos, FALSE );
    if (retour != MOSQ_ERR_SUCCESS)
     { Info( __func__, mqtt->log_facility, mqtt->log_prefixe, LOG_ERR, "MQTT last will setup error on topic '%s': %s",
             topic_full, mosquitto_strerror ( retour ) );
     }
  }
/******************************************************************************************************************************/
/* Mqtt_subscribe: Abonne le client MQTT à un topic spécifique                                                                */
/* Entrées: le client MQTT, le topic à abonner                                                                                */
/* Sortie : Néant                                                                                                             */
/******************************************************************************************************************************/
 void Mqtt_subscribe( struct ABLS_MQTT *mqtt, gchar *format, ... )
  { gchar topic_full[256];
    va_list ap;
    if(!mqtt || !format) return;
    va_start( ap, format );
    g_vsnprintf ( topic_full, sizeof(topic_full), format, ap );
    va_end ( ap );
    g_rw_lock_writer_lock(&mqtt->subscribed_topics_lock);
    mqtt->subscribed_topics = g_slist_append(mqtt->subscribed_topics, g_strdup(topic_full));
    g_rw_lock_writer_unlock(&mqtt->subscribed_topics_lock);
  }
/******************************************************************************************************************************/
/* Mqtt_unsubscribe: Désabonne le client MQTT d'un topic spécifique                                                           */
/* Entrées: le client MQTT, le topic à désabonner                                                                             */
/* Sortie : Néant                                                                                                             */
/******************************************************************************************************************************/
 void Mqtt_unsubscribe( struct ABLS_MQTT *mqtt, gchar *format, ... )
  { gchar topic_full[256];
    va_list ap;
    if(!mqtt || !format) return;
    va_start( ap, format );
    g_vsnprintf ( topic_full, sizeof(topic_full), format, ap );
    va_end ( ap );
    g_rw_lock_writer_lock(&mqtt->subscribed_topics_lock);
    GSList *found = g_slist_find_custom ( mqtt->subscribed_topics, topic_full, (GCompareFunc)g_ascii_strcasecmp );
    if (found)
     { mqtt->subscribed_topics = g_slist_remove(mqtt->subscribed_topics, found->data);
       g_free(found->data);
     }
    g_rw_lock_writer_unlock(&mqtt->subscribed_topics_lock);
  }
/******************************************************************************************************************************/
/* Mqtt_on_log_CB: Affiche un log de la librairie MQTT                                                                        */
/* Entrée: les parametres d'affichage de log de la librairie                                                                  */
/* Sortie: Néant                                                                                                              */
/******************************************************************************************************************************/
 static void Mqtt_on_log_CB( struct mosquitto *mosq, void *obj, int level, const char *message )
  { struct ABLS_MQTT *mqtt = (struct ABLS_MQTT *)obj;
    gint info_level;
    switch(level)
     { case MOSQ_LOG_DEBUG:   return;                                         /* On ne log pas les message de DEBUG mosquitto */
       default:
       case MOSQ_LOG_INFO:    info_level = LOG_INFO;    break;
       case MOSQ_LOG_NOTICE:  info_level = LOG_NOTICE;  break;
       case MOSQ_LOG_WARNING: info_level = LOG_WARNING; break;
       case MOSQ_LOG_ERR:     info_level = LOG_ERR;     break;
     }
    Info( __func__, mqtt->log_facility, mqtt->log_prefixe, info_level, "%s", message );
  }
/******************************************************************************************************************************/
/* Mqtt_on_connect_CB: appelé par la librairie quand le broker est connecté                                                   */
/* Entrée: les parametres d'affichage de log de la librairie                                                                  */
/* Sortie: Néant                                                                                                              */
/******************************************************************************************************************************/
 static void Mqtt_on_connect_CB( struct mosquitto *mosq, void *obj, int return_code )
  { struct ABLS_MQTT *mqtt = (struct ABLS_MQTT *)obj;
    if (return_code == 0)
     { Info( __func__, mqtt->log_facility, mqtt->log_prefixe, LOG_NOTICE, "Connected %s@%s:%d (client_id=%s) -> return code %d: %s",
             mqtt->username, mqtt->hostname, mqtt->port, mqtt->client_id, return_code, mosquitto_connack_string( return_code ) );
       mqtt->connected = TRUE;
       g_rw_lock_reader_lock(&mqtt->subscribed_topics_lock);
       GSList *liste = mqtt->subscribed_topics;
       while (liste)                                                                    /* souscrit aux topics a la connexion */
        { gchar *topic = (gchar *)liste->data;
          if ( mosquitto_subscribe( mqtt->MOSQ_session, NULL, topic, mqtt->qos ) != MOSQ_ERR_SUCCESS )
           { Info( __func__, mqtt->log_facility, mqtt->log_prefixe, LOG_ERR, "Subscribe to topic '%s' FAILED", topic ); }
          else
           { Info( __func__, mqtt->log_facility, mqtt->log_prefixe, LOG_INFO, "Subscribe to topic '%s' OK", topic ); }
          liste = liste->next;
        }
       g_rw_lock_reader_unlock(&mqtt->subscribed_topics_lock);
       while ( g_async_queue_length ( mqtt->queue ) )                              /* s'il y en a, envoie les messages queued */
        { JsonNode *node = g_async_queue_pop ( mqtt->queue );
          gchar *topic    = Json_get_string ( node, "mqtt_queued_topic" );
          gboolean retain = Json_get_bool   ( node, "mqtt_queued_retain" );
          gchar *buffer = Json_to_string ( node );
          if (buffer)
           { mosquitto_publish( mqtt->MOSQ_session, NULL, topic, strlen(buffer), buffer, mqtt->qos, retain );
             g_free(buffer);
           }
          Json_unref(node);
        }
     }
    else
     { Info( __func__, mqtt->log_facility, mqtt->log_prefixe, LOG_ERR, "Connection failed %s@%s:%d (client_id=%s) -> return code %d: %s",
             mqtt->username, mqtt->hostname, mqtt->port, mqtt->client_id, return_code, mosquitto_connack_string( return_code ) );
     }

  }
/******************************************************************************************************************************/
/* Mqtt_on_disconnect_CB: appelé par la librairie quand le broker est déconnecté                                              */
/* Entrée: les parametres d'affichage de log de la librairie                                                                  */
/* Sortie: Néant                                                                                                              */
/******************************************************************************************************************************/
 static void Mqtt_on_disconnect_CB( struct mosquitto *mosq, void *obj, int return_code )
  { struct ABLS_MQTT *mqtt = (struct ABLS_MQTT *)obj;
    Info( __func__, mqtt->log_facility, mqtt->log_prefixe, LOG_NOTICE, "Disconnected %s@%s:%d (client_id=%s) -> return code %d: %s",
      mqtt->username, mqtt->hostname, mqtt->port, mqtt->client_id, return_code, mosquitto_connack_string( return_code ) );
    mqtt->connected = FALSE;
  }
/******************************************************************************************************************************/
/* Mqtt_on_message_CB: Appelé par mosquitto lorsqu'un message MQTT est reçu                                                   */
/* Entrée: session MQTT, contexte utilisateur, message reçu                                                                   */
/* Sortie: Néant                                                                                                              */
/******************************************************************************************************************************/
 static void Mqtt_on_message_CB ( struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg )
  { struct ABLS_MQTT *mqtt = (struct ABLS_MQTT *)obj;
    Info( __func__, mqtt->log_facility, mqtt->log_prefixe, LOG_DEBUG, "MQTT Message received on topic: %s", msg->topic );
    gchar **tokens = g_strsplit ( msg->topic, "/", 5 );                /* On ne s'attend pas à plus de 5 tokens dans le topic */
    if (!tokens)
     { Info( __func__, mqtt->log_facility, mqtt->log_prefixe, LOG_ERR, "MQTT with no topic. Dropping" );
       return;
     }

/*-------------------------------------------------- Message with payload ----------------------------------------------------*/
    JsonNode *message = NULL;                                      /* Request peut etre nulle si mal formée ou pas de payload */
    if (msg->payload && msg->payloadlen > 0)
     { gchar *payload = g_strndup ( (const gchar *)msg->payload, msg->payloadlen );
       message = Json_get_from_string ( payload );                 /* Request peut etre nulle si mal formée ou pas de payload */
       g_free ( payload );
       if (!message)
        { message = Json_create();                                            /* Crée un node vide si pas de payload not json */
          if (!message)
           { Info( __func__, mqtt->log_facility, mqtt->log_prefixe, LOG_ALERT, "Memory error creating message. Dropping" ); }
          else { Json_add_string ( message, "payload", payload ); }     /* Ajoute le payload brut pour traitement plus simple */
        }
     }                                                             /* Request peut etre nulle si mal formee ou pas de payload */
    else
     { message = Json_create();                                                        /* Crée un node vide si pas de payload */
       if (!message)
        { Info( __func__, mqtt->log_facility, mqtt->log_prefixe, LOG_ALERT, "Memory error creating message. Dropping" ); }
     }

    if (message)                                                            /* Si on a bien un payload ou un tampon tout neuf */
     { for (gint i=0; tokens[i]; i++)
        { gchar name[32];
          g_snprintf ( name, sizeof(name), "mqtt_topic_lvl%d", i );
          Json_add_string ( message, name, tokens[i] );         /* Ajoute les tokens dans le node pour traitement plus simple */
        }
       Json_add_string ( message, "mqtt_topic", msg->topic );                         /* Ajoute le topic complet dans le node */
       g_async_queue_push ( mqtt->queue, message );/* Ajoute le message dans la queue pour traitement par le thread principal */
     }
    g_strfreev( tokens );                                                                      /* Libération des tokens topic */
  }
/******************************************************************************************************************************/
/* Mqtt_get_message: Tente de récupérer un message JSON depuis la queue MQTT sans bloquer                                     */
/* Entrées: le client MQTT                                                                                                    */
/* Sortie : un JsonNode si disponible, NULL sinon                                                                             */
/******************************************************************************************************************************/
 JsonNode *Mqtt_get_message ( struct ABLS_MQTT *mqtt )
  { if (!mqtt || !mqtt->queue) return(NULL);
    if (mqtt->connected == FALSE && mqtt->next_top_connect <= time(NULL) )                        /* tentative de reconnexion */
     { Info( __func__, mqtt->log_facility, mqtt->log_prefixe, LOG_INFO, "Retrying MQTT connection to '%s'.", mqtt->hostname );
       mosquitto_reconnect_async(	mqtt->MOSQ_session	);
       mqtt->next_top_connect = time(NULL) + ABLS_MQTT_RECONNECT_DELAY;
     }
    return (JsonNode *)g_async_queue_try_pop ( mqtt->queue );
  }
/******************************************************************************************************************************/
/* Mqtt_send_message: Envoie le node au broker                                                                                */
/* Entrée: le topic, le node, le flag retain                                                                                  */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
 void Mqtt_send_message ( struct ABLS_MQTT *mqtt, JsonNode *node, gboolean retain, gchar *topic, ... )
  { gchar topic_full[256];
    va_list ap;
    if (!mqtt || !mqtt->MOSQ_session || !topic) return;

    va_start( ap, topic );
    g_vsnprintf ( topic_full, sizeof(topic_full), topic, ap );
    va_end ( ap );

    gboolean free_node=FALSE;
    if (!node) { node = Json_create(); free_node = TRUE; }
    /*Json_add_int ( node, "mqtt_time", time(NULL) );*/
    gchar *buffer = Json_to_string ( node );
    if (!buffer) goto end;

    gint retour = mosquitto_publish( mqtt->MOSQ_session, NULL, topic_full, strlen(buffer), buffer, mqtt->qos, retain );
    if (retour != MOSQ_ERR_SUCCESS )                               /* Si problème d'envoi au MQTT Broker, on garde en mémoire */
     { Info( __func__, mqtt->log_facility, mqtt->log_prefixe, LOG_ERR, "MQTT publish error: %s. Keeping in memory to send later.", mosquitto_strerror ( retour ) );
       JsonNode *new_node = json_node_copy ( node );                      /* Prépare un new node avec topic et retain intégré */
       Json_add_string ( new_node, "mqtt_queued_topic", topic_full );
       Json_add_bool   ( new_node, "mqtt_queued_retain", retain );
       g_async_queue_push ( mqtt->queue, new_node );
     }
end:
    if(buffer) g_free(buffer);
    if (free_node) Json_unref(node);
  }
/******************************************************************************************************************************/
/* Mqtt_init: Alloue/initialise un client MQTT et configure les callbacks                                                     */
/* Entrées: paramètres de connexion, sécurité TLS et journalisation                                                           */
/* Sortie: pointeur ABLS_MQTT prêt à démarrer, ou NULL si erreur                                                              */
/******************************************************************************************************************************/
 struct ABLS_MQTT *Mqtt_init ( const gchar *log_facility, const gchar *log_prefixe, const gchar *client_id,
                               gboolean is_ssl, const gchar *ca_file, const gchar *ca_path,
                               const gchar *username, const gchar *password, const gchar *hostname, gint port, gint qos )
  { const gchar *resolved_ca_file = ca_file;
    const gchar *resolved_ca_path = ca_path;

    if (!log_facility) log_facility = "mqtt";

    if (!resolved_ca_file) resolved_ca_file = Mqtt_default_ca_file();
    if (!resolved_ca_path) resolved_ca_path = Mqtt_default_ca_path();

    if (is_ssl && !resolved_ca_file && !resolved_ca_path)
     { Info( __func__, log_facility, log_prefixe, LOG_ERR, "MQTT TLS setup error: no CA file or CA path found." );
       return(NULL);
     }

    struct ABLS_MQTT *mqtt = g_try_malloc0 ( sizeof ( struct ABLS_MQTT ) );
    if (!mqtt)
      { Info( __func__, log_facility, log_prefixe, LOG_ALERT, "Memory error." ); return(NULL); }

    mqtt->log_facility = g_strdup(log_facility);
    mqtt->log_prefixe  = g_strdup(log_prefixe);
    mqtt->client_id    = g_strdup(client_id);
    mqtt->hostname     = g_strdup(hostname);
    mqtt->username     = g_strdup(username);
    mqtt->password     = g_strdup(password);
    mqtt->port         = port;
    mqtt->connected    = FALSE;
    mqtt->qos          = qos;
    mqtt->queue        = g_async_queue_new_full( (GDestroyNotify)Json_unref );
    mqtt->subscribed_topics = NULL;
    g_rw_lock_init(&mqtt->subscribed_topics_lock);

    mqtt->MOSQ_session = mosquitto_new( client_id, TRUE, mqtt );
    if (!mqtt->MOSQ_session)
     { Info( __func__, mqtt->log_facility, mqtt->log_prefixe, LOG_ERR, "Cannot create mosquitto session." );
       Mqtt_stop (mqtt);
       return(NULL);
     }

    mosquitto_log_callback_set        ( mqtt->MOSQ_session, Mqtt_on_log_CB );
    mosquitto_connect_callback_set    ( mqtt->MOSQ_session, Mqtt_on_connect_CB );
    mosquitto_disconnect_callback_set ( mqtt->MOSQ_session, Mqtt_on_disconnect_CB );
    mosquitto_message_callback_set    ( mqtt->MOSQ_session, Mqtt_on_message_CB );
    mosquitto_reconnect_delay_set     ( mqtt->MOSQ_session, 10, 60, TRUE );

    if (is_ssl)
     { gint retour_tls = mosquitto_tls_set( mqtt->MOSQ_session, resolved_ca_file, resolved_ca_path, NULL, NULL, NULL );
       if ( retour_tls != MOSQ_ERR_SUCCESS )
        { Info( __func__, mqtt->log_facility, mqtt->log_prefixe, LOG_ERR, "MQTT TLS setup error: %s", mosquitto_strerror(retour_tls) );
          Mqtt_stop (mqtt);
          return(NULL);
        }
       Info( __func__, mqtt->log_facility, mqtt->log_prefixe, LOG_INFO, "MQTT TLS trust store: cafile='%s', capath='%s'",
             resolved_ca_file ? resolved_ca_file : "", resolved_ca_path ? resolved_ca_path : "" );
     }

    if (username)
     { mosquitto_username_pw_set( mqtt->MOSQ_session, username, password ); }
    mqtt->next_top_connect = time(NULL) + ABLS_MQTT_RECONNECT_DELAY;
    return(mqtt);
  }
/******************************************************************************************************************************/
/* Mqtt_start: Lance la connexion réseau MQTT et la boucle asynchrone                                                         */
/* Entrée: client MQTT initialisé                                                                                             */
/* Sortie: TRUE si démarrage réussi, FALSE sinon                                                                              */
/******************************************************************************************************************************/
 gboolean Mqtt_start ( struct ABLS_MQTT *mqtt )
  { if (!mqtt) return(FALSE);
    Info( __func__, mqtt->log_facility, mqtt->log_prefixe, LOG_NOTICE, "Connecting as '%s' with id '%s' on '%s:%d'.",
          mqtt->username, mqtt->client_id, mqtt->hostname, mqtt->port );

    gboolean retour = mosquitto_connect_async( mqtt->MOSQ_session, mqtt->hostname, mqtt->port, 60 );
    if ( retour != MOSQ_ERR_SUCCESS )
     { Info( __func__, mqtt->log_facility, mqtt->log_prefixe, LOG_ERR, "Connection error %s@%s:%d (client_id=%s): %s",
             mqtt->username, mqtt->hostname, mqtt->port, mqtt->client_id, mosquitto_strerror ( retour ) );
     } /* On poursuit quand même */

    retour = mosquitto_loop_start( mqtt->MOSQ_session );                       /* meme si connect en erreur, on lance la loop */
    if ( retour != MOSQ_ERR_SUCCESS )
     { Info( __func__, mqtt->log_facility, mqtt->log_prefixe, LOG_ERR, "MQTT loop not started: %s", mosquitto_strerror ( retour ) );
       return(FALSE);
     }
    return(TRUE);
  }
/******************************************************************************************************************************/
/* Mqtt_is_connected: Retourne l'etat de connexion MQTT                                                                       */
/* Entrée: client MQTT                                                                                                        */
/* Sortie: TRUE si connecté, FALSE sinon                                                                                      */
/******************************************************************************************************************************/
 gboolean Mqtt_is_connected ( struct ABLS_MQTT *mqtt )
  { if (!mqtt) return(FALSE);
    return(mqtt->connected);
  }
/******************************************************************************************************************************/
/* Mqtt_stop: Arrête la session MQTT et libère toutes les ressources associées                                                */
/* Entrée: client MQTT                                                                                                        */
/* Sortie: Néant                                                                                                              */
/******************************************************************************************************************************/
 void Mqtt_stop ( struct ABLS_MQTT *mqtt )
  { if (!mqtt) return;
    if (mqtt->MOSQ_session)
     { mosquitto_disconnect( mqtt->MOSQ_session );
       mosquitto_loop_stop( mqtt->MOSQ_session, FALSE );
       mosquitto_destroy( mqtt->MOSQ_session );
       mqtt->MOSQ_session = NULL;
     }
    if (mqtt->queue) g_async_queue_unref(mqtt->queue);        /* Fait automatiquement json_unref sur les éléments de la queue */
    if (mqtt->subscribed_topics) g_slist_free_full(mqtt->subscribed_topics, g_free);
    g_rw_lock_clear(&mqtt->subscribed_topics_lock);
    Info( __func__, mqtt->log_facility, mqtt->log_prefixe, LOG_NOTICE, "Disconnected %s@%s:%d (client_id=%s).",
          mqtt->username, mqtt->hostname, mqtt->port, mqtt->client_id );
    g_free ( mqtt->log_facility );
    g_free ( mqtt->log_prefixe );
    g_free ( mqtt->client_id );
    g_free ( mqtt->hostname );
    g_free ( mqtt->username );
    g_free ( mqtt->password );
    g_free(mqtt);
  }
/*----------------------------------------------------------------------------------------------------------------------------*/
