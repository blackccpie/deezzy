/*
The MIT License

Copyright (c) 2017-2017 Albert Murienne

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <user.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include <deezer-connect.h>
#include <deezer-player.h>

//#define log(msg) std::cout << __FILE__ << "(" << __LINE__ << "): " << msg << std::endl
#define log(fmt, ...) printf("[%s:%d]" fmt, __FILE__, __LINE__, ##__VA_ARGS__);

typedef struct {
    int                   nb_track_played;
    bool                  is_playing;
    char*                 sz_content_url;
    int                   activation_count;
    dz_connect_handle     dzconnect;
    dz_player_handle      dzplayer;
    dz_queuelist_repeat_mode_t repeat_mode;
    bool                  is_shuffle_mode;
} app_context;

typedef app_context *app_context_handle;

static int print_version    = true;
static int is_shutting_down = false;
static int print_device_id  = true;
static app_context_handle app_ctxt = NULL;

#define DEEZZY_APPLICATION_ID      "247082"	// SET YOUR APPLICATION ID
#define DEEZZY_APPLICATION_NAME    "Deezzy" 	// SET YOUR APPLICATION NAME
#define DEEZZY_APPLICATION_VERSION "00001"	// SET YOUR APPLICATION VERSION

static void app_commands_display();
static void app_commands_get_next();
static void app_change_content(char * content);
static void app_load_content();
static void app_playback_start_or_stop();
static void app_playback_pause_or_resume();
static void app_playback_next();
static void app_playback_previous();
static void app_playback_toogle_repeat();
static void app_playback_toogle_random();
static void app_shutdown();

static void dz_connect_on_deactivate(void* delegate,
                                     void* operation_userdata,
                                     dz_error_t status,
                                     dz_object_handle result);
static void dz_player_on_deactivate(void* delegate,
                                    void* operation_userdata,
                                    dz_error_t status,
                                    dz_object_handle result);

// callback for dzconnect events
static void app_connect_onevent_cb(dz_connect_handle handle,
                                   dz_connect_event_handle event,
                                   void* delegate);
// callback for dzplayer events
static void app_player_onevent_cb(dz_player_handle handle,
                                  dz_player_event_handle event,
                                  void* supervisor);

int main(int argc, char *argv[])
{
    struct dz_connect_configuration config;
    dz_error_t dzerr = DZ_ERROR_NO_ERROR;

    if (print_version) {
        log("<-- Deezer native SDK Version : %s\n", dz_connect_get_build_id());
    }

    memset(&config, 0, sizeof(struct dz_connect_configuration));

    config.app_id            = DEEZZY_APPLICATION_ID;      // SET YOUR APPLICATION ID
    config.product_id        = DEEZZY_APPLICATION_NAME;    // SET YOUR APPLICATION NAME
    config.product_build_id  = DEEZZY_APPLICATION_VERSION; // SET YOUR APPLICATION VERSION
    config.user_profile_path = USER_CACHE_PATH;          // SET THE USER CACHE PATH
    config.connect_event_cb  = app_connect_onevent_cb;

    log("--> Application ID:    %s\n", config.app_id);
    log("--> Product ID:        %s\n", config.product_id);
    log("--> Product BUILD ID:  %s\n", config.product_build_id);
    log("--> User Profile Path: %s\n", config.user_profile_path);


    // Nanoplayer only supports one argument which is the content description
    if (argc < 2) {
        log("Please give the content as argument like:\n");
        log("\t \"dzmedia:///track/10287076\"      (Single track example)\n");
        log("\t \"dzmedia:///album/607845\"        (Album example)\n");
        log("\t \"dzmedia:///playlist/1363560485\" (Playlist example)\n");
        log("\t \"dzradio:///radio-223\"           (Radio example)\n");
        log("\t \"dzradio:///user-743548285\"      (User Mix example)\n");
        return -1;
    }

    app_ctxt = (app_context_handle)calloc(1,sizeof(app_context));

    app_change_content(argv[1]);

    app_ctxt->dzconnect = dz_connect_new(&config);

    if (app_ctxt->dzconnect == NULL) {
        log("dzconnect null\n");
        return -1;
    }

    if (print_device_id) {
        log("Device ID: %s\n", dz_connect_get_device_id(app_ctxt->dzconnect));
    }

    dzerr = dz_connect_debug_log_disable(app_ctxt->dzconnect);
    if (dzerr != DZ_ERROR_NO_ERROR) {
        log("dz_connect_debug_log_disable error\n");
        return -1;
    }

    dzerr = dz_connect_activate(app_ctxt->dzconnect, app_ctxt);
    if (dzerr != DZ_ERROR_NO_ERROR) {
        log("dz_connect_activate error\n");
        return -1;
    }
    app_ctxt->activation_count++;

    /* Calling dz_connect_cache_path_set()
     * is mandatory in order to have the attended behavior */
    dz_connect_cache_path_set(app_ctxt->dzconnect, NULL, NULL, USER_CACHE_PATH);

    app_ctxt->dzplayer = dz_player_new(app_ctxt->dzconnect);
    if (app_ctxt->dzplayer == NULL) {
        log("dzplayer null\n");
        return -1;
    }

    dzerr = dz_player_activate(app_ctxt->dzplayer, app_ctxt);
    if (dzerr != DZ_ERROR_NO_ERROR) {
        log("dz_player_activate error\n");
        return -1;
    }
    app_ctxt->activation_count++;

    dzerr = dz_player_set_event_cb(app_ctxt->dzplayer, app_player_onevent_cb);
    if (dzerr != DZ_ERROR_NO_ERROR) {
        log("dz_player_set_event_cb error\n");
        return -1;
    }

    dzerr = dz_player_set_output_volume(app_ctxt->dzplayer, NULL, NULL, 20);
    if (dzerr != DZ_ERROR_NO_ERROR) {
        log("dz_player_set_output_volume error\n");
        return -1;
    }

    dzerr = dz_player_set_crossfading_duration(app_ctxt->dzplayer, NULL, NULL, 3000);
    if (dzerr != DZ_ERROR_NO_ERROR) {
        log("dz_player_set_crossfading_duration error\n");
        return -1;
    }

    app_ctxt->repeat_mode = DZ_QUEUELIST_REPEAT_MODE_OFF;
    app_ctxt->is_shuffle_mode = false;

    dzerr = dz_connect_set_access_token(app_ctxt->dzconnect,NULL, NULL, USER_ACCESS_TOKEN);
    if (dzerr != DZ_ERROR_NO_ERROR) {
        log("dz_connect_set_access_token error\n");
        return -1;
    }

    /* Calling dz_connect_offline_mode(FALSE) is mandatory to force the login */
    dzerr = dz_connect_offline_mode(app_ctxt->dzconnect, NULL, NULL, false);
    if (dzerr != DZ_ERROR_NO_ERROR) {
        log("dz_connect_offline_mode error\n");
        return -1;
    }

    while ((app_ctxt->activation_count > 0)) {
        // Get the next user action only if not shutting down.
        if (!is_shutting_down) {
            app_commands_get_next();
        }
    }

    if (app_ctxt->dzplayer) {
        log("-- FREE PLAYER @ %p --\n",app_ctxt->dzplayer);
        dz_object_release((dz_object_handle)app_ctxt->dzplayer);
        app_ctxt->dzplayer = NULL;
    }

    if (app_ctxt->dzconnect) {
        log("-- FREE CONNECT @ %p --\n",app_ctxt->dzconnect);
        dz_object_release((dz_object_handle)app_ctxt->dzconnect);
        app_ctxt->dzconnect = NULL;
    }

    log("-- shutdowned --\n");

    if (app_ctxt) {
        free(app_ctxt);
        app_ctxt = NULL;
    }

    return 0;
}

