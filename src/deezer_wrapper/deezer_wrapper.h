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

#pragma once

#include <memory>

struct dz_connect_configuration;

class deezer_wrapper_exception : public std::runtime_error
{
public:

    deezer_wrapper_exception( const std::string& error ) : std::runtime_error( error ) {}
	virtual ~deezer_wrapper_exception() throw() {}
};

class deezer_wrapper
{
public:

    struct track_infos
    {
        int id;
        std::string title;
        std::string artist;
        int duration;
        std::string album_title;
        std::string cover_art;
    };

    /*struct track_metadata
    {
        int duration;
    };*/

    enum class connect_event
    {
        unknown,                           ///< connect event has not been set yet, not a valid value.
        user_offline_available,            ///< user logged in, and credentials from offline store are loaded.

        user_access_token_ok,              ///< (not available) dz_connect_login_with_email() ok, and access_token is available.
        user_access_token_failed,          ///< (not available) dz_connect_login_with_email() failed.

        user_login_ok,                     ///< login with access_token ok, infos from user available.
        user_login_fail_network_error,     ///< login with access_token failed because of network condition.
        user_login_fail_bad_credentials,   ///< login with access_token failed because of bad credentials.
        user_login_fail_user_info,         ///< login with access_token failed because of other problem.
        user_login_fail_offline_mode,      ///< login with access_token failed because we are in forced offline mode.

        user_new_options,                  ///< user options have just changed.

        advertisement_start,               ///< a new advertisement needs to be displayed.
        advertisement_stop,                ///< an advertisement needs to be stopped.
    };

    enum class player_event
    {
        unknown,                                ///< player event has not been set yet, not a valid value.

        // data access related event.
        limitation_forced_pause,                ///< another deezer player session was created elsewhere, the player has entered pause mode.

        // track selection related event.
        queuelist_loaded,                       ///< content has been loaded.
        queuelist_no_right,                     ///< you don't have the right to play this content: track, album or playlist.
        queuelist_track_not_available_offline,  ///< you're offline, the track is not available.
        queuelist_track_rights_after_audioads,  /**< you have right to play it, but you should render an ads first :
                                                        - use dz_player_event_get_advertisement_infos_json().
                                                        - play an ad with dz_player_play_audioads().
                                                        - wait for #dz_player_event_render_track_end.
                                                        - use dz_player_play() with previous track or dz_player_play_cmd_resumed_after_ads (to be done even on mixes for now).
                                                */
        queuelist_skip_no_right,                ///< you're on a mix, and you had no right to do skip.

        queuelist_track_selected,               ///< a track is selected among the ones available on the server, and will be fetched and read.

        queuelist_need_natural_next,            ///< we need a new natural_next action.

        // data loading related event.
        mediastream_data_ready,                 ///< data is ready to be introduced into audio output (first data after a play).
        mediastream_data_ready_after_seek,      ///< data is ready to be introduced into audio output (first data after a seek).

        // play (audio rendering on output) related event.
        render_track_start_failure,             ///< error, track is unable to play. */
        render_track_start,                     ///< a track has started to play. */
        render_track_end,                       ///< a track has stopped because the stream has ended. */
        render_track_paused,                    ///< currently on paused. */
        render_track_seeking,                   ///< waiting for new data on seek. */
        render_track_underflow,                 ///< underflow happened whilst playing a track. */
        render_track_resumed,                   ///< player resumed play after a underflow or a pause. */
        render_track_removed,                   ///< player stopped playing a track. */
    };

public:

    class observer
    {
    protected:
        friend class deezer_wrapper;
        virtual void on_connect_event( const deezer_wrapper::connect_event& event ) { /* EMPTY */ }
        virtual void on_player_event( const deezer_wrapper::player_event& event ) { /* EMPTY */ }
        virtual void on_index_progress( int progress_ms ) { /* EMPTY */ }
        virtual void on_render_progress( int progress_ms ) { /* EMPTY */ }
        virtual void on_track_duration( int duration_ms ) { /* EMPTY */ }
    };

public:
    deezer_wrapper( const std::string& app_id,
                    const std::string& product_id,
                    const std::string& product_build_id,
    				bool print_version );
    ~deezer_wrapper();

    std::string user_id();

    void register_observer( deezer_wrapper::observer* observer );

    void set_content( const std::string& content );
    void load_content();
    std::string get_content();

    bool active();

    void connect();
    void disconnect();

    void playback_start();
    void playback_stop();
    void playback_pause();
    void playback_resume();
    void playback_seek( int position_ms );

    void playback_toogle_repeat();
    void playback_toogle_random();

    void playback_next();
    void playback_previous();

    void playback_like();
    void playback_dislike();

    void play_audioads();

    const track_infos& current_track_infos();

private:

    class deezer_wrapper_impl;
    std::unique_ptr<deezer_wrapper_impl> m_pimpl;
};
