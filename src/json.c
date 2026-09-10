/******************************************************************************************************************************/
/* json.c           Fonctions helper pour la manipulation des payload au format JSON                                          */
/* Projet Abls-Habitat version 4.7       Gestion d'habitat                                      lun 21 avr 2003 22:06:10 CEST */
/* Auteur: LEFEVRE Sebastien                                                                                                  */
/******************************************************************************************************************************/
/*
 * json.c
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

 #include "abls-libs.h"

 #include <sys/types.h>
 #include <sys/stat.h>
 #include <string.h>
 #include <strings.h>
 #include <unistd.h>
 #include <fcntl.h>
 #include <errno.h>

/******************************************************************************************************************************/
/* Json_create: Prepare un RootNode pour creer un nouveau buffer json                                                         */
/* Entrée: néant                                                                                                              */
/* Sortie: NULL si erreur                                                                                                     */
/******************************************************************************************************************************/
 JsonNode *Json_create ( void )
  { JsonNode *RootNode;
    RootNode = json_node_alloc();
    json_node_take_object ( RootNode, json_object_new() );                         /* Création de l'objet de plus haut niveau */
    return(RootNode);
  }
/******************************************************************************************************************************/
/* Json_copy_member_into: Copie un membre d'un node vers un autre                                                             */
/* Entrée: Le source node, le nom du membre, le source destination                                                            */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
 void Json_copy_member_into ( JsonNode *SrcNode, const gchar *name, JsonNode *DestNode )
  { if (!SrcNode || !DestNode)
     { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Node is NULL for '%s'", name ); return; }
    JsonObject *src_obj  = json_node_get_object(SrcNode);                                   /* Récupération de l'objet source */
    if (!src_obj) return;
    JsonObject *dest_obj = json_node_get_object(DestNode);                             /* Récupération de l'objet destination */
    if (!dest_obj) return;
    JsonNode *src_member_node = json_object_get_member( src_obj, name );        /* Récupération du membre name dans l'obj_src */
    if (!src_member_node) return;
    json_object_set_member ( dest_obj, name, json_node_copy(src_member_node) );                  /* Copie dans la destination */
  }
/******************************************************************************************************************************/
/* Json_to_log: Dump JsonNode to log                                                                                          */
/* Entrée: Le node json a dumper                                                                                              */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
 void Json_to_log ( gchar *log_facility, gchar *log_prefix, JsonNode *RootNode )
  { gchar *name;
    JsonObjectIter iter;
    JsonNode *ObjectMemberNode;
    if (!log_facility) log_facility="json";
    if (!RootNode)
     { Info ( __func__, log_facility, log_prefix, LOG_ERR, "RootNode is NULL" );
       return;
     }
    JsonObject *RootObject = json_node_get_object(RootNode);                                /* Récupération de l'objet source */
    json_object_iter_init(&iter, RootObject);
    while (json_object_iter_next(&iter, (const gchar **)&name, &ObjectMemberNode))                        /* Pour tous les membres de l'objet */
     { JsonNodeType value_json_type = json_node_get_node_type ( ObjectMemberNode );
       switch (value_json_type)                                                                     /* Selon le type de noeud */
        { default:
          case JSON_NODE_NULL:
             Info ( __func__, log_facility, log_prefix, LOG_INFO, "%s = 'null'", name );
             break;
          case JSON_NODE_OBJECT:
           { gchar prefix[64];
             g_snprintf ( prefix, sizeof(prefix), "%s.%s", log_prefix, name );
             JsonNode *child_node = Json_get_object_as_node ( RootNode, name );
             Info ( __func__, log_facility, prefix, LOG_INFO, "%s = '{object}'", name );
             Json_to_log ( log_facility, prefix, child_node );
             break;
           }
          case JSON_NODE_ARRAY:
           { JsonArray *array = json_node_get_array(ObjectMemberNode);
             Info ( __func__, log_facility, log_prefix, LOG_INFO, "%s = '[array]'", name );
             if (array)
              { guint index, array_length;
                array_length = json_array_get_length(array);
                gchar prefix[64];
                for (index=0; index<array_length; index++)
                 { g_snprintf ( prefix, sizeof(prefix), "%s[%s].%d", log_prefix, name, index );
                   JsonNode *child_node = json_array_get_element(array, index);
                   Json_to_log ( log_facility, prefix, child_node );
                 }
              }
             break;
           }
          case JSON_NODE_VALUE:
           { GType valueType = json_node_get_value_type( ObjectMemberNode );                       /* Selon le type de valeur */
             switch (valueType)
              { case G_TYPE_INT64:
                  { Info ( __func__, log_facility, log_prefix, LOG_INFO, "%s = '%" G_GINT64_FORMAT "'", name, json_node_get_int(ObjectMemberNode) );
                    break;
                  }
                case G_TYPE_DOUBLE:
                  { Info ( __func__, log_facility, log_prefix, LOG_INFO, "%s = '%f'", name, json_node_get_double(ObjectMemberNode) );
                    break;
                  }
                case G_TYPE_BOOLEAN:
                  { Info ( __func__, log_facility, log_prefix, LOG_INFO, "%s = '%s'", name, ( json_node_get_boolean(ObjectMemberNode) ? "true" : "false") );
                    break;
                  }
                case G_TYPE_STRING:
                  { if (g_strrstr ( name, "password" ) || g_strrstr ( name, "secret" ) || g_strrstr ( name, "token" ) )
                     { Info ( __func__, log_facility, log_prefix, LOG_INFO, "%s = '******'", name ); }
                    else
                     { Info ( __func__, log_facility, log_prefix, LOG_INFO, "%s = '%s'", name, json_node_get_string(ObjectMemberNode) ); }
                  }
                  break;
                default:
                  { Info ( __func__, log_facility, log_prefix, LOG_INFO, "%s = 'unknown value type'", name ); }
              }
             break;
           }
        }
     }
  }
/******************************************************************************************************************************/
/* Json_add_string: Ajoute un enregistrement name/string dans le RootNode                                                     */
/* Entrée: le RootNode, le nom du parametre, la valeur                                                                        */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
 void Json_add_string ( JsonNode *RootNode, gchar *name, const gchar *chaine )
  { if (!RootNode) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Node is NULL for '%s'", name ); return; }
    JsonObject *object = json_node_get_object (RootNode);
    if (chaine) json_object_set_string_member ( object, name, chaine );
           else json_object_set_null_member   ( object, name );
  }
/******************************************************************************************************************************/
/* Json_add_bool: Ajoute un enregistrement name/bool dans le RootNode                                                         */
/* Entrée: le RootNode, le nom du parametre, la valeur                                                                        */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
 void Json_add_bool ( JsonNode *RootNode, gchar *name, gboolean valeur )
  { if (!RootNode) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Node is NULL for '%s'", name ); return; }
    JsonObject *object = json_node_get_object (RootNode);
    json_object_set_boolean_member ( object, name, valeur );
  }
/******************************************************************************************************************************/
/* Json_add_double: Ajoute un enregistrement name/double dans le RootNode                                                     */
/* Entrée: le RootNode, le nom du parametre, la valeur                                                                        */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
 void Json_add_double ( JsonNode *RootNode, gchar *name, gdouble valeur )
  { if (!RootNode) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Node is NULL for '%s'", name ); return; }
    JsonObject *object = json_node_get_object (RootNode);
    json_object_set_double_member ( object, name, valeur );
  }
/******************************************************************************************************************************/
/* Json_add_int: Ajoute un enregistrement name/int dans le RootNode                                                           */
/* Entrée: le RootNode, le nom du parametre, la valeur                                                                        */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
 void Json_add_int ( JsonNode *RootNode, gchar *name, gint64 valeur )
  { if (!RootNode) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Node is NULL for '%s'", name ); return; }
    JsonObject *object = json_node_get_object (RootNode);
    json_object_set_int_member ( object, name, valeur );
  }
/******************************************************************************************************************************/
/* Json_add_null: Ajoute un enregistrement NULL dans le RootNode                                                              */
/* Entrée: le RootNode, le nom du parametre, la valeur                                                                        */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
 void Json_add_null ( JsonNode *RootNode, gchar *name )
  { if (!RootNode) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Node is NULL for '%s'", name ); return; }
    JsonObject *object = json_node_get_object (RootNode);
    json_object_set_null_member   ( object, name );
  }
/******************************************************************************************************************************/
/* Json_remove: Supprime un membre du RootNode                                                                                */
/* Entrée: le RootNode, le nom du parametre                                                                                   */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
 void Json_remove ( JsonNode *RootNode, gchar *name )
  { if (!RootNode) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Node is NULL for '%s'", name ); return; }
    JsonObject *object = json_node_get_object (RootNode);
    if (!object) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Object is null for '%s'", name ); return; }
    json_object_remove_member ( object, name );
  }
/******************************************************************************************************************************/
/* Json_add_array: Ajoute un enregistrement name/array dans le RootNode                                                       */
/* Entrée: le RootNode, le nom du parametre, la valeur                                                                        */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
 JsonArray *Json_add_array ( JsonNode *RootNode, gchar *name )
  { if (!RootNode) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Node is NULL for '%s'", name ); return(NULL); }
    JsonObject *object = json_node_get_object (RootNode);
    JsonArray *tableau = json_array_new();
    json_object_set_array_member ( object, name, tableau );
    return(tableau);
  }
/******************************************************************************************************************************/
/* Json_add_object: Ajoute un enregistrement name/object dans le RootNode                                                     */
/* Entrée: le RootNode, le nom du parametre, la valeur                                                                        */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
 JsonNode *Json_add_object ( JsonNode *RootNode, gchar *name )
  { if (!RootNode) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Node is NULL for '%s'", name ); return(NULL); }
    JsonObject *RootObject = json_node_get_object (RootNode);
    JsonNode *new_node = json_node_alloc();
    json_node_set_object ( new_node, json_object_new() );
    json_object_set_member ( RootObject, name, new_node );
    return(new_node);
  }
/******************************************************************************************************************************/
/* Json_array_add_element: Ajoute un enregistrement dans le tableau                                                           */
/* Entrée: le tableau, l'élément à ajouter                                                                                    */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
 void Json_array_add_element ( JsonArray *array, JsonNode *element )
  { if (!array) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Array is NULL" ); return; }
    json_array_add_element ( array, element );
  }
/******************************************************************************************************************************/
/* Json_array_add_one_element: Ajoute un enregistrement dans le tableau nommé en paramètre                                    */
/* Entrée: le RootNode, le nom du parametre, la valeur                                                                        */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
 void Json_array_add_one_element ( JsonNode *RootNode, gchar *array_name, JsonNode *element )
  { if (!RootNode) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Node is NULL for '%s'", array_name ); return; }
    JsonArray *tableau = Json_get_array ( RootNode, array_name );
    if (tableau) json_array_add_element ( tableau, element );
  }
/******************************************************************************************************************************/
/* Json_array_del_one_element: Supprime un enregistrement dans le tableau nommé en paramètre                                  */
/* Entrée: le RootNode, le nom du parametre, l'index de l'élément à supprimer                                                 */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
 void Json_array_del_one_element ( JsonNode *RootNode, gchar *array_name, guint index )
  { if (!RootNode) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Node is NULL for '%s'", array_name ); return; }
    JsonArray *tableau = Json_get_array ( RootNode, array_name );
    if (tableau) json_array_remove_element ( tableau, index );
  }
/******************************************************************************************************************************/
/* Json_array_get_element: Recupere un element du tableau nommé en paramètre                                                  */
/* Entrée: le RootNode, le nom du tableau, l'index                                                                            */
/* Sortie: le JsonNode demandé ou NULL                                                                                        */
/******************************************************************************************************************************/
 JsonNode *Json_array_get_element_at ( JsonNode *RootNode, gchar *array_name, guint index )
  { if (!RootNode) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Node is NULL for '%s'", array_name ); return(NULL); }
    JsonArray *tableau = Json_get_array ( RootNode, array_name );
    if (!tableau) return(NULL);
    return(json_array_get_element ( tableau, index ));
  }
/******************************************************************************************************************************/
/* Json_array_get_length: Recupere la taille du tableau nommé en paramètre                                                    */
/* Entrée: le RootNode, le nom du tableau                                                                                     */
/* Sortie: la taille du tableau, 0 si absent                                                                                  */
/******************************************************************************************************************************/
 guint Json_array_get_length ( JsonNode *RootNode, gchar *array_name )
  { if (!RootNode) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Node is NULL for '%s'", array_name ); return(0); }
    JsonArray *tableau = Json_get_array ( RootNode, array_name );
    if (!tableau) return(0);
    return(json_array_get_length ( tableau ));
  }
/******************************************************************************************************************************/
/* Json_foreach_array_element: Lance une fonction en parametre sur chacun des elements d'un tableau                           */
/* Entrée: le RootNode, le nom du parametre, la fonction et les donnees utilisateur                                           */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
 void Json_foreach_array_element ( JsonNode *RootNode, gchar *array_name, JsonArrayForeach fonction, gpointer data )
  { if (!RootNode) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Node is NULL for '%s'", array_name ); return; }
    json_array_foreach_element ( Json_get_array ( RootNode, array_name ), fonction, data );
  }
/******************************************************************************************************************************/
/* Json_foreach_array_element_by_thread_start_one: Ajoute un élément du tableau au pool de threads après y définir son index  */
/* Entrée: le tableau, l'index de l'élément, l'élément JSON et le pool de threads                                             */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
 static void Json_foreach_array_element_by_thread_start_one ( JsonArray *array, guint index, JsonNode *element, gpointer user_data )
  { struct ABLS_RUN_POOL *pool = user_data;
    Json_add_int ( element, "index", index );
    Run_thread_pool_push ( pool, element );
  }
/******************************************************************************************************************************/
/* Json_foreach_array_element_by_thread: Parcourt un tableau JSON et soumet chaque élément à un pool de threads               */
/* Entrée: le RootNode, le nom du tableau, la fonction à exécuter, les données utilisateur et le nombre maximum de threads    */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
 void Json_foreach_array_element_by_thread ( JsonNode *RootNode, gchar *array_name,
                                             GFunc fonction, gpointer fonction_data, guint max_threads )
  { if (!RootNode) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Node is NULL for '%s'", array_name ); return; }
    JsonArray *array = Json_get_array ( RootNode, array_name );
    if (!array) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Array is NULL for '%s'", array_name ); return; }

    struct ABLS_RUN_POOL *pool = Run_thread_pool_init ( "json_array_by_thread", fonction, fonction_data, max_threads );
    if (!pool) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Unable to allocate memory for threads" ); return; }
    Json_foreach_array_element ( RootNode, array_name, Json_foreach_array_element_by_thread_start_one, pool );
    Run_thread_pool_end ( pool, TRUE );
  }
/******************************************************************************************************************************/
/* Json_to_string: transforme un JsonNode en string                                                                           */
/* Entrée: le JsonNode a convertir                                                                                            */
/* Sortie: un nouveau buffer                                                                                                  */
/******************************************************************************************************************************/
 gchar *Json_to_string ( JsonNode *RootNode )
  { return ( json_to_string ( RootNode, FALSE ) ); }
/******************************************************************************************************************************/
/* Json_to_pretty_string: transforme un JsonNode en string avec une mise en forme lisible                                     */
/* Entrée: le JsonNode a convertir                                                                                            */
/* Sortie: un nouveau buffer                                                                                                  */
/******************************************************************************************************************************/
 gchar *Json_to_pretty_string ( JsonNode *RootNode )
  { return ( json_to_string ( RootNode, TRUE ) ); }
/******************************************************************************************************************************/
/* Json_get_from_string: Recupere l'object de plus haut niveau dans une chaine JSON                                           */
/* Entrée: la chaine de caractere                                                                                             */
/* Sortie: l'objet                                                                                                            */
/******************************************************************************************************************************/
 JsonNode *Json_get_from_string ( const gchar *chaine )
  { return(json_from_string ( chaine, NULL )); }
/******************************************************************************************************************************/
/* Json_get_string: Recupere la chaine de caractere dont le nom est en parametre                                              */
/* Entrée: le RootNode, le nom du parametre                                                                                   */
/* Sortie: la chaine de caractere                                                                                             */
/******************************************************************************************************************************/
 gchar *Json_get_string ( JsonNode *RootNode, gchar *chaine )
  { if (!RootNode) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Node is NULL for '%s'", chaine ); return(NULL); }
    JsonObject *object = json_node_get_object (RootNode);
    if (!object) { Info ( __func__, "json", NULL, LOG_ERR, "Object is null for '%s'", chaine );  return(NULL); }
    return((gchar *)json_object_get_string_member ( object, chaine ));
  }
/******************************************************************************************************************************/
/* Json_get_double: Recupere la valeur double dont le nom est en parametre                                                    */
/* Entrée: le RootNode, le nom du parametre                                                                                   */
/* Sortie: la valeur double                                                                                                   */
/******************************************************************************************************************************/
 gdouble Json_get_double ( JsonNode *RootNode, gchar *chaine )
  { if (!RootNode) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Node is NULL for '%s'", chaine ); return(0.0); }
    JsonObject *object = json_node_get_object (RootNode);
    if (!object) { Info ( __func__, "json", NULL, LOG_ERR, "Object is null for '%s'", chaine );  return(0.0); }
    return(json_object_get_double_member ( object, chaine ));
  }
/******************************************************************************************************************************/
/* Json_get_bool: Recupere la valeur booléenne dont le nom est en parametre                                                   */
/* Entrée: le RootNode, le nom du parametre                                                                                   */
/* Sortie: la valeur booléenne                                                                                                */
/******************************************************************************************************************************/
 gboolean Json_get_bool ( JsonNode *RootNode, gchar *chaine )
  { if (!RootNode) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Node is NULL for '%s'", chaine ); return(FALSE); }
    JsonObject *object = json_node_get_object (RootNode);
    if (!object) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Object is null for '%s'", chaine );  return(FALSE); }
    return(json_object_get_boolean_member ( object, chaine ));
  }
/******************************************************************************************************************************/
/* Json_get_int: Recupere l'entier dont le nom est en parametre                                                               */
/* Entrée: le RootNode, le nom du parametre                                                                                   */
/* Sortie: la valeur entière                                                                                                  */
/******************************************************************************************************************************/
 gint Json_get_int ( JsonNode *RootNode, gchar *chaine )
  { if (!RootNode) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Node is NULL for '%s'", chaine ); return(0); }
    JsonObject *object = json_node_get_object (RootNode);
    if (!object) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Object is null for '%s'", chaine );  return(0); }
    return(json_object_get_int_member ( object, chaine ));
  }
/******************************************************************************************************************************/
/* Json_get_array: Recupere le tableau dont le nom est en parametre                                                           */
/* Entrée: le RootNode, le nom du parametre                                                                                   */
/* Sortie: le tableau                                                                                                         */
/******************************************************************************************************************************/
 JsonArray *Json_get_array ( JsonNode *RootNode, gchar *chaine )
  { if (!RootNode) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Node is NULL for '%s'", chaine ); return(NULL); }

    JsonObject *object = json_node_get_object (RootNode);
    if (!object) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Object is null for '%s'", chaine );  return(NULL); }
    return(json_object_get_array_member ( object, chaine ));
  }
/******************************************************************************************************************************/
/* Json_get_object_as_object: Recupere l'objet dont le nom est en parametre                                                   */
/* Entrée: le RootNode, le nom du parametre                                                                                   */
/* Sortie: l'objet                                                                                                            */
/******************************************************************************************************************************/
 JsonObject *Json_get_object_as_object ( JsonNode *RootNode, gchar *chaine )
  { if (!RootNode)
     { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Node is NULL for '%s'", chaine ); return(NULL); }
    JsonObject *object = json_node_get_object (RootNode);
    if (!object) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Object is null for '%s'", chaine );  return(NULL); }
    return(json_object_get_object_member ( object, chaine ));
  }
/******************************************************************************************************************************/
/* Json_get_object_as_node: Recupere le node dont le nom est en parametre                                                     */
/* Entrée: le RootNode, le nom du parametre                                                                                   */
/* Sortie: le node                                                                                                            */
/******************************************************************************************************************************/
 JsonNode *Json_get_object_as_node ( JsonNode *RootNode, gchar *chaine )
  { if (!RootNode)
     { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Node is NULL for '%s'", chaine ); return(NULL); }
    JsonObject *object = json_node_get_object (RootNode);
    if (!object) { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Object is null for '%s'", chaine );  return(NULL); }
    return(json_object_get_member ( object, chaine ));
  }
/******************************************************************************************************************************/
/* Json_has_member: Verifie si un membre existe dans l'objet JSON                                                             */
/* Entrée: le RootNode, le nom du parametre                                                                                   */
/* Sortie: TRUE si le membre existe, FALSE sinon                                                                              */
/******************************************************************************************************************************/
 gboolean Json_has_member ( JsonNode *RootNode, const gchar *chaine )
  { if (!RootNode)
     { Info ( __func__, "json", NULL, LOG_ERR, "RootNode is null for '%s'", chaine );  return(FALSE); }
    JsonObject *object = json_node_get_object (RootNode);
    if (!object)
     { Info ( __func__, "json", NULL, LOG_ERR, "Object is null for '%s'", chaine );  return(FALSE); }
    if (!json_object_has_member ( object, chaine ))
     { Info ( __func__, "json", NULL, LOG_DEBUG, "%s is missing", chaine ); return(FALSE); }
    if (json_object_get_null_member ( object, chaine ))
     { Info ( __func__, "json", NULL, LOG_DEBUG, "%s is null", chaine ); return(FALSE); }
    return( TRUE );
  }
/******************************************************************************************************************************/
/* Json_read_from_file: Recupere un ficher et le lit au format Json                                                           */
/* Entrée: le nom de fichier                                                                                                  */
/* Sortie: le buffer JsonNode                                                                                                 */
/******************************************************************************************************************************/
 JsonNode *Json_read_from_file ( gchar *filename )
  { struct stat stat_buf;
    if (stat ( filename, &stat_buf)==-1) return(NULL);

    JsonNode *node = NULL;
    gchar *content = g_try_malloc0 ( stat_buf.st_size+1 );
    if (!content) return(NULL);

    gint fd = open ( filename, O_RDONLY );
    if (fd < 0)
     { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Unable to open json file %s: %s", filename, strerror(errno) );
       goto end;
     }

    if (read ( fd, content, stat_buf.st_size ) == stat_buf.st_size)
     { node = Json_get_from_string ( content );
       if (!node) Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Unable to parse: file %s is not JSON", filename );
     } else Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Unable to read json file %s: %s", filename, strerror(errno) );
    close(fd);
end:
    g_free(content);
    return(node);
  }
/******************************************************************************************************************************/
/* Json_write_to_file: Sauvegarde un JsonNode dans un fichier                                                                 */
/* Entrée: le nom de fichier et le buffer Json                                                                                */
/* Sortie: FALSE si erreur                                                                                                    */
/******************************************************************************************************************************/
 gboolean Json_write_to_file ( gchar *filename, JsonNode *RootNode )
  { unlink ( filename );
    gint fd = creat ( filename, S_IWUSR | S_IRUSR );
    if (fd < 0)
     { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Create '%s' failed: %s", filename, strerror(errno) );
       return(FALSE);
     }

    gchar *buf = Json_to_pretty_string ( RootNode );
    if (!buf)
     { close(fd);
       Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Json to Buf failed, writing to '%s'", filename );
       return(FALSE);
     }

    gint taille = strlen(buf);
    gboolean retour = TRUE;
    if (write ( fd, buf, taille ) != taille)
     { Info ( __func__, FACILITY_JSON, NULL, LOG_ERR, "Error writing %d bytes to '%s': %s", taille, filename, strerror(errno) );
       retour = FALSE;
     }

    close(fd);
    g_free(buf);
    return(retour);
  }
/******************************************************************************************************************************/
/* Json_ref: Ajoute une reference sur un JsonNode                                                                             */
/* Entrée: le JsonNode à référencer                                                                                           */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
  void Json_ref ( JsonNode *RootNode )
  { if (RootNode) json_node_ref ( RootNode ); }
/******************************************************************************************************************************/
/* Json_unref: Libère un JsonNode                                                                                             */
/* Entrée: le JsonNode à libérer                                                                                              */
/* Sortie: néant                                                                                                              */
/******************************************************************************************************************************/
  void Json_unref ( JsonNode *RootNode )
  { if (RootNode) json_node_unref ( RootNode ); }
/*----------------------------------------------------------------------------------------------------------------------------*/