static void app_shutdown()
{
    log("SHUTDOWN (1/2) - dzplayer = %p\n",app_ctxt->dzplayer);
    if (app_ctxt->dzplayer)
        dz_player_deactivate(app_ctxt->dzplayer, dz_player_on_deactivate, NULL);
    log("SHUTDOWN (2/2) - dzconnect = %p\n",app_ctxt->dzconnect);
    if (app_ctxt->dzconnect)
        dz_connect_deactivate(app_ctxt->dzconnect, dz_connect_on_deactivate, NULL);
}

void app_connect_onevent_cb(dz_connect_handle handle,
                            dz_connect_event_handle event,
                            void* delegate)
{
    dz_connect_event_t type = dz_connect_event_get_type(event);

    app_context_handle context = (app_context_handle)delegate;

    switch (type) {
        case DZ_CONNECT_EVENT_USER_OFFLINE_AVAILABLE:
            log("(App:%p) ++++ CONNECT_EVENT ++++ USER_OFFLINE_AVAILABLE\n",context);
            break;

        case DZ_CONNECT_EVENT_USER_ACCESS_TOKEN_OK:
            {
                const char* szAccessToken;
                szAccessToken = dz_connect_event_get_access_token(event);
                log("(App:%p) ++++ CONNECT_EVENT ++++ USER_ACCESS_TOKEN_OK Access_token : %s\n",context, szAccessToken);
            }
            break;

        case DZ_CONNECT_EVENT_USER_ACCESS_TOKEN_FAILED:
            log("(App:%p) ++++ CONNECT_EVENT ++++ USER_ACCESS_TOKEN_FAILED\n",context);
            break;

        case DZ_CONNECT_EVENT_USER_LOGIN_OK:
            log("(App:%p) ++++ CONNECT_EVENT ++++ USER_LOGIN_OK\n",context);
            app_load_content();
            break;

        case DZ_CONNECT_EVENT_USER_NEW_OPTIONS:
            log("(App:%p) ++++ CONNECT_EVENT ++++ USER_NEW_OPTIONS\n",context);
            break;

        case DZ_CONNECT_EVENT_USER_LOGIN_FAIL_NETWORK_ERROR:
            log("(App:%p) ++++ CONNECT_EVENT ++++ USER_LOGIN_FAIL_NETWORK_ERROR\n",context);
            break;

        case DZ_CONNECT_EVENT_USER_LOGIN_FAIL_BAD_CREDENTIALS:
            log("(App:%p) ++++ CONNECT_EVENT ++++ USER_LOGIN_FAIL_BAD_CREDENTIALS\n",context);
            break;

        case DZ_CONNECT_EVENT_USER_LOGIN_FAIL_USER_INFO:
            log("(App:%p) ++++ CONNECT_EVENT ++++ USER_LOGIN_FAIL_USER_INFO\n",context);
            break;

        case DZ_CONNECT_EVENT_USER_LOGIN_FAIL_OFFLINE_MODE:
            log("(App:%p) ++++ CONNECT_EVENT ++++ USER_LOGIN_FAIL_OFFLINE_MODE\n",context);
            break;

        case DZ_CONNECT_EVENT_ADVERTISEMENT_START:
            log("(App:%p) ++++ CONNECT_EVENT ++++ ADVERTISEMENT_START\n",context);
            break;

        case DZ_CONNECT_EVENT_ADVERTISEMENT_STOP:
            log("(App:%p) ++++ CONNECT_EVENT ++++ ADVERTISEMENT_STOP\n",context);
            break;

        case DZ_CONNECT_EVENT_UNKNOWN:
        default:
            log("(App:%p) ++++ CONNECT_EVENT ++++ UNKNOWN or default (type = %d)\n",context,type);
            break;
    }
}

