# ABLS-LIBS

Bibliotheque C partagee pour Abls-Habitat.

Le projet contient trois modules metier principaux:

- `info`: journalisation structuree vers syslog
- `json`: helpers de manipulation JSON (json-glib)
- `mqtt`: couche MQTT partagee (libmosquitto)

## Etat du build

Le build CMake actuel compile et installe la librairie avec `info`, `json` et `mqtt`.

- Sources compilees: `src/info.c`, `src/json.c`, `src/mqtt.c`
- Headers installes: `include/info.h`, `include/json.h`, `include/mqtt.h`, `include/abls-libs.h`
- Nom pkg-config: `abls-libs`

## Installation rapide

```sh
./install_deps.sh
./build.sh
sudo ./install.sh
```

## Utilisation via pkg-config

```sh
pkg-config --cflags --libs abls-libs
```

Exemple de compilation:

```sh
cc app.c -o app $(pkg-config --cflags --libs abls-libs)
```

## Module Info

Header: `include/info.h`

Objectif: emission de logs JSON vers syslog, avec filtrage par niveau et forcage debug par `facility`.

### API publique

```c
void Info_init(const gchar *entete, const gchar *prefixe_name, guint log_level);
void Info_change_log_level(guint new_log_level);

void Info(const gchar *function, const gchar *facility, const gchar *prefixe,
                    guint priority, const gchar *format, ...);

gint Info_reset_nbr_log(void);

void Info_debug_facility(const gchar *prefixe, const gchar *context);
void Info_undebug_facility(const gchar *prefixe, const gchar *context);
void Info_clear_debug_facilities(void);
```

### Comportement

- `Info_init` initialise syslog, configure le niveau global et enregistre un cleanup `on_exit`.
- `Info` emet un JSON contenant `thread`, puis optionnellement `facility`, `function`, et la cle de prefixe definie par `prefixe_name`.
- Le message est logge si:
    - son `priority` est <= au niveau courant, ou
    - la `facility` est forcee en debug.
- `Info_reset_nbr_log` retourne le nombre de logs emis depuis le dernier reset et remet le compteur a zero.
- Les facilities forcees debug sont gerees de maniere thread-safe.

### Exemple

```c
#include <syslog.h>
#include <abls-libs/info.h>

Info_init("my-service", "thread_tech_id", LOG_INFO);
Info(__func__, "json", "main", LOG_NOTICE, "Service started");
```

### Format de log

Exemple de payload envoye a syslog:

```json
{
    "thread": "worker-1",
    "thread_tech_id": "main",
    "facility": "json",
    "function": "LoadConfig",
    "message": "Config loaded"
}
```

## Module Json

Header: `include/json.h`

Objectif: faciliter la creation, lecture, extraction et persistance de `JsonNode`.

### API publique (principales fonctions)

Creation et cycle de vie:

```c
JsonNode *Json_create(void);
void Json_unref(JsonNode *RootNode);
JsonNode *Json_get_from_string(const gchar *chaine);
gchar *Json_to_string(JsonNode *RootNode);
```

Ajout de membres:

```c
void Json_add_string(JsonNode *RootNode, gchar *name, const gchar *chaine);
void Json_add_bool(JsonNode *RootNode, gchar *name, gboolean valeur);
void Json_add_double(JsonNode *RootNode, gchar *name, gdouble valeur);
void Json_add_int(JsonNode *RootNode, gchar *name, gint64 valeur);
void Json_add_null(JsonNode *RootNode, gchar *name);
JsonArray *Json_add_array(JsonNode *RootNode, gchar *name);
JsonNode *Json_add_object(JsonNode *RootNode, gchar *name);
```

Accesseurs:

```c
gchar *Json_get_string(JsonNode *RootNode, gchar *chaine);
gdouble Json_get_double(JsonNode *RootNode, gchar *chaine);
gboolean Json_get_bool(JsonNode *RootNode, gchar *chaine);
gint Json_get_int(JsonNode *RootNode, gchar *chaine);
JsonArray *Json_get_array(JsonNode *RootNode, gchar *chaine);
JsonObject *Json_get_object_as_object(JsonNode *RootNode, gchar *chaine);
JsonNode *Json_get_object_as_node(JsonNode *RootNode, gchar *chaine);
gboolean Json_has_member(JsonNode *RootNode, gchar *chaine);
```

Tableaux:

