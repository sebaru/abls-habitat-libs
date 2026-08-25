/******************************************************************************************************************************/
/* src/config.c         Parsage de configuration depuis FILE, ENV, ARGV — ABLS-LIBS                                           */
/* Projet Abls-Habitat                   Gestion d'habitat                                      sam 04 jul 2026 18:28:10 CEST */
/* Auteur: LEFEVRE Sebastien                                                                                                  */
/******************************************************************************************************************************/
/*
 * config.c
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

 #include <string.h>
 #include <strings.h>
 #include "abls-libs.h"

 struct ABLS_CONFIG_PARAMETER
  { const gchar *name;
    const gchar *description;
    const gchar *arg_description;
    ABLS_CONFIG_PARAMETER_TYPE type;
    gboolean     valeur_flag;
    gchar       *valeur_string;
    gint         valeur_int;
  };

 static GSList *Config_parameters = NULL;

/******************************************************************************************************************************/
/* Config_clear_parameters: Libere le registre statique des parametres CLI                                                   */
/* Entrée: néant                                                                                                              */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
 static void Config_clear_parameters ( void )
  { g_slist_free_full ( Config_parameters, g_free );
    Config_parameters = NULL;
  }
/******************************************************************************************************************************/
/* Config_build_entries: Construit et retourne un tableau temporaire de GOptionEntry depuis le registre statique              */
/* Entrée: néant                                                                                                              */
/* Sortie: pointeur vers le tableau alloue, ou NULL en cas d'erreur d'allocation                                             */
/* Note: le tableau retourne doit etre libere avec g_free()                                                                  */
/******************************************************************************************************************************/
 static GOptionEntry *Config_build_entries ( void )
  { GOptionEntry *entries;

    entries = g_new0 ( GOptionEntry, g_slist_length ( Config_parameters ) + 1 );
    if (!entries)
     { Info ( __func__, FACILITY_CONFIG, NULL, LOG_ALERT, "Memory error trying to allocate GOptionEntry array" );
       return(NULL);
     }

    guint index = 0;
    GSList *liste = Config_parameters;
    while( liste )
     { struct ABLS_CONFIG_PARAMETER *parameter = liste->data;
       entries[index].long_name = parameter->name;
       entries[index].short_name = 0;
       entries[index].flags = 0;
       switch ( parameter->type )
        { case CONFIG_BOOL   :
          case CONFIG_FLAG   : entries[index].arg = G_OPTION_ARG_NONE;
                               entries[index].arg_data = (gpointer)&parameter->valeur_flag;
                               break;
          case CONFIG_STRING : entries[index].arg = G_OPTION_ARG_STRING;
                               entries[index].arg_data = (gpointer)&parameter->valeur_string;
                               break;
          case CONFIG_INT    : entries[index].arg = G_OPTION_ARG_INT;
                               entries[index].arg_data = (gpointer)&parameter->valeur_int;
                               break;
          default            : Info ( __func__, FACILITY_CONFIG, NULL, LOG_ERR,
                                     "Unknown parameter type for '%s', skipping", parameter->name );
                               break;}
       entries[index].description = parameter->description;
       entries[index].arg_description = parameter->arg_description;
       liste = liste->next;
       index++;
     }
    return(entries);
  }
/******************************************************************************************************************************/
/* Config_add_parameter: Enregistre une option CLI pour le prochain parsing ARGV                                              */
/* Entrée: name (nom long de l'option), arg_description (description de l'argument), description (texte d'aide associee),     */
/*         type (type attendu de la valeur, ou flag sans valeur)                                                              */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
 void Config_add_parameter ( const gchar *name, const gchar *arg_description, const gchar *description, ABLS_CONFIG_PARAMETER_TYPE type )
  { if (!name)
     { Info ( __func__, FACILITY_CONFIG, NULL, LOG_WARNING, "Invalid empty parameter name, skipping registration" );
       return;
     }

    struct ABLS_CONFIG_PARAMETER *parameter = g_try_malloc0 ( sizeof ( struct ABLS_CONFIG_PARAMETER ) );
    if (!parameter)
     { Info ( __func__, FACILITY_CONFIG, NULL, LOG_ALERT, "Memory error trying to allocate parameter structure" );
       return;
     }

    parameter->name            = name;                                                                 /* Ajout dans la liste */
    parameter->arg_description = arg_description;
    parameter->description     = description;
    parameter->type            = type;
    Config_parameters = g_slist_append ( Config_parameters, parameter );
  }
