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

#include <deezer-connect.h>
#include <deezer-player.h>

#include <iostream>

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

using app_context_handle = app_context*;

static int print_version    = true;
static int is_shutting_down = false;
static int print_device_id  = true;
static app_context_handle app_ctxt = nullptr;

#define DEEZZY_APPLICATION_ID      "247082"	// SET YOUR APPLICATION ID
#define DEEZZY_APPLICATION_NAME    "Deezzy" 	// SET YOUR APPLICATION NAME
#define DEEZZY_APPLICATION_VERSION "00001"	// SET YOUR APPLICATION VERSION

static void app_commands_display();
static void app_commands_get_next();
static void app_change_content( const std::string& content );
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
    dz_connect_configuration config = {0};
    dz_error_t dzerr = DZ_ERROR_NO_ERROR;

    if ( print_version )
    {
        std::cout << "<-- Deezer native SDK Version : " << dz_connect_get_build_id() << std::endl;
    }

    config.app_id            = DEEZZY_APPLICATION_ID;      // SET YOUR APPLICATION ID
    config.product_id        = DEEZZY_APPLICATION_NAME;    // SET YOUR APPLICATION NAME
    config.product_build_id  = DEEZZY_APPLICATION_VERSION; // SET YOUR APPLICATION VERSION
    config.user_profile_path = USER_CACHE_PATH;          // SET THE USER CACHE PATH
    config.connect_event_cb  = app_connect_onevent_cb;

    std::cout << "--> Application ID : " << config.app_id << std::endl;
    std::cout << "--> Product ID : " << config.product_id << std::endl;
    std::cout << "--> Product BUILD ID : " << config.product_build_id << std::endl;
    std::cout << "--> User Profile Path : " << config.user_profile_path << std::endl;


    // Nanoplayer only supports one argument which is the content description
    if (argc < 2) {
        std::cout << "Please give the content as argument like:" << std::endl;
        std::cout << "\t \"dzmedia:///track/10287076\"      (Single track example)" << std::endl;
        std::cout << "\t \"dzmedia:///album/607845\"        (Album example)" << std::endl;
        std::cout << "\t \"dzmedia:///playlist/1363560485\" (Playlist example)" << std::endl;
        std::cout << "\t \"dzradio:///radio-223\"           (Radio example)" << std::endl;
        std::cout << "\t \"dzradio:///user-743548285\"      (User Mix example)" << std::endl;
        return -1;
    }

    app_ctxt = (app_context_handle)calloc( 1, sizeof( app_context ) );

    app_change_content( argv[1] );

    app_ctxt->dzconnect = dz_connect_new( &config );

    if ( app_ctxt->dzconnect == nullptr )
    {
        std::cout << "dzconnect null" << std::endl;
        return -1;
    }

    if ( print_device_id )
    {
        std::cout << "Device ID : " << dz_connect_get_device_id( app_ctxt->dzconnect ) << std::endl;
    }

    dzerr = dz_connect_debug_log_disable( app_ctxt->dzconnect );
    if ( dzerr != DZ_ERROR_NO_ERROR )
    {
        std::cout << "dz_connect_debug_log_disable error" << std::endl;
        return -1;
    }

    dzerr = dz_connect_activate( app_ctxt->dzconnect, app_ctxt );
    if ( dzerr != DZ_ERROR_NO_ERROR )
    {
        std::cout << "dz_connect_activate error" << std::endl;
        return -1;
    }
    app_ctxt->activation_count++;

    /* Calling dz_connect_cache_path_set()
     * is mandatory in order to have the attended behavior */
    dz_connect_cache_path_set( app_ctxt->dzconnect, NULL, NULL, USER_CACHE_PATH );

    app_ctxt->dzplayer = dz_player_new( app_ctxt->dzconnect );
    if ( app_ctxt->dzplayer == nullptr ) {
        std::cout << "dzplayer null" << std::endl;
        return -1;
    }

    dzerr = dz_player_activate( app_ctxt->dzplayer, app_ctxt );
    if ( dzerr != DZ_ERROR_NO_ERROR )
    {
        std::cout << "dz_player_activate error" << std::endl;
        return -1;
    }
    app_ctxt->activation_count++;

    dzerr = dz_player_set_event_cb( app_ctxt->dzplayer, app_player_onevent_cb );
    if (dzerr != DZ_ERROR_NO_ERROR) {
        std::cout << "dz_player_set_event_cb error" << std::endl;
        return -1;
    }

    dzerr = dz_player_set_output_volume(app_ctxt->dzplayer, NULL, NULL, 20);
    if (dzerr != DZ_ERROR_NO_ERROR) {
        std::cout << "dz_player_set_output_volume error" << std::endl;
        return -1;
    }

    dzerr = dz_player_set_crossfading_duration(app_ctxt->dzplayer, NULL, NULL, 3000);
    if (dzerr != DZ_ERROR_NO_ERROR) {
        std::cout << "dz_player_set_crossfading_duration error" << std::endl;
        return -1;
    }

    app_ctxt->repeat_mode = DZ_QUEUELIST_REPEAT_MODE_OFF;
    app_ctxt->is_shuffle_mode = false;

    dzerr = dz_connect_set_access_token(app_ctxt->dzconnect, nullptr, nullptr, USER_ACCESS_TOKEN);
    if (dzerr != DZ_ERROR_NO_ERROR) {
        std::cout << "dz_connect_set_access_token error" << std::endl;
        return -1;
    }

    /* Calling dz_connect_offline_mode(FALSE) is mandatory to force the login */
    dzerr = dz_connect_offline_mode(app_ctxt->dzconnect, nullptr, nullptr, false);
    if (dzerr != DZ_ERROR_NO_ERROR) {
        std::cout << "dz_connect_offline_mode error" << std::endl;
        return -1;
    }

    while ( ( app_ctxt->activation_count > 0 ) ) {
        // Get the next user action only if not shutting down.
        if ( !is_shutting_down ) {
            app_commands_get_next();
        }
    }

    if (app_ctxt->dzplayer) {
        std::cout << "-- FREE PLAYER @ " << app_ctxt->dzplayer << " --" << std::endl;
        dz_object_release( (dz_object_handle)app_ctxt->dzplayer );
        app_ctxt->dzplayer = nullptr;
    }

    if (app_ctxt->dzconnect) {
        std::cout << "-- FREE CONNECT @ " << app_ctxt->dzconnect << " --" << std::endl;
        dz_object_release( (dz_object_handle)app_ctxt->dzconnect );
        app_ctxt->dzconnect = nullptr;
    }

    std::cout << "-- shutdowned --" << std::endl;

    if ( app_ctxt )
    {
        free( app_ctxt );
        app_ctxt = nullptr;
    }

    return 0;
}