```c
void Json_array_add_element(JsonArray *array, JsonNode *element);
void Json_array_add_one_element(JsonNode *RootNode, gchar *array_name, JsonNode *element);
void Json_array_del_one_element(JsonNode *RootNode, gchar *array_name, guint index);
JsonNode *Json_array_get_element_at(JsonNode *RootNode, gchar *array_name, guint index);
guint Json_array_get_length(JsonNode *RootNode, gchar *array_name);
void Json_foreach_array_element(JsonNode *RootNode, gchar *array_name,
                                                                JsonArrayForeach fonction, gpointer data);
```

Fichiers:

```c
JsonNode *Json_read_from_file(gchar *filename);
gboolean Json_write_to_file(gchar *filename, JsonNode *RootNode);
```

### Regles de memoire

- `Json_create` et `Json_get_from_string` retournent un node a liberer avec `Json_unref`.
- `Json_to_string` retourne une chaine allouee a liberer avec `g_free`.

## Module Config

Header: `include/config.h`

Objectif: charger et fusionner configuration depuis 3 sources: fichier JSON, variables d'environnement (`ABLS_*`), et arguments de ligne de commande (via `GOptionContext`).

### API publique

```c
void Config_apply_FILE(JsonNode *target, const gchar *filename);
void Config_apply_ENV(JsonNode *target);
void Config_add_parameter(const gchar *name, const gchar *description);
void Config_apply_ARGV(JsonNode *target, int *argc, char ***argv);
```

### Comportement

- `Config_apply_FILE` charge un fichier JSON et injecte ses membres dans `target` (écrase les clés existantes).
- `Config_apply_ENV` scanne les variables d'environnement prefixées par `ABLS_` (minuscule apres prefixe dans JSON), avec type-inference (bool/int/string).
- `Config_add_parameter` enregistre dynamiquement les options CLI a accepter en conservant les pointeurs `name` et `description` fournis par l'appelant (pas de duplication).
- `Config_apply_ARGV` reconstruit un tableau `GOptionEntry` temporaire via `GOptionContext`, parse `argc`/`argv`, puis libere ce tableau.

### Regles de memoire

- Fonctions acceptent `target=NULL` en argument et retournent sans effet (safe).
- Pas d'allocation dynamique de retour (void).
- Erreurs loguees via `Info` (facility="config").
- L'appelant conserve la propriete de `name` et `description` passes a `Config_add_parameter`; ces chaines doivent rester valides jusqu'a l'appel de `Config_apply_ARGV`.
- `Config_apply_ARGV` vide le registre interne des parametres, mais ne libere pas les chaines de l'appelant.

### Exemple d'usage

**Cas 1 : Config FILE + ENV (sans options CLI)**:

```c
#include <abls-libs/config.h>
#include <abls-libs/json.h>

int main(void)
{
    JsonNode *cfg = Json_create();

    /* Charger depuis fichier */
    Config_apply_FILE(cfg, "/etc/app/config.json");

    /* Surcharger avec variables d'environnement (ABLS_*) */
    Config_apply_ENV(cfg);

    /* Utiliser cfg... */
    gchar *json_str = Json_to_string(cfg);
    g_print("%s\n", json_str);
    g_free(json_str);

    Json_unref(cfg);
    return 0;
}
```

**Cas 2 : Config FILE + ENV + ARGV (injection JSON)**:

```c
#include <abls-libs/config.h>
#include <abls-libs/json.h>

int main(int argc, char **argv)
{
    JsonNode *cfg = Json_create();

    Config_apply_FILE(cfg, "/etc/app/config.json");
    Config_apply_ENV(cfg);

        /* Options CLI enregistrees dynamiquement avant le parsing */
        Config_add_parameter("verbose", "Verbose mode");
        Config_add_parameter("port", "TCP port");

        Config_apply_ARGV(cfg, &argc, &argv);

    gchar *json_str = Json_to_string(cfg);
    g_print("%s\n", json_str);
    g_free(json_str);

    Json_unref(cfg);
    return 0;
}
```

**Note**: Le callback `Config_argv_callback` injecte automatiquement les valeurs dans `target` si configuré correctement. Les parametres enregistres via `Config_add_parameter(...)` sont consommes par `Config_apply_ARGV(...)`, qui libere uniquement ses structures internes. L'ordre typique est: FILE → ENV → ARGV (chaque étape surcharge la précédente).
- Les accesseurs `Json_get_*` retournent des valeurs liees au `JsonNode` source (ne pas liberer les pointeurs retournes).