/******************************************************************************************************************************/
/* Config_apply_FILE: Charge configuration depuis fichier JSON                                                                */
/* Entrée: target (JsonNode a remplir), filename (chemin du fichier)                                                          */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
 void Config_apply_FILE ( JsonNode *target, const gchar *filename )
  { const char *name;
    JsonObjectIter iter;
    JsonNode *ObjectMemberNode;

    if (!target || !filename) return;

    Info ( __func__, FACILITY_CONFIG, NULL, LOG_INFO, "Trying to read config file '%s'", filename );
    JsonNode *from_file = Json_read_from_file ( (gchar *)filename );
    if (from_file)                                                              /* Copy des elements de from_file vers target */
     { JsonObject *fromFileObject = json_node_get_object(from_file);                        /* Récupération de l'objet source */
       json_object_iter_init(&iter, fromFileObject);
       while (json_object_iter_next(&iter, &name, &ObjectMemberNode))
        { Json_copy_member_into ( from_file, name, target ); }
       Json_unref( from_file );
     } else Info ( __func__, FACILITY_CONFIG, NULL, LOG_WARNING, "Unable to read file config '%s'", filename );
  }
/******************************************************************************************************************************/
/* Config_apply_FILE_if_missing: Charge configuration depuis fichier JSON si les clés sont manquantes                         */
/* Entrée: target (JsonNode a remplir), filename (chemin du fichier)                                                          */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
 void Config_apply_FILE_if_missing ( JsonNode *target, const gchar *filename )
  { const gchar *name;
    JsonObjectIter iter;
    JsonNode *ObjectMemberNode;

    if (!target || !filename) return;

    Info ( __func__, FACILITY_CONFIG, NULL, LOG_INFO, "Trying to read config file '%s'", filename );
    JsonNode *from_file = Json_read_from_file ( (gchar *)filename );
    if (from_file)                                                              /* Copy des elements de from_file vers target */
     { JsonObject *fromFileObject = json_node_get_object(from_file);                        /* Récupération de l'objet source */
       json_object_iter_init(&iter, fromFileObject);
       while (json_object_iter_next(&iter, &name, &ObjectMemberNode))
        { if (!Json_has_member(target, name)) Json_copy_member_into ( from_file, name, target ); }
       Json_unref( from_file );
     } else Info ( __func__, FACILITY_CONFIG, NULL, LOG_WARNING, "Unable to read file config '%s'", filename );
  }
/******************************************************************************************************************************/
/* Config_apply_ENV: Applique variables d'environnement ABLS_* dans le JSON target                                           */
/* Entrée: target (JsonNode a remplir)                                                                                        */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
 void Config_apply_ENV ( JsonNode *target )
  { const gchar *valeur;
    gchar **env_vars, **env;
    gchar *env_name;

    if (!target) return;

    Info ( __func__, FACILITY_CONFIG, NULL, LOG_INFO, "Apply ENVironment Variables" );
    env_vars = g_listenv();                                               /* Récupérer la liste des variables d'environnement */
    for (env = env_vars; *env != NULL; env++)                                               /* Parcourir toutes les variables */
     { const gchar *prefixe = "ABLS_";
       if (g_str_has_prefix(*env, prefixe))                                   /* Vérifier si la variable commence par "ABLS_" */
        { valeur = g_getenv ( *env );                                                                    /* Extrait la valeur */
          if (valeur)
           { env_name = g_ascii_strdown( *env + strlen(prefixe), -1 );                                /* Passage en lowercase */
             Info ( __func__, FACILITY_CONFIG, NULL, LOG_NOTICE, "Apply ENV '%s' -> '%s' = '%s'", *env, env_name, valeur );
                  if ( !strcasecmp ( valeur, "TRUE"  ) ) { Json_add_bool ( target, env_name, TRUE ); }
             else if ( !strcasecmp ( valeur, "FALSE" ) ) { Json_add_bool ( target, env_name, FALSE ); }
             else
              { gchar *endptr = NULL;                 /* Convert only strict integers; keep values like 127.0.0.1 as strings. */
                g_ascii_strtoll ( valeur, &endptr, 10 );
                if (endptr && *endptr == '\0' && endptr != valeur)
                 { Json_add_int  ( target, env_name, atoi(valeur) ); }
                else Json_add_string ( target, env_name, valeur );                        /* Sinon d'une chaine de caracteres */
              }
             g_free(env_name);
           }
        }
     }
  }
