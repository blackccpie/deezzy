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

#include "deezer_wrapper.h"

#include <iostream>

static int is_shutting_down = false;

#define DEEZZY_APPLICATION_ID      "247082"	// SET YOUR APPLICATION ID
#define DEEZZY_APPLICATION_NAME    "Deezzy" 	// SET YOUR APPLICATION NAME
#define DEEZZY_APPLICATION_VERSION "00001"	// SET YOUR APPLICATION VERSION

static deezer_wrapper dzr_wrapper(  DEEZZY_APPLICATION_ID,
                                    DEEZZY_APPLICATION_NAME,
                                    DEEZZY_APPLICATION_VERSION,
                                    true /*print_version*/ );

class deezzy_observer final : public deezer_wrapper::observer
{
public:
    void on_connect_event( const deezer_wrapper::connect_event& event ) override
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
                dzr_wrapper.load_content();
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
    void on_player_event( const deezer_wrapper::player_event& event ) override
    {
        switch( event )
        {
            case deezer_wrapper::player_event::unknown:
                break;
            case deezer_wrapper::player_event::limitation_forced_pause:
                break;
            case deezer_wrapper::player_event::queuelist_loaded:
                dzr_wrapper.playback_start_or_stop();
                break;
            case deezer_wrapper::player_event::queuelist_no_right:
                break;
            case deezer_wrapper::player_event::queuelist_track_not_available_offline:
                break;
            case deezer_wrapper::player_event::queuelist_track_rights_after_audioads:
                dzr_wrapper.play_audioads();
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
};

static void app_commands_display();
static void app_commands_get_next();

int main(int argc, char *argv[])
{
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

    try
    {
        std::shared_ptr<deezzy_observer> dzr_observer = std::make_shared<deezzy_observer>();
        dzr_wrapper.register_observer( dzr_observer );
        dzr_wrapper.set_content( argv[1] );
        dzr_wrapper.connect();

        while ( ( dzr_wrapper.active() ) )
        {
            // Get the next user action only if not shutting down.
            if ( !is_shutting_down ) {
                app_commands_get_next();
            }
        }

        std::cout << "-- shutdowned --" << std::endl;
    }
    catch( deezer_wrapper_exception& e )
    {
        std::cerr << "-- runtime error : " << e.what() << " --" << std::endl;
    }
    catch(...)
    {
        std::cerr << "-- unknown runtime error --" << std::endl;
    }

    return 0;
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
            dzr_wrapper.playback_start_or_stop();
            break;

        case 'P':
            dzr_wrapper.playback_pause_or_resume();
            break;

        case '+':
            dzr_wrapper.playback_next();
            break;

        case '-':
            dzr_wrapper.playback_previous();
            break;

        case 'R':
            dzr_wrapper.playback_toogle_repeat();
            break;

        case '?':
            dzr_wrapper.playback_toogle_random();
            break;

        case 'Q':
            is_shutting_down = true;
            dzr_wrapper.disconnect();
            break;
        case '1':
            dzr_wrapper.set_content( "dzmedia:///album/607845" );
            dzr_wrapper.load_content();
            break;
        case '2':
            dzr_wrapper.set_content( "dzmedia:///playlist/1363560485" );
            dzr_wrapper.load_content();
            break;
        case '3':
            dzr_wrapper.set_content( "dzradio:///user-743548285" );
            dzr_wrapper.load_content();
            break;
        case '4':
            dzr_wrapper.set_content( "dzmedia:///track/10287076" );
            dzr_wrapper.load_content();
            break;

        default:
            std::cout << " - Command [" << c << "] not recognized -" << std::endl;
            app_commands_display();
            break;
    }
}