### Exemple

```c
#include <abls-libs/json.h>

JsonNode *msg = Json_create();
Json_add_string(msg, "service", "api");
Json_add_int(msg, "status", 200);

gchar *payload = Json_to_string(msg);
g_free(payload);
Json_unref(msg);
```

## Module Mqtt

Header: `include/mqtt.h`

Objectif: encapsuler la session mosquitto, les abonnements, une file asynchrone de messages JSON et la logique de reconnexion.

### API publique

```c
struct ABLS_MQTT *Mqtt_init(const gchar *log_facility, const gchar *log_prefixe,
                                                        const gchar *client_id, gboolean is_ssl,
                                                        const gchar *ca_file, const gchar *ca_path,
                                                        const gchar *username, const gchar *password,
                                                        const gchar *hostname, gint port, gint qos);

gboolean Mqtt_start(struct ABLS_MQTT *mqtt);
void Mqtt_stop(struct ABLS_MQTT *mqtt);

void Mqtt_subscribe(struct ABLS_MQTT *mqtt, gchar *format, ...);
void Mqtt_unsubscribe(struct ABLS_MQTT *mqtt, gchar *format, ...);

gboolean Mqtt_topic_is(JsonNode *request, gint level_count, ...);

void Mqtt_send_message(struct ABLS_MQTT *mqtt, JsonNode *node,
                                             gboolean retain, gchar *topic, ...);
JsonNode *Mqtt_get_message(struct ABLS_MQTT *mqtt);
```

### Comportement attendu

- `Mqtt_init` prepare la session et configure les callbacks mosquitto. Il duplique ses chaines de configuration (`log_facility`, `log_prefixe`, `client_id`, `hostname`, `username`, `password`) avec `g_strdup`, ce qui garantit une mémoire possédée/libérable par le module.
- `Mqtt_start` connecte au broker puis demarre la boucle asynchrone.
- `Mqtt_subscribe` et `Mqtt_unsubscribe` manipulent une liste de topics protegee par lock.
- Les topics stockés dans cette liste sont dupliqués en mémoire (`g_strdup`) pour garantir une copie possédée par le module MQTT (pas une simple recopie de pointeur ou d'un buffer temporaire).
- `Mqtt_topic_is` compare dynamiquement les `mqtt_topic_lvlX` d'un message avec une liste de niveaux attendus.
- `Mqtt_get_message` lit la file de reception sans blocage.
- `Mqtt_send_message` publie un payload JSON et peut conserver en file les messages en echec de publication.

`Mqtt_topic_is` est strict pour chaque niveau non `NULL` (egalite exacte), et ignore les niveaux passes a `NULL`.

Exemple:

```c
/* Controle lvl0 et lvl2, ignore lvl1 */
if (Mqtt_topic_is(msg, 3, "shellies", NULL, "status")) {
    /* ... */
}
```

### Notes importantes

- Le code MQTT present dans `src/mqtt.c` contient des incoherences de nommage et d'integration qui doivent etre stabilisees avant activation dans la cible CMake.
- Tant que `mqtt` n'est pas ajoute a la cible partagee, il ne fait pas partie de la librairie distribuee via `abls-libs`.

## Header parapluie

`include/abls-libs.h` inclut actuellement:

- `info.h`
- `json.h`

Le header `mqtt.h` doit etre inclus explicitement si ce module est utilise hors cible courante.

## Packaging RPM

```sh
cd build && cpack -G RPM
```

Produit:

- `abls-libs-X.Y.Z-1.rpm` (runtime)
- `abls-libs-devel-X.Y.Z-1.rpm` (headers + pkg-config)

## Packaging DEB (Debian/Raspberry)

Install dependencies:

```sh
sudo ./install_deps.sh
```

Build DEB packages:

```sh
./build_apt.sh --dist bookworm
```

Generated packages:

- `abls-libs_VERSION_ARCH.deb` (runtime)
- `abls-libs-dev_VERSION_ARCH.deb` (headers + pkg-config)

Artifacts are copied to:

- `build/deb/<suite>/<arch>/`

`build_apt.sh` builds only the native host architecture.

## Install depuis GitHub Release

```sh
dnf install https://github.com/sebaru/abls-libs/releases/download/v1.0-0/abls-libs-devel-1.0.0-1.rpm
```