static void app_change_content(char * content)
{
    if (app_ctxt->sz_content_url) {
        free(app_ctxt->sz_content_url);
    }
    app_ctxt->sz_content_url = (char*)calloc(1,strlen(content)+1);
    memccpy(app_ctxt->sz_content_url, content, 1, strlen(content));

    log("CHANGE => %s\n", app_ctxt->sz_content_url);
}

static void app_load_content()
{
    log("LOAD => %s\n", app_ctxt->sz_content_url);
    dz_player_load(app_ctxt->dzplayer,
                   NULL,
                   NULL,
                   app_ctxt->sz_content_url);
}

static void app_playback_start_or_stop()
{
    if (!app_ctxt->is_playing) {
        log("PLAY track n° %d of => %s\n", app_ctxt->nb_track_played, app_ctxt->sz_content_url);
        dz_player_play(app_ctxt->dzplayer, NULL, NULL,
                       DZ_PLAYER_PLAY_CMD_START_TRACKLIST,
                       DZ_INDEX_IN_QUEUELIST_CURRENT);

    } else {
        log("STOP => %s\n", app_ctxt->sz_content_url);
        dz_player_stop(app_ctxt->dzplayer, NULL, NULL);
    }
}

static void app_playback_pause_or_resume()
{
    if (app_ctxt->is_playing) {
        log("PAUSE track n° %d of => %s\n", app_ctxt->nb_track_played, app_ctxt->sz_content_url);
        dz_player_pause(app_ctxt->dzplayer, NULL, NULL);
    } else {
        log("RESUME track n° %d of => %s\n", app_ctxt->nb_track_played, app_ctxt->sz_content_url);
        dz_player_resume(app_ctxt->dzplayer, NULL, NULL);
    }
}