static void app_shutdown()
{
    std::cout << "SHUTDOWN (1/2) - dzplayer = " << app_ctxt->dzplayer << std::endl;
    if ( app_ctxt->dzplayer )
        dz_player_deactivate( app_ctxt->dzplayer, dz_player_on_deactivate, nullptr );
    std::cout << "SHUTDOWN (2/2) - dzconnect = " << app_ctxt->dzconnect << std::endl;
    if ( app_ctxt->dzconnect )
        dz_connect_deactivate( app_ctxt->dzconnect, dz_connect_on_deactivate, nullptr );
}

void app_connect_onevent_cb( dz_connect_handle handle,
                            dz_connect_event_handle event,
                            void* delegate )
{
    dz_connect_event_t type = dz_connect_event_get_type( event );

    app_context_handle context = (app_context_handle)delegate;

    switch (type)
    {
        case DZ_CONNECT_EVENT_USER_OFFLINE_AVAILABLE:
            std::cout << "(App:" << context << ") ++++ CONNECT_EVENT ++++ USER_OFFLINE_AVAILABLE" << std::endl;
            break;

        case DZ_CONNECT_EVENT_USER_ACCESS_TOKEN_OK:
            {
                const char* szAccessToken;
                szAccessToken = dz_connect_event_get_access_token( event );
                std::cout << "(App:" << context << ") ++++ CONNECT_EVENT ++++ USER_ACCESS_TOKEN_OK Access_token : " << szAccessToken << std::endl;
            }
            break;

        case DZ_CONNECT_EVENT_USER_ACCESS_TOKEN_FAILED:
            std::cout << "(App:" << context << ") ++++ CONNECT_EVENT ++++ USER_ACCESS_TOKEN_FAILED" << std::endl;
            break;

        case DZ_CONNECT_EVENT_USER_LOGIN_OK:
            std::cout << "(App:" << context << ") ++++ CONNECT_EVENT ++++ USER_LOGIN_OK" << std::endl;
            app_load_content();
            break;

        case DZ_CONNECT_EVENT_USER_NEW_OPTIONS:
            std::cout << "(App:" << context << ") ++++ CONNECT_EVENT ++++ USER_NEW_OPTIONS" << std::endl;
            break;

        case DZ_CONNECT_EVENT_USER_LOGIN_FAIL_NETWORK_ERROR:
            std::cout << "(App:" << context << ") ++++ CONNECT_EVENT ++++ USER_LOGIN_FAIL_NETWORK_ERROR" << std::endl;
            break;

        case DZ_CONNECT_EVENT_USER_LOGIN_FAIL_BAD_CREDENTIALS:
            std::cout << "(App:" << context << ") ++++ CONNECT_EVENT ++++ USER_LOGIN_FAIL_BAD_CREDENTIALS" << std::endl;
            break;

        case DZ_CONNECT_EVENT_USER_LOGIN_FAIL_USER_INFO:
            std::cout << "(App:" << context << ") ++++ CONNECT_EVENT ++++ USER_LOGIN_FAIL_USER_INFO" << std::endl;
            break;

        case DZ_CONNECT_EVENT_USER_LOGIN_FAIL_OFFLINE_MODE:
            std::cout << "(App:" << context << ") ++++ CONNECT_EVENT ++++ USER_LOGIN_FAIL_OFFLINE_MODE" << std::endl;
            break;

        case DZ_CONNECT_EVENT_ADVERTISEMENT_START:
            std::cout << "(App:" << context << ") ++++ CONNECT_EVENT ++++ ADVERTISEMENT_START" << std::endl;
            break;

        case DZ_CONNECT_EVENT_ADVERTISEMENT_STOP:
            std::cout << "(App:" << context << ") ++++ CONNECT_EVENT ++++ ADVERTISEMENT_STOP" << std::endl;
            break;

        case DZ_CONNECT_EVENT_UNKNOWN:
        default:
            std::cout << "(App:" << context << ") ++++ CONNECT_EVENT ++++ UNKNOWN or default (type = " << type << ")" << std::endl;
            break;
    }
}

