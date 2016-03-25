﻿/*
 * CLAP - CLever Audio Plugin
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Copyright (c) 2014...2016 Alexandre BIQUE <bique.alexandre@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef CLAP_H
# define CLAP_H

# ifdef __cplusplus
extern "C" {
# endif

# include <stddef.h>
# include <stdbool.h>
# include <stdint.h>

# define CLAP_VERSION_MAKE(Major, Minor, Revision) \
  ((((Major) & 0xff) << 16) | (((Minor) & 0xff) << 8) | ((Revision) & 0xff))
# define CLAP_VERSION CLAP_VERSION_MAKE(0, 2, 0)
# define CLAP_VERSION_MAJ(Version) (((Version) >> 16) & 0xff)
# define CLAP_VERSION_MIN(Version) (((Version) >> 8) & 0xff)
# define CLAP_VERSION_REV(Version) ((Version) & 0xff)

#if defined _WIN32 || defined __CYGWIN__
# ifdef __GNUC__
#  define CLAP_EXPORT __attribute__ ((dllexport))
# else
#  define CLAP_EXPORT __declspec(dllexport)
# endif
#else
# if __GNUC__ >= 4
#  define CLAP_EXPORT __attribute__ ((visibility ("default")))
# else
#  define CLAP_EXPORT
# endif
#endif

///////////////////////////
// FORWARD DELCLARATIONS //
///////////////////////////

struct clap_plugin;
struct clap_host;

enum clap_string_size
{
  CLAP_ID_SIZE         = 64,
  CLAP_NAME_SIZE       = 64,
  CLAP_DESC_SIZE       = 256,
  CLAP_DISPLAY_SIZE    = 64,
  CLAP_TAGS_SIZE       = 256,
};

enum clap_log_severity
{
  CLAP_LOG_DEBUG   = 0,
  CLAP_LOG_INFO    = 1,
  CLAP_LOG_WARNING = 2,
  CLAP_LOG_ERROR   = 3,
  CLAP_LOG_FATAL   = 4,
};

# define CLAP_ATTR_ID              "clap/id"
# define CLAP_ATTR_NAME            "clap/name"
# define CLAP_ATTR_DESCRIPTION     "clap/description"
# define CLAP_ATTR_VERSION         "clap/version"
# define CLAP_ATTR_MANUFACTURER    "clap/manufacturer"
# define CLAP_ATTR_URL             "clap/url"
# define CLAP_ATTR_SUPPORT         "clap/support"
# define CLAP_ATTR_LICENSE         "clap/license"
# define CLAP_ATTR_CATEGORIES      "clap/categories"
# define CLAP_ATTR_TYPE            "clap/type"
# define CLAP_ATTR_CHUNK_SIZE      "clap/chunk_size"
// Should be "1" if the plugin supports tunning.
# define CLAP_ATTR_SUPPORTS_TUNING "clap/supports_tuning"
// Should be "1" if the plugin is doing remote processing.
// This is a hint for the host to optimize task scheduling.
# define CLAP_ATTR_IS_REMOTE_PROCESSING "clap/is_remote_processing"
// Should be "1" if the plugin supports in place processing.
# define CLAP_ATTR_SUPPORTS_IN_PLACE_PROCESSING "clap/supports_in_place_processing"

# define CLAP_EVENT_TYPE_SPACE    0
# define CLAP_EVENT_TYPE_SPACE_ID "clap/events"

////////////////
// PARAMETERS //
////////////////

union clap_param_value
{
  bool    b;
  float   f;
  int32_t i;
};

////////////
// EVENTS //
////////////

enum clap_event_type
{
  CLAP_EVENT_NOTE_ON    = 0,    // note attribute
  CLAP_EVENT_NOTE_OFF   = 1,    // note attribute
  CLAP_EVENT_NOTE_CHOKE = 2,    // note attribute

  CLAP_EVENT_PARAM_SET  = 3,    // param attribute
  CLAP_EVENT_PARAM_RAMP = 4,    // param attribute

  CLAP_EVENT_CONTROL    = 5,    // control attribute
  CLAP_EVENT_MIDI       = 6,    // midi attribute

  CLAP_EVENT_PLAY  = 12, // no attribute
  CLAP_EVENT_PAUSE = 13, // no attribute
  CLAP_EVENT_STOP  = 14, // no attribute
  CLAP_EVENT_JUMP  = 15,  // attribute jump
};

struct clap_event_param
{
  /* key/voice index */
  int8_t                  key;
  int8_t                  channel;

  /* parameter */
  int32_t                 index; // parameter index
  union clap_param_value  value;
  float                   increment;        // for param ramp
};

struct clap_event_note
{
  int8_t  key;
  int8_t  channel;
  float   pitch;
  float   velocity; // 0..1
};