static void app_playback_toogle_repeat()
{
    switch(app_ctxt->repeat_mode)
    {
        case DZ_QUEUELIST_REPEAT_MODE_OFF:
            app_ctxt->repeat_mode = DZ_QUEUELIST_REPEAT_MODE_ONE;
            break;
        case DZ_QUEUELIST_REPEAT_MODE_ONE:
            app_ctxt->repeat_mode = DZ_QUEUELIST_REPEAT_MODE_ALL;
            break;
        case DZ_QUEUELIST_REPEAT_MODE_ALL:
            app_ctxt->repeat_mode = DZ_QUEUELIST_REPEAT_MODE_OFF;
            break;
    }

    log("REPEAT mode => %d\n", app_ctxt->repeat_mode);

    dz_player_set_repeat_mode(app_ctxt->dzplayer,
                              NULL,
                              NULL,
                              app_ctxt->repeat_mode);
    return;
}

static void app_playback_toogle_random()
{
    app_ctxt->is_shuffle_mode = (app_ctxt->is_shuffle_mode?false:true);

    log("SHUFFLE mode => %s\n", app_ctxt->is_shuffle_mode?"ON":"OFF");

    dz_player_enable_shuffle_mode(app_ctxt->dzplayer,
                                  NULL,
                                  NULL,
                                  app_ctxt->is_shuffle_mode);
}

static void app_playback_next()
{
    log("NEXT => %s\n", app_ctxt->sz_content_url);
    dz_player_play(app_ctxt->dzplayer,
                   NULL,
                   NULL,
                   DZ_PLAYER_PLAY_CMD_START_TRACKLIST,
                   DZ_INDEX_IN_QUEUELIST_NEXT);
}

static void app_playback_previous()
{
    log("PREVIOUS => %s\n", app_ctxt->sz_content_url);
    dz_player_play(app_ctxt->dzplayer,
                   NULL,
                   NULL,
                   DZ_PLAYER_PLAY_CMD_START_TRACKLIST,
                   DZ_INDEX_IN_QUEUELIST_PREVIOUS);

}