static void app_change_content( const std::string& content )
{
    if (app_ctxt->sz_content_url) {
        free(app_ctxt->sz_content_url);
    }
    app_ctxt->sz_content_url = (char*)calloc( 1, content.size()+1 );
    memccpy( app_ctxt->sz_content_url, content.c_str(), 1, content.size() );

    std::cout << "CHANGE => " << app_ctxt->sz_content_url << std::endl;
}

static void app_load_content()
{
    std::cout << "LOAD => " << app_ctxt->sz_content_url << std::endl;
    dz_player_load( app_ctxt->dzplayer,
                   nullptr,
                   nullptr,
                   app_ctxt->sz_content_url );
}

static void app_playback_start_or_stop()
{
    if ( !app_ctxt->is_playing )
    {
        std::cout << "PLAY track n° " << app_ctxt->nb_track_played << " of => " << app_ctxt->sz_content_url << std::endl;
        dz_player_play( app_ctxt->dzplayer, nullptr, nullptr,
                       DZ_PLAYER_PLAY_CMD_START_TRACKLIST,
                       DZ_INDEX_IN_QUEUELIST_CURRENT );

    }
    else
    {
        std::cout << "STOP => " << app_ctxt->sz_content_url << std::endl;
        dz_player_stop( app_ctxt->dzplayer, nullptr, nullptr );
    }
}

