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

#include "deezer_wrapper/deezer_wrapper.h"

#include <algorithm>
#include <iostream>

#define TEST_PLAYER_APPLICATION_ID      "247082"	// SET YOUR APPLICATION ID
#define TEST_PLAYER_APPLICATION_NAME    "Deezzy"    // SET YOUR APPLICATION NAME
#define TEST_PLAYER_APPLICATION_VERSION "00001"     // SET YOUR APPLICATION VERSION

deezer_wrapper dz_wrapper( TEST_PLAYER_APPLICATION_ID, TEST_PLAYER_APPLICATION_NAME, TEST_PLAYER_APPLICATION_VERSION, true );

char* get_option( char ** begin, char ** end, const std::string& option )
{
    auto** itr = std::find( begin, end, option );
    if ( itr != end && ++itr != end )
    {
        return *itr;
    }
    return nullptr;
}

bool leave()
{
    char c;
    std::cin >> std::noskipws >> c;
    return c == 'Q';
}

class my_observer : public deezer_wrapper::observer
{
    private:
        void on_connect_event( const deezer_wrapper::connect_event& event ) final override
        {
            switch( event )
            {
                case deezer_wrapper::connect_event::unknown:
                    break;
                case deezer_wrapper::connect_event::user_offline_available:
                    break;
                case deezer_wrapper::connect_event::user_access_token_ok:
                    break;
                case deezer_wrapper::connect_event::user_access_token_failed:
                    break;
                case deezer_wrapper::connect_event::user_login_ok:
                    break;
                case deezer_wrapper::connect_event::user_login_fail_network_error:
                    break;
                case deezer_wrapper::connect_event::user_login_fail_bad_credentials:
                    break;
                case deezer_wrapper::connect_event::user_login_fail_user_info:
                    break;
                case deezer_wrapper::connect_event::user_login_fail_offline_mode:
                    break;
                case deezer_wrapper::connect_event::user_new_options:
                    break;
                case deezer_wrapper::connect_event::advertisement_start:
                    break;
                case deezer_wrapper::connect_event::advertisement_stop:
                    break;
                default:
                    break;
            }
        }
        void on_player_event( const deezer_wrapper::player_event& event ) final override
        {
            switch( event )
            {
                case deezer_wrapper::player_event::unknown:
                    break;
                case deezer_wrapper::player_event::limitation_forced_pause:
                    break;
                case deezer_wrapper::player_event::queuelist_loaded:
                    dz_wrapper.playback_start();
                    break;
                case deezer_wrapper::player_event::queuelist_no_right:
                    break;
                case deezer_wrapper::player_event::queuelist_track_not_available_offline:
                    break;
                case deezer_wrapper::player_event::queuelist_track_rights_after_audioads:
                    break;
                case deezer_wrapper::player_event::queuelist_skip_no_right:
                    break;
                case deezer_wrapper::player_event::queuelist_track_selected:
                    break;
                case deezer_wrapper::player_event::queuelist_need_natural_next:
                    break;
                case deezer_wrapper::player_event::mediastream_data_ready:
                    break;
                case deezer_wrapper::player_event::mediastream_data_ready_after_seek:
                    break;
                case deezer_wrapper::player_event::render_track_start_failure:
                    break;
                case deezer_wrapper::player_event::render_track_start:
                    break;
                case deezer_wrapper::player_event::render_track_end:
                    break;
                case deezer_wrapper::player_event::render_track_paused:
                    break;
                case deezer_wrapper::player_event::render_track_seeking:
                    break;
                case deezer_wrapper::player_event::render_track_underflow:
                    break;
                case deezer_wrapper::player_event::render_track_resumed:
                    break;
                case deezer_wrapper::player_event::render_track_removed:
                    break;
                default:
                    break;
            }
        }
        void on_index_progress( int progress_ms ) final override
        {
        }
        void on_render_progress( int progress_ms ) final override
        {
        }
        void on_track_duration( int duration_ms ) final override
        {
        }
};

int main( int argc, char *argv[] )
{
    auto* playlist = get_option( argv, argv+argc, "-p" );

    my_observer player_observer;

    dz_wrapper.register_observer( &player_observer );
    dz_wrapper.connect();
    dz_wrapper.set_content( playlist ? std::string( playlist ) : ( "dzradio:///user-" + dz_wrapper.user_id() ) );
    dz_wrapper.load_content();

    while ( dz_wrapper.active() )
    {
        if ( leave() )
            break;
    }

    dz_wrapper.playback_stop();
    dz_wrapper.disconnect();
}