void app_player_onevent_cb( dz_player_handle       handle,
                            dz_player_event_handle event,
                            void *                 supervisor)
{
    dz_streaming_mode_t   streaming_mode;
    dz_index_in_queuelist idx;
    app_context_handle context = (app_context_handle)supervisor;

    dz_player_event_t type = dz_player_event_get_type(event);

    if (!dz_player_event_get_queuelist_context(event, &streaming_mode, &idx)) {
        streaming_mode = DZ_STREAMING_MODE_ONDEMAND;
        idx = DZ_INDEX_IN_QUEUELIST_INVALID;
    }

    switch (type) {

        case DZ_PLAYER_EVENT_LIMITATION_FORCED_PAUSE:
            log("(App:%p) ==== PLAYER_EVENT ==== LIMITATION_FORCED_PAUSE for idx: %d\n", context, idx);
            break;

        case DZ_PLAYER_EVENT_QUEUELIST_LOADED:
            log("(App:%p) ==== PLAYER_EVENT ==== QUEUELIST_LOADED for idx: %d\n", context, idx);
            //app_playback_start_or_stop();
            break;

        case DZ_PLAYER_EVENT_QUEUELIST_NO_RIGHT:
            log("(App:%p) ==== PLAYER_EVENT ==== QUEUELIST_NO_RIGHT for idx: %d\n", context, idx);
            break;

        case DZ_PLAYER_EVENT_QUEUELIST_NEED_NATURAL_NEXT:
            log("(App:%p) ==== PLAYER_EVENT ==== QUEUELIST_NEED_NATURAL_NEXT for idx: %d\n", context, idx);
            break;

        case DZ_PLAYER_EVENT_QUEUELIST_TRACK_NOT_AVAILABLE_OFFLINE:
            log("(App:%p) ==== PLAYER_EVENT ==== QUEUELIST_TRACK_NOT_AVAILABLE_OFFLINE for idx: %d\n", context, idx);
            break;

        case DZ_PLAYER_EVENT_QUEUELIST_TRACK_RIGHTS_AFTER_AUDIOADS:
            log("(App:%p) ==== PLAYER_EVENT ==== QUEUELIST_TRACK_RIGHTS_AFTER_AUDIOADS for idx: %d\n", context, idx);
            dz_player_play_audioads(app_ctxt->dzplayer, NULL, NULL);
            break;

        case DZ_PLAYER_EVENT_QUEUELIST_SKIP_NO_RIGHT:
            log("(App:%p) ==== PLAYER_EVENT ==== QUEUELIST_SKIP_NO_RIGHT for idx: %d\n", context, idx);
            break;

        case DZ_PLAYER_EVENT_QUEUELIST_TRACK_SELECTED:
        {
            bool is_preview;
            bool can_pause_unpause;
            bool can_seek;
            int  nb_skip_allowed;
            const char *selected_dzapiinfo;
            const char *next_dzapiinfo;

            is_preview = dz_player_event_track_selected_is_preview(event);

            dz_player_event_track_selected_rights(event, &can_pause_unpause, &can_seek, &nb_skip_allowed);

            selected_dzapiinfo = dz_player_event_track_selected_dzapiinfo(event);
            next_dzapiinfo = dz_player_event_track_selected_next_track_dzapiinfo(event);

            log("(App:%p) ==== PLAYER_EVENT ==== QUEUELIST_TRACK_SELECTED for idx: %d - is_preview:%d\n", context, idx, is_preview);
            log("\tcan_pause_unpause:%d can_seek:%d nb_skip_allowed:%d\n", can_pause_unpause, can_seek, nb_skip_allowed);
            if (selected_dzapiinfo)
                log("\tnow:%s\n", selected_dzapiinfo);
            if (next_dzapiinfo)
                log("\tnext:%s\n", next_dzapiinfo);
        }

            app_ctxt->nb_track_played++;
            break;

        case DZ_PLAYER_EVENT_MEDIASTREAM_DATA_READY:
            log("(App:%p) ==== PLAYER_EVENT ==== MEDIASTREAM_DATA_READY for idx: %d\n", context, idx);
            break;

        case DZ_PLAYER_EVENT_MEDIASTREAM_DATA_READY_AFTER_SEEK:
            log("(App:%p) ==== PLAYER_EVENT ==== MEDIASTREAM_DATA_READY_AFTER_SEEK for idx: %d\n", context, idx);
            break;

        case DZ_PLAYER_EVENT_RENDER_TRACK_START_FAILURE:
            log("(App:%p) ==== PLAYER_EVENT ==== RENDER_TRACK_START_FAILURE for idx: %d\n", context, idx);
            app_ctxt->is_playing = false;
            break;

        case DZ_PLAYER_EVENT_RENDER_TRACK_START:
            log("(App:%p) ==== PLAYER_EVENT ==== RENDER_TRACK_START for idx: %d\n", context, idx);
            app_ctxt->is_playing = true;
            break;

        case DZ_PLAYER_EVENT_RENDER_TRACK_END:
            log("(App:%p) ==== PLAYER_EVENT ==== RENDER_TRACK_END for idx: %d\n", context, idx);
            app_ctxt->is_playing = false;
            log("- nb_track_played : %d\n",app_ctxt->nb_track_played);
            // Detect if we come from from playing an ad, if yes restart automatically the playback.
            if (idx == DZ_INDEX_IN_QUEUELIST_INVALID) {
                app_playback_start_or_stop();
            }
            break;

        case DZ_PLAYER_EVENT_RENDER_TRACK_PAUSED:
            log("(App:%p) ==== PLAYER_EVENT ==== RENDER_TRACK_PAUSED for idx: %d\n", context, idx);
            app_ctxt->is_playing = false;
            break;

        case DZ_PLAYER_EVENT_RENDER_TRACK_UNDERFLOW:
            log("(App:%p) ==== PLAYER_EVENT ==== RENDER_TRACK_UNDERFLOW for idx: %d\n", context, idx);
            app_ctxt->is_playing = false;
            break;

        case DZ_PLAYER_EVENT_RENDER_TRACK_RESUMED:
            log("(App:%p) ==== PLAYER_EVENT ==== RENDER_TRACK_RESUMED for idx: %d\n", context, idx);
            app_ctxt->is_playing = true;
            break;

        case DZ_PLAYER_EVENT_RENDER_TRACK_SEEKING:
            log("(App:%p) ==== PLAYER_EVENT ==== RENDER_TRACK_SEEKING for idx: %d\n", context, idx);
            app_ctxt->is_playing = false;
            break;

        case DZ_PLAYER_EVENT_RENDER_TRACK_REMOVED:
            log("(App:%p) ==== PLAYER_EVENT ==== RENDER_TRACK_REMOVED for idx: %d\n", context, idx);
            app_ctxt->is_playing = false;
            break;

        case DZ_PLAYER_EVENT_UNKNOWN:
        default:
            log("(App:%p) ==== PLAYER_EVENT ==== UNKNOWN or default (type = %d)\n", context,type);
            break;
    }
}