struct clap_event_control
{
  int8_t key;
  int8_t channel;
  int8_t control;
  float  value; // 0..1
};

struct clap_event_midi
{
  /* midi event */
  const uint8_t *buffer;
  int32_t        size;
};

struct clap_event_jump
{
  int32_t tempo;      // tempo in samples
  int32_t bar;        // the bar number
  int32_t bar_offset; // 0 <= bar_offset < tsig_denom * tempo
  int32_t tsig_num;   // time signature numerator
  int32_t tsig_denom; // time signature denominator
};

struct clap_event
{
  struct clap_event    *next; // linked list, NULL on end
  enum clap_event_type  type;
  int64_t               time; // offset from the first sample in the process block

  union {
    struct clap_event_note        note;
    struct clap_event_control     control;
    struct clap_event_param       param;
    struct clap_event_midi        midi;
    struct clap_event_jump	  jump;
  };
};

/////////////
// PROCESS //
/////////////

enum clap_process_status
{
  /* Processing failed. The output buffer must be discarded. */
  CLAP_PROCESS_ERROR    = 0,

  /* Processing succeed. */
  CLAP_PROCESS_CONTINUE = 1,

  /* Processing succeed, but no more processing is required, until next event. */
  CLAP_PROCESS_SLEEP    = 2,
};

struct clap_process
{
  /* audio buffers */
  int32_t frames_count;

  /* process info */
  int64_t steady_time; // the steady time in samples

  /* events */
  struct clap_event *events;
};

//////////
// HOST //
//////////

struct clap_host
{
  int32_t clap_version; // initialized to CLAP_VERSION

  void *host_data; // private pointer for the host

  /* returns the size of the original string, 0 if not string */
  int32_t (*get_attribute)(struct clap_host *host,
                           const char       *attr,
                           char             *buffer,
                           int32_t           size);

  /* for events generated by the plugin. */
  void (*events)(struct clap_host   *host,
                 struct clap_plugin *plugin,
                 struct clap_event  *events);

  /* The time in samples, this clock is monotonicaly increasing,
   * it means that each time you read the value, it must be greater
   * or equal to the previous one. */
  const int64_t *steady_time;

  /* Log a message through the host. */
  void (*log)(struct clap_host       *host,
              struct clap_plugin     *plugin,
              enum clap_log_severity  severity,
              const char             *msg);

  /* feature extensions */
  void *(*extension)(struct clap_host *host, const char *extention_id);

  /* host private data */
  void *host_data;
};

////////////
// PLUGIN //
////////////

// bitfield
enum clap_plugin_type
{
  CLAP_PLUGIN_INSTRUMENT   = (1 << 0),
  CLAP_PLUGIN_EFFECT       = (1 << 1),
  CLAP_PLUGIN_EVENT_EFFECT = (1 << 2), // can be seen as midi effect
  CLAP_PLUGIN_ANALYZER     = (1 << 3),
};

struct clap_plugin
{
  int32_t clap_version; // initialized to CLAP_VERSION

  void *host_data;   // reserved pointer for the host
  void *plugin_data; // reserved pointer for the plugin

  /* Free the plugin and its resources.
   * It is not required to deactivate the plugin prior to this call. */
  void (*destroy)(struct clap_plugin *plugin);

  /* Copy at most size of the attribute's value into buffer.
   * This function must place a '\0' byte at the end of the string.
   * Returns the size of the original string or 0 if there is no
   * value for this attributes. */
  int32_t (*get_attribute)(struct clap_plugin *plugin,
                           const char         *attr,
                           char               *buffer,
                           int32_t             size);

  /* activation */
  bool (*activate)(struct clap_plugin *plugin);
  void (*deactivate)(struct clap_plugin *plugin);

  /* process */
  enum clap_process_status (*process)(struct clap_plugin  *plugin,
                                      struct clap_process *process);

  /* features extensions */
  void *(*extension)(struct clap_plugin *plugin, const char *id);
};

/* typedef for dlsym() cast */
typedef struct clap_plugin *(*clap_create_f)(int32_t           plugin_index,
			                     struct clap_host *host,
                                             int32_t           sample_rate,
                                             int32_t          *plugins_count);

/* Plugin entry point. If plugins_count is not null, then clap_create has
 * to store the number of plugins available in *plugins_count.
 * If clap_create failed to create a plugin, it returns NULL.
 * The return value has to be freed by calling plugin->destroy(plugin).
 *
 * Common sample rate values are: 44100, 48000, 88200, 96000,
 * 176400, 192000.
 *
 * This function must be thread-safe. */
CLAP_EXPORT struct clap_plugin *
clap_create(int32_t           plugin_index,
            struct clap_host *host,
            int32_t           sample_rate,
            int32_t          *plugins_count);

# ifdef __cplusplus
}
# endif

#endif /* !CLAP_H */