static void app_playback_pause_or_resume()
{
    if ( app_ctxt->is_playing )
    {
        std::cout << "PAUSE track n° " << app_ctxt->nb_track_played << " of => " << app_ctxt->sz_content_url << std::endl;
        dz_player_pause( app_ctxt->dzplayer, nullptr, nullptr );
    }
    else
    {
        std::cout << "RESUME track n° " << app_ctxt->nb_track_played << " of => " << app_ctxt->sz_content_url << std::endl;
        dz_player_resume( app_ctxt->dzplayer, nullptr, nullptr );
    }
}

static void app_playback_toogle_repeat()
{
    switch( app_ctxt->repeat_mode )
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

    std::cout << "REPEAT mode => " << app_ctxt->repeat_mode << std::endl;

    dz_player_set_repeat_mode( app_ctxt->dzplayer,
                              nullptr,
                              nullptr,
                              app_ctxt->repeat_mode );
    return;
}

static void app_playback_toogle_random()
{
    app_ctxt->is_shuffle_mode = (app_ctxt->is_shuffle_mode?false:true);

    std::cout << "SHUFFLE mode => " << std::string( app_ctxt->is_shuffle_mode ? "ON" : "OFF" ) << std::endl;

    dz_player_enable_shuffle_mode( app_ctxt->dzplayer,
                                  nullptr,
                                  nullptr,
                                  app_ctxt->is_shuffle_mode );
}

static void app_playback_next()
{
    std::cout << "NEXT => " << app_ctxt->sz_content_url << std::endl;
    dz_player_play( app_ctxt->dzplayer,
                   nullptr,
                   nullptr,
                   DZ_PLAYER_PLAY_CMD_START_TRACKLIST,
                   DZ_INDEX_IN_QUEUELIST_NEXT );
}

static void app_playback_previous()
{
    std::cout << "PREVIOUS => " << app_ctxt->sz_content_url << std::endl;
    dz_player_play( app_ctxt->dzplayer,
                   nullptr,
                   nullptr,
                   DZ_PLAYER_PLAY_CMD_START_TRACKLIST,
                   DZ_INDEX_IN_QUEUELIST_PREVIOUS );

}