static void dz_connect_on_deactivate(void*            delegate,
                                     void*            operation_userdata,
                                     dz_error_t       status,
                                     dz_object_handle result)
{
    app_ctxt->activation_count--;
    log("CONNECT deactivated - c = %d with status = %d\n",
        app_ctxt->activation_count,
        status);
}

static void dz_player_on_deactivate(void*            delegate,
                                    void*            operation_userdata,
                                    dz_error_t       status,
                                    dz_object_handle result)
{
    app_ctxt->activation_count--;
    log("PLAYER deactivated - c = %d with status = %d\n",
        app_ctxt->activation_count,
        status);
}

static void app_commands_display()
{
    log("\n#########  MENU  #########\n");
    log("   - Please press key for comands:  -\n");
    log("  P : PLAY / PAUSE\n");
    log("  S : START/STOP\n");
    log("  + : NEXT\n");
    log("  - : PREVIOUS\n");
    log("  R : NEXT REPEAT MODE\n");
    log("  ? : TOGGLE SHUFFLE MODE\n");
    log("  Q : QUIT\n");
    log("  [1-4] : LOAD CONTENT [1-4]\n");
    log("\n##########################\n\n");

}

static void app_commands_get_next()
{
    char c = getchar();

    if (c == '\n') {
        // skip final end of line char
        return;
    }

    app_commands_display();

    switch (c) {
        case 'S':
            app_playback_start_or_stop();
            break;

        case 'P':
            app_playback_pause_or_resume();
            break;

        case '+':
            app_playback_next();
            break;

        case '-':
            app_playback_previous();
            break;

        case 'R':
            app_playback_toogle_repeat();
            break;

        case '?':
            app_playback_toogle_random();
            break;

        case 'Q':
            is_shutting_down = true;
            app_shutdown();
            break;
        case '1':
            app_change_content("dzmedia:///album/607845");
            app_load_content();
            break;
        case '2':
            app_change_content("dzmedia:///playlist/1363560485");
            app_load_content();
            break;
        case '3':
            app_change_content("dzradio:///user-743548285");
            app_load_content();
            break;
        case '4':
            app_change_content("dzmedia:///track/10287076");
            app_load_content();
            break;

        default:
            log(" - Command [%c] not recognised -\n",c);
            app_commands_display();
            break;
    }
}