/******************************************************************************************************************************/
/* Config_apply_ARGV: Parse argc/argv via GOptionContext                                                                     */
/* Entrée: target (JsonNode a remplir), argc/argv pointers                                                                    */
/* Sortie: néant (GError loggue si parsing echoue)                                                                            */
/* NOTE: Les options sont construites dynamiquement depuis le registre Config_add_parameter()                                 */
/******************************************************************************************************************************/
 void Config_apply_ARGV ( JsonNode *target, gint argc, gchar **argv )
  { GOptionEntry *entries = NULL;
    GOptionContext *ctx = NULL;
    GError *error = NULL;
    gboolean is_help_requested = FALSE;

    Config_add_parameter ( "help", NULL, "Display this help", CONFIG_FLAG );          /* Ajout d'une option d'aide par défaut */
    entries = Config_build_entries ( );
    if (!entries)
     { Info ( __func__, FACILITY_CONFIG, NULL, LOG_WARNING, "Invalid entries, skipping" );
       goto end;
     }

    if (!target || argc <= 0 || !argv)
     { Info ( __func__, FACILITY_CONFIG, NULL, LOG_WARNING, "Invalid ARGV parsing context, skipping" );
       goto end;
     }

    Info ( __func__, FACILITY_CONFIG, NULL, LOG_NOTICE, "Apply Command-Line Arguments" );

    ctx = g_option_context_new ( "- ABLS Configuration" );                                          /* Créer contexte GOption */
    if (!ctx)
     { Info ( __func__, FACILITY_CONFIG, NULL, LOG_ERR, "Unable to create GOptionContext" );
       goto end;
     }

    g_option_context_set_help_enabled(ctx, FALSE);
    GOptionGroup *group = g_option_group_new ( "abls", "ABLS options", "Show ABLS options", target, NULL );
    g_option_group_add_entries ( group, entries );
    g_option_context_set_main_group ( ctx, group );

    if (!g_option_context_parse ( ctx, &argc, &argv, &error ))                                                      /* Parser */
     { Info ( __func__, FACILITY_CONFIG, NULL, LOG_WARNING, "ARGV parsing failed: %s", error->message );
       goto end;
     }

    GSList *liste = Config_parameters;                                                   /* Ajout des parametres dans le json */
    while( liste )
     { struct ABLS_CONFIG_PARAMETER *parameter = liste->data;
       gchar *name = g_strdup ( parameter->name );
       if (!name)
        { Info ( __func__, FACILITY_CONFIG, NULL, LOG_ERR, "Unable to convert parameter name '%s' to JSON name, skipping", parameter->name ); }
       else
        { g_strdelimit ( name, "-", '_' );                                        /* Convertir le nom de l'option en nom JSON */
          switch (parameter->type)
           { case CONFIG_FLAG :
             case CONFIG_BOOL :
              { if (parameter->valeur_flag) { Json_add_bool(target, name, TRUE); }
                break;
              }
             case CONFIG_STRING :
              { if (parameter->valeur_string) { Json_add_string(target, name, parameter->valeur_string); }
                break;
              }
             case CONFIG_INT :
              { if (parameter->valeur_int != 0) Json_add_int(target, name, parameter->valeur_int);
                break;
              }
             default : Info ( __func__, FACILITY_CONFIG, NULL, LOG_ERR,
                               "Unknown parameter type for '%s', skipping", parameter->name );
           }
          g_free(name);
        }
       liste = liste->next;
     }

    is_help_requested = Json_has_member(target, "help");                                                 /* si demande d'aide */
    if (is_help_requested)
     { gchar *help_text = g_option_context_get_help ( ctx, TRUE, NULL );
       g_print ( "%s", help_text );
       g_free(help_text);
     }

end:
    if (error) g_error_free ( error );
    if (ctx) g_option_context_free ( ctx );
    if (entries) g_free ( entries );
    Config_clear_parameters ( );

    if (is_help_requested) exit(0);                                  /* si "--help", afficher l'aide et quitter */
  }
/*----------------------------------------------------------------------------------------------------------------------------*/