void app_player_onevent_cb( dz_player_handle       handle,
                            dz_player_event_handle event,
                            void *                 supervisor)
{
    dz_streaming_mode_t   streaming_mode;
    dz_index_in_queuelist idx;
    auto context = (app_context_handle)supervisor;

    auto type = dz_player_event_get_type(event);

    if ( !dz_player_event_get_queuelist_context( event, &streaming_mode, &idx ) )
    {
        streaming_mode = DZ_STREAMING_MODE_ONDEMAND;
        idx = DZ_INDEX_IN_QUEUELIST_INVALID;
    }

    switch (type)
    {
        case DZ_PLAYER_EVENT_LIMITATION_FORCED_PAUSE:
            std::cout << "(App:" << context << ") ==== PLAYER_EVENT ==== LIMITATION_FORCED_PAUSE for idx: " << idx << std::endl;
            break;

        case DZ_PLAYER_EVENT_QUEUELIST_LOADED:
            std::cout << "(App:" << context << ") ==== PLAYER_EVENT ==== QUEUELIST_LOADED for idx: " << idx << std::endl;
            //app_playback_start_or_stop();
            break;

        case DZ_PLAYER_EVENT_QUEUELIST_NO_RIGHT:
            std::cout << "(App:" << context << ") ==== PLAYER_EVENT ==== QUEUELIST_NO_RIGHT for idx: " << idx << std::endl;
            break;

        case DZ_PLAYER_EVENT_QUEUELIST_NEED_NATURAL_NEXT:
            std::cout << "(App:" << context << ") ==== PLAYER_EVENT ==== QUEUELIST_NEED_NATURAL_NEXT for idx: " << idx << std::endl;
            break;

        case DZ_PLAYER_EVENT_QUEUELIST_TRACK_NOT_AVAILABLE_OFFLINE:
            std::cout << "(App:" << context << ") ==== PLAYER_EVENT ==== QUEUELIST_TRACK_NOT_AVAILABLE_OFFLINE for idx: " << idx << std::endl;
            break;

        case DZ_PLAYER_EVENT_QUEUELIST_TRACK_RIGHTS_AFTER_AUDIOADS:
            std::cout << "(App:" << context << ") ==== PLAYER_EVENT ==== QUEUELIST_TRACK_RIGHTS_AFTER_AUDIOADS for idx: " << idx << std::endl;
            dz_player_play_audioads(app_ctxt->dzplayer, NULL, NULL);
            break;

        case DZ_PLAYER_EVENT_QUEUELIST_SKIP_NO_RIGHT:
            std::cout << "(App:" << context << ") ==== PLAYER_EVENT ==== QUEUELIST_SKIP_NO_RIGHT for idx: " << idx << std::endl;
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

            std::cout << "(App:" << context << ") ==== PLAYER_EVENT ==== QUEUELIST_TRACK_SELECTED for idx: " << idx << " - is_preview: " << is_preview << std::endl;
            std::cout << "\tcan_pause_unpause:" << can_pause_unpause << " can_seek:" << can_seek << " nb_skip_allowed:" << nb_skip_allowed << std::endl;
            if (selected_dzapiinfo)
                std::cout << "\tnow:" << selected_dzapiinfo << std::endl;
            if (next_dzapiinfo)
                std::cout << "\tnext:" << next_dzapiinfo << std::endl;
        }

            app_ctxt->nb_track_played++;
            break;

        case DZ_PLAYER_EVENT_MEDIASTREAM_DATA_READY:
            std::cout << "(App:" << context << ") ==== PLAYER_EVENT ==== MEDIASTREAM_DATA_READY for idx: " << idx << std::endl;
            break;

        case DZ_PLAYER_EVENT_MEDIASTREAM_DATA_READY_AFTER_SEEK:
            std::cout << "(App:" << context << ") ==== PLAYER_EVENT ==== MEDIASTREAM_DATA_READY_AFTER_SEEK for idx: " << idx << std::endl;
            break;

        case DZ_PLAYER_EVENT_RENDER_TRACK_START_FAILURE:
            std::cout << "(App:" << context << ") ==== PLAYER_EVENT ==== RENDER_TRACK_START_FAILURE for idx: " << idx << std::endl;
            app_ctxt->is_playing = false;
            break;

        case DZ_PLAYER_EVENT_RENDER_TRACK_START:
            std::cout << "(App:" << context << ") ==== PLAYER_EVENT ==== RENDER_TRACK_START for idx: " << idx << std::endl;
            app_ctxt->is_playing = true;
            break;

        case DZ_PLAYER_EVENT_RENDER_TRACK_END:
            std::cout << "(App:" << context << ") ==== PLAYER_EVENT ==== RENDER_TRACK_END for idx: " << idx << std::endl;
            app_ctxt->is_playing = false;
            std::cout << "- nb_track_played : " << app_ctxt->nb_track_played << std::endl;
            // Detect if we come from from playing an ad, if yes restart automatically the playback.
            if (idx == DZ_INDEX_IN_QUEUELIST_INVALID) {
                app_playback_start_or_stop();
            }
            break;

        case DZ_PLAYER_EVENT_RENDER_TRACK_PAUSED:
            std::cout << "(App:" << context << ") ==== PLAYER_EVENT ==== RENDER_TRACK_PAUSED for idx: " << idx << std::endl;
            app_ctxt->is_playing = false;
            break;

        case DZ_PLAYER_EVENT_RENDER_TRACK_UNDERFLOW:
            std::cout << "(App:" << context << ") ==== PLAYER_EVENT ==== RENDER_TRACK_UNDERFLOW for idx: " << idx << std::endl;
            app_ctxt->is_playing = false;
            break;

        case DZ_PLAYER_EVENT_RENDER_TRACK_RESUMED:
            std::cout << "(App:" << context << ") ==== PLAYER_EVENT ==== RENDER_TRACK_RESUMED for idx: " << idx << std::endl;
            app_ctxt->is_playing = true;
            break;

        case DZ_PLAYER_EVENT_RENDER_TRACK_SEEKING:
            std::cout << "(App:" << context << ") ==== PLAYER_EVENT ==== RENDER_TRACK_SEEKING for idx: " << idx << std::endl;
            app_ctxt->is_playing = false;
            break;

        case DZ_PLAYER_EVENT_RENDER_TRACK_REMOVED:
            std::cout << "(App:" << context << ") ==== PLAYER_EVENT ==== RENDER_TRACK_REMOVED for idx: " << idx << std::endl;
            app_ctxt->is_playing = false;
            break;

        case DZ_PLAYER_EVENT_UNKNOWN:
        default:
            std::cout << "(App:" << context << ") ==== PLAYER_EVENT ==== UNKNOWN or default (type = " << type << ")" << std::endl;
            break;
    }
}

