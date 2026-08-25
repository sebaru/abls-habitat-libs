/******************************************************************************************************************************/
/* include/json.h      Declaration des prototypes de manipulation JSON — abls-libs                                           */
/* Projet Abls-Habitat version 1.0       Gestion d'habitat                                                14.06.2026          */
/* Auteur: LEFEVRE Sebastien                                                                                                  */
/******************************************************************************************************************************/
/*
 * json.h
 * This file is part of Abls-Habitat
 *
 * Copyright (C) 1988-2026 - Sebastien LEFEVRE
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

#ifndef _ABLS_JSON_H_
 #define _ABLS_JSON_H_

 #include <glib.h>
 #include <json-glib/json-glib.h>

#define FACILITY_JSON "json"

/*-- Creation / copie / affichage --------------------------------------------------------------------------------------------*/
 extern JsonNode  *Json_create                  ( void );
 extern void       Json_copy_member_into        ( JsonNode *SrcNode, const gchar *name, JsonNode *DestNode );
 extern void       Json_to_log                  ( gchar *log_facility, gchar *log_prefix, JsonNode *RootNode );
 extern void       Json_ref                     ( JsonNode *RootNode );
 extern void       Json_unref                   ( JsonNode *RootNode );
/*-- Construction d'objet JSON -----------------------------------------------------------------------------------------------*/
 extern void       Json_add_string              ( JsonNode *RootNode, gchar *name, const gchar *chaine );
 extern void       Json_add_bool                ( JsonNode *RootNode, gchar *name, gboolean valeur );
 extern void       Json_add_double              ( JsonNode *RootNode, gchar *name, gdouble valeur );
 extern void       Json_add_int                 ( JsonNode *RootNode, gchar *name, gint64 valeur );
 extern void       Json_add_null                ( JsonNode *RootNode, gchar *name );
 extern void       Json_remove                  ( JsonNode *RootNode, gchar *name );
 extern JsonArray *Json_add_array               ( JsonNode *RootNode, gchar *name );
 extern JsonNode  *Json_add_object              ( JsonNode *RootNode, gchar *name );
 extern void       Json_array_add_element       ( JsonArray *array, JsonNode *element );
 extern void       Json_array_add_one_element   ( JsonNode *RootNode, gchar *array_name, JsonNode *element );
 extern void       Json_array_del_one_element   ( JsonNode *RootNode, gchar *array_name, guint index );
 extern JsonNode  *Json_array_get_element_at    ( JsonNode *RootNode, gchar *array_name, guint index );
 extern guint      Json_array_get_length        ( JsonNode *RootNode, gchar *array_name );
 extern void       Json_foreach_array_element   ( JsonNode *RootNode, gchar *array_name,
                                                  JsonArrayForeach fonction, gpointer data );
 extern void       Json_foreach_array_element_by_thread ( JsonNode *RootNode, gchar *array_name,
                                                          GFunc fonction, gpointer fonction_data, guint max_threads );

/*-- Conversion string / parsing ---------------------------------------------------------------------------------------------*/
 extern gchar     *Json_to_string               ( JsonNode *RootNode );
 extern JsonNode  *Json_get_from_string         ( const gchar *chaine );

/*-- Extraction de valeurs ---------------------------------------------------------------------------------------------------*/
 extern gchar     *Json_get_string              ( JsonNode *RootNode, gchar *chaine );
 extern gdouble    Json_get_double              ( JsonNode *RootNode, gchar *chaine );
 extern gboolean   Json_get_bool                ( JsonNode *RootNode, gchar *chaine );
 extern gint       Json_get_int                 ( JsonNode *RootNode, gchar *chaine );
 extern JsonArray *Json_get_array               ( JsonNode *RootNode, gchar *chaine );
 extern JsonObject *Json_get_object_as_object   ( JsonNode *RootNode, gchar *chaine );
 extern JsonNode  *Json_get_object_as_node      ( JsonNode *RootNode, gchar *chaine );
 extern gboolean   Json_has_member              ( JsonNode *RootNode, const gchar *chaine );
 extern guint      Json_array_get_length        ( JsonNode *RootNode, gchar *array_name );
 extern JsonNode  *Json_array_get_element_at    ( JsonNode *RootNode, gchar *array_name, guint index );

/*-- Lecture de fichier / configuration --------------------------------------------------------------------------------------*/
 extern JsonNode  *Json_read_from_file          ( gchar *filename );
 extern gboolean   Json_write_to_file           ( gchar *filename, JsonNode *RootNode );

#endif /* _ABLS_JSON_H_ */
/*----------------------------------------------------------------------------------------------------------------------------*/