static void dz_connect_on_deactivate(void*            delegate,
                                     void*            operation_userdata,
                                     dz_error_t       status,
                                     dz_object_handle result)
{
    app_ctxt->activation_count--;
    std::cout << "CONNECT deactivated - c = " << app_ctxt->activation_count << " with status = " << status << std::endl;
}

static void dz_player_on_deactivate(void*            delegate,
                                    void*            operation_userdata,
                                    dz_error_t       status,
                                    dz_object_handle result)
{
    app_ctxt->activation_count--;
    std::cout << "PLAYER deactivated - c = " << app_ctxt->activation_count << " with status = " << status << std::endl;
}

static void app_commands_display()
{
    std::cout << std::endl << "#########  MENU  #########" << std::endl;
    std::cout << "   - Please press key for comands:  -" << std::endl;
    std::cout << "  P : PLAY / PAUSE" << std::endl;
    std::cout << "  S : START/STOP" << std::endl;
    std::cout << "  + : NEXT" << std::endl;
    std::cout << "  - : PREVIOUS" << std::endl;
    std::cout << "  R : NEXT REPEAT MODE" << std::endl;
    std::cout << "  ? : TOGGLE SHUFFLE MODE" << std::endl;
    std::cout << "  Q : QUIT" << std::endl;
    std::cout << "  [1-4] : LOAD CONTENT [1-4]" << std::endl;
    std::cout << std::endl << "##########################" << std::endl << std::endl;

}

static void app_commands_get_next()
{
    char c = getchar();

    if ( c == '\n' ) {
        // skip final end of line char
        return;
    }

    app_commands_display();

    switch (c)
    {
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
            app_change_content( "dzmedia:///album/607845" );
            app_load_content();
            break;
        case '2':
            app_change_content( "dzmedia:///playlist/1363560485" );
            app_load_content();
            break;
        case '3':
            app_change_content( "dzradio:///user-743548285" );
            app_load_content();
            break;
        case '4':
            app_change_content( "dzmedia:///track/10287076" );
            app_load_content();
            break;

        default:
            std::cout << " - Command [" << c << "] not recognized -" << std::endl;
            app_commands_display();
            break;
    }
}
