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
#include "private_user.h"

#include "third_party/json.hpp"

#include <deezer-connect.h>
#include <deezer-player.h>

#include <iostream>

using namespace std::placeholders;

class deezer_wrapper::deezer_wrapper_impl
{
private:
    struct wrapper_context
    {
        const std::string app_id;
        const std::string product_id;
        const std::string product_build_id;
        int track_played_count = 0;
        int activation_count = 0;
        bool is_shuffle_mode = false;
        std::string content_url;
        dz_connect_handle dzconnect = nullptr;
        dz_player_handle dzplayer = nullptr;
        dz_queuelist_repeat_mode_t repeat_mode = DZ_QUEUELIST_REPEAT_MODE_OFF;
    };
public:
    deezer_wrapper_impl(    const std::string& app_id,
                            const std::string& product_id,
                            const std::string& product_build_id,
                            bool print_version ) : m_config{0}, m_ctx{ app_id, product_id, product_build_id }
    {
        m_config.app_id            = m_ctx.app_id.c_str();
        m_config.product_id        = m_ctx.product_id.c_str();
        m_config.product_build_id  = m_ctx.product_build_id.c_str();
        m_config.user_profile_path = USER_CACHE_PATH;
        m_config.connect_event_cb  = deezer_wrapper_impl::_static_connect_callback;

        std::cout << "--> Application ID : " << m_config.app_id << std::endl;
        std::cout << "--> Product ID : " << m_config.product_id << std::endl;
        std::cout << "--> Product BUILD ID : " << m_config.product_build_id << std::endl;
        std::cout << "--> User Profile Path : " << m_config.user_profile_path << std::endl;

        if ( print_version )
        {
            std::cout << "<-- Deezer native SDK Version : " << dz_connect_get_build_id() << std::endl;
        }
    }
    void register_observer( deezer_wrapper::observer* observer )
    {
        m_observer = observer;
    }
    void set_content( const std::string& content )
    {
        m_ctx.content_url = content;
        std::cout << "CHANGE => " << m_ctx.content_url << std::endl;
    }
    void load_content()
    {
        std::cout << "LOAD => " << m_ctx.content_url << std::endl;
        dz_player_load( m_ctx.dzplayer, nullptr, nullptr,
                        m_ctx.content_url.c_str() );
    }
    std::string get_content()
    {
        return m_ctx.content_url;
    }
    bool active()
    {
        return m_ctx.activation_count > 0;
    }
    void connect()
    {
        dz_error_t dzerr = DZ_ERROR_NO_ERROR;

        m_ctx.dzconnect = dz_connect_new( &m_config );
        if ( m_ctx.dzconnect == nullptr )
        {
            throw deezer_wrapper_exception( "cannot create dzconnect object" );
        }

        std::cout << "Device ID : " << dz_connect_get_device_id( m_ctx.dzconnect ) << std::endl;

        dzerr = dz_connect_debug_log_disable( m_ctx.dzconnect );
        if ( dzerr != DZ_ERROR_NO_ERROR )
        {
            throw deezer_wrapper_exception( "cannot disable debug log" );
        }

        dzerr = dz_connect_activate( m_ctx.dzconnect, this );
        if ( dzerr != DZ_ERROR_NO_ERROR )
        {
            throw deezer_wrapper_exception( "cannot activate connection" );
        }
        m_ctx.activation_count++;

        /* Calling dz_connect_cache_path_set()
         * is mandatory in order to have the attended behavior */
        dz_connect_cache_path_set( m_ctx.dzconnect, nullptr, nullptr, USER_CACHE_PATH );

        m_ctx.dzplayer = dz_player_new( m_ctx.dzconnect );
        if ( m_ctx.dzplayer == nullptr )
        {
            throw deezer_wrapper_exception( "cannot create dzplayer object" );
        }

        dzerr = dz_player_activate( m_ctx.dzplayer, this );
        if ( dzerr != DZ_ERROR_NO_ERROR )
        {
            throw deezer_wrapper_exception( "cannot activate player" );
        }
        m_ctx.activation_count++;

        dzerr = dz_player_set_event_cb( m_ctx.dzplayer, deezer_wrapper_impl::_static_player_callback );
        if ( dzerr != DZ_ERROR_NO_ERROR )
        {
            throw deezer_wrapper_exception( "cannot set event callback" );
        }

        dzerr = dz_player_set_index_progress_cb( m_ctx.dzplayer, deezer_wrapper_impl::_static_index_progress_callback, 1000000/*1s*/ );
        if ( dzerr != DZ_ERROR_NO_ERROR )
        {
            throw deezer_wrapper_exception( "cannot set index progress callback" );
        }

        dzerr = dz_player_set_render_progress_cb( m_ctx.dzplayer, deezer_wrapper_impl::_static_render_progress_callback, 1000000/*1s*/ );
        if ( dzerr != DZ_ERROR_NO_ERROR )
        {
            throw deezer_wrapper_exception( "cannot set render progress callback" );
        }

        dzerr = dz_player_set_metadata_cb( m_ctx.dzplayer, deezer_wrapper_impl::_static_metadata_callback );
        if ( dzerr != DZ_ERROR_NO_ERROR )
        {
            throw deezer_wrapper_exception( "cannot set metadata callback" );
        }

        dzerr = dz_player_set_output_volume( m_ctx.dzplayer, nullptr, nullptr, 20 );
        if ( dzerr != DZ_ERROR_NO_ERROR )
        {
            throw deezer_wrapper_exception( "cannot set output volume" );
        }

        dzerr = dz_player_set_crossfading_duration( m_ctx.dzplayer, nullptr, nullptr, 3000 );
        if ( dzerr != DZ_ERROR_NO_ERROR )
        {
            throw deezer_wrapper_exception( "cannot set crossfading duration" );
        }

        m_ctx.repeat_mode = DZ_QUEUELIST_REPEAT_MODE_OFF;
        m_ctx.is_shuffle_mode = false;

        dzerr = dz_connect_set_access_token( m_ctx.dzconnect, nullptr, nullptr, USER_ACCESS_TOKEN );
        if ( dzerr != DZ_ERROR_NO_ERROR )
        {
            throw deezer_wrapper_exception( "cannot set access token" );
        }

        /* Calling dz_connect_offline_mode(FALSE) is mandatory to force the login */
        dzerr = dz_connect_offline_mode( m_ctx.dzconnect, nullptr, nullptr, false );
        if ( dzerr != DZ_ERROR_NO_ERROR )
        {
            throw deezer_wrapper_exception( "cannot enforce mandatory login" );
        }
    }
    void disconnect()
    {
        if ( m_ctx.dzplayer )
        {
            std::cout << "-- DEACTIVATE & RELEASE PLAYER @" << m_ctx.dzplayer << " --" << std::endl;
            dz_player_deactivate( m_ctx.dzplayer, deezer_wrapper_impl::_static_player_on_deactivate, nullptr );
            dz_object_release( reinterpret_cast<dz_object_handle>( m_ctx.dzplayer ) );
            m_ctx.dzplayer = nullptr;
        }

        if ( m_ctx.dzconnect )
        {
            std::cout << "-- DEACTIVATE & RELEASE CONNECT @" << m_ctx.dzconnect << " --" << std::endl;
            dz_connect_deactivate( m_ctx.dzconnect, deezer_wrapper_impl::_static_connect_on_deactivate, nullptr );
            dz_object_release( reinterpret_cast<dz_object_handle>( m_ctx.dzconnect ) );
            m_ctx.dzconnect = nullptr;
        }
    }
    void playback_start()
    {
        std::cout << "PLAY track n° " << m_ctx.track_played_count << " of => " << m_ctx.content_url << std::endl;
        dz_player_play( m_ctx.dzplayer, nullptr, nullptr,
                        DZ_PLAYER_PLAY_CMD_START_TRACKLIST,
                        DZ_INDEX_IN_QUEUELIST_CURRENT );
    }
    void playback_stop()
    {
        std::cout << "STOP => " << m_ctx.content_url << std::endl;
        dz_player_stop( m_ctx.dzplayer, nullptr, nullptr );
    }
    void playback_pause()
    {
        std::cout << "PAUSE track n° " << m_ctx.track_played_count << " of => " << m_ctx.content_url << std::endl;
        dz_player_pause( m_ctx.dzplayer, nullptr, nullptr );
    }
    void playback_resume()
    {
        std::cout << "RESUME track n° " << m_ctx.track_played_count << " of => " << m_ctx.content_url << std::endl;
        dz_player_resume( m_ctx.dzplayer, nullptr, nullptr );
    }
    void playback_seek( int position_ms )
    {
        std::cout << "SEEK track n° " << m_ctx.track_played_count << " of => " << m_ctx.content_url << " @" << position_ms << "ms" << std::endl;
        dz_player_seek( m_ctx.dzplayer, nullptr, nullptr, position_ms * 1000 );
    }
    void playback_toogle_repeat()
    {
        switch( m_ctx.repeat_mode )
        {
            case DZ_QUEUELIST_REPEAT_MODE_OFF:
                m_ctx.repeat_mode = DZ_QUEUELIST_REPEAT_MODE_ONE;
                break;
            case DZ_QUEUELIST_REPEAT_MODE_ONE:
                m_ctx.repeat_mode = DZ_QUEUELIST_REPEAT_MODE_ALL;
                break;
            case DZ_QUEUELIST_REPEAT_MODE_ALL:
                m_ctx.repeat_mode = DZ_QUEUELIST_REPEAT_MODE_OFF;
                break;
        }

        std::cout << "REPEAT mode => " << m_ctx.repeat_mode << std::endl;

        dz_player_set_repeat_mode(  m_ctx.dzplayer, nullptr, nullptr,
                                    m_ctx.repeat_mode );
    }
    void playback_toogle_random()
    {
        m_ctx.is_shuffle_mode = !m_ctx.is_shuffle_mode;

        std::cout << "SHUFFLE mode => " << std::string( m_ctx.is_shuffle_mode ? "ON" : "OFF" ) << std::endl;

        dz_player_enable_shuffle_mode(  m_ctx.dzplayer, nullptr, nullptr,
                                        m_ctx.is_shuffle_mode );
    }
    void playback_next()
    {
        std::cout << "NEXT => " << m_ctx.content_url << std::endl;
        dz_player_play( m_ctx.dzplayer, nullptr, nullptr,
                        DZ_PLAYER_PLAY_CMD_START_TRACKLIST,
                        DZ_INDEX_IN_QUEUELIST_NEXT );
    }
    void playback_previous()
    {
        std::cout << "PREVIOUS => " << m_ctx.content_url << std::endl;
        dz_player_play( m_ctx.dzplayer, nullptr, nullptr,
                        DZ_PLAYER_PLAY_CMD_START_TRACKLIST,
                        DZ_INDEX_IN_QUEUELIST_PREVIOUS );
    }
    void play_audioads()
    {
        dz_player_play_audioads( m_ctx.dzplayer, nullptr, nullptr );
    }
    const deezer_wrapper::track_infos& current_track_infos()
    {
        return m_current_track_infos;
    }
private:
    // callback for dzconnect events
    static void _static_connect_callback(   dz_connect_handle handle,
                                            dz_connect_event_handle event,
                                            void* delegate )
    {
        reinterpret_cast<deezer_wrapper_impl*>( delegate )->_connect_callback( handle, event );
    }
    void _connect_callback( dz_connect_handle handle,
                            dz_connect_event_handle event )
    {
        auto type = dz_connect_event_get_type( event );
        connect_event output_event;

        switch( type )
        {
            case DZ_CONNECT_EVENT_USER_OFFLINE_AVAILABLE:
                std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ USER_OFFLINE_AVAILABLE" << std::endl;
                output_event = connect_event::user_offline_available;
                break;

            case DZ_CONNECT_EVENT_USER_ACCESS_TOKEN_OK:
                {
                    const char* szAccessToken = dz_connect_event_get_access_token( event );
                    std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ USER_ACCESS_TOKEN_OK Access_token : " << szAccessToken << std::endl;
                }
                output_event = connect_event::user_access_token_ok;
                break;

            case DZ_CONNECT_EVENT_USER_ACCESS_TOKEN_FAILED:
                std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ USER_ACCESS_TOKEN_FAILED" << std::endl;
                output_event = connect_event::user_access_token_failed;
                break;

            case DZ_CONNECT_EVENT_USER_LOGIN_OK:
                std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ USER_LOGIN_OK" << std::endl;
                output_event = connect_event::user_login_ok;
                break;

            case DZ_CONNECT_EVENT_USER_NEW_OPTIONS:
                std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ USER_NEW_OPTIONS" << std::endl;
                output_event = connect_event::user_new_options;
                break;

            case DZ_CONNECT_EVENT_USER_LOGIN_FAIL_NETWORK_ERROR:
                std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ USER_LOGIN_FAIL_NETWORK_ERROR" << std::endl;
                output_event = connect_event::user_login_fail_network_error;
                break;

            case DZ_CONNECT_EVENT_USER_LOGIN_FAIL_BAD_CREDENTIALS:
                std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ USER_LOGIN_FAIL_BAD_CREDENTIALS" << std::endl;
                output_event = connect_event::user_login_fail_bad_credentials;
                break;

            case DZ_CONNECT_EVENT_USER_LOGIN_FAIL_USER_INFO:
                std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ USER_LOGIN_FAIL_USER_INFO" << std::endl;
                output_event = connect_event::user_login_fail_user_info;
                break;

            case DZ_CONNECT_EVENT_USER_LOGIN_FAIL_OFFLINE_MODE:
                std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ USER_LOGIN_FAIL_OFFLINE_MODE" << std::endl;
                output_event = connect_event::user_login_fail_offline_mode;
                break;

            case DZ_CONNECT_EVENT_ADVERTISEMENT_START:
                std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ ADVERTISEMENT_START" << std::endl;
                output_event = connect_event::advertisement_start;
                break;

            case DZ_CONNECT_EVENT_ADVERTISEMENT_STOP:
                std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ ADVERTISEMENT_STOP" << std::endl;
                output_event = connect_event::advertisement_stop;
                break;

            case DZ_CONNECT_EVENT_UNKNOWN:
            default:
                std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ UNKNOWN or default (type = " << type << ")" << std::endl;
                output_event = connect_event::unknown;
                break;
        }

        if ( m_observer )
            m_observer->on_connect_event( output_event );
    }
    // callback for dzplayer events
    static void _static_player_callback(    dz_player_handle handle,
                                            dz_player_event_handle event,
                                            void* delegate )
    {
        reinterpret_cast<deezer_wrapper_impl*>( delegate )->_player_callback( handle, event );
    }
    void _player_callback(  dz_player_handle handle,
                            dz_player_event_handle event )
    {
        dz_streaming_mode_t streaming_mode;
        dz_index_in_queuelist idx;

        auto type = dz_player_event_get_type(event);
        player_event output_event;

        if ( !dz_player_event_get_queuelist_context( event, &streaming_mode, &idx ) )
        {
            streaming_mode = DZ_STREAMING_MODE_ONDEMAND;
            idx = DZ_INDEX_IN_QUEUELIST_INVALID;
        }

        switch( type )
        {
            case DZ_PLAYER_EVENT_LIMITATION_FORCED_PAUSE:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== LIMITATION_FORCED_PAUSE for idx: " << idx << std::endl;
                output_event = player_event::limitation_forced_pause;
                break;

            case DZ_PLAYER_EVENT_QUEUELIST_LOADED:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== QUEUELIST_LOADED for idx: " << idx << std::endl;
                output_event = player_event::queuelist_loaded;
                break;

            case DZ_PLAYER_EVENT_QUEUELIST_NO_RIGHT:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== QUEUELIST_NO_RIGHT for idx: " << idx << std::endl;
                output_event = player_event::queuelist_no_right;
                break;

            case DZ_PLAYER_EVENT_QUEUELIST_NEED_NATURAL_NEXT:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== QUEUELIST_NEED_NATURAL_NEXT for idx: " << idx << std::endl;
                output_event = player_event::queuelist_need_natural_next;
                break;

            case DZ_PLAYER_EVENT_QUEUELIST_TRACK_NOT_AVAILABLE_OFFLINE:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== QUEUELIST_TRACK_NOT_AVAILABLE_OFFLINE for idx: " << idx << std::endl;
                output_event = player_event::queuelist_track_not_available_offline;
                break;

            case DZ_PLAYER_EVENT_QUEUELIST_TRACK_RIGHTS_AFTER_AUDIOADS:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== QUEUELIST_TRACK_RIGHTS_AFTER_AUDIOADS for idx: " << idx << std::endl;
                output_event = player_event::queuelist_track_rights_after_audioads;
                break;

            case DZ_PLAYER_EVENT_QUEUELIST_SKIP_NO_RIGHT:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== QUEUELIST_SKIP_NO_RIGHT for idx: " << idx << std::endl;
                output_event = player_event::queuelist_skip_no_right;
                break;

            case DZ_PLAYER_EVENT_QUEUELIST_TRACK_SELECTED:
                {
                    bool is_preview;
                    bool can_pause_unpause;
                    bool can_seek;
                    int  nb_skip_allowed;

                    is_preview = dz_player_event_track_selected_is_preview( event );

                    dz_player_event_track_selected_rights( event, &can_pause_unpause, &can_seek, &nb_skip_allowed );

                    auto* selected_dzapiinfo = dz_player_event_track_selected_dzapiinfo( event );
                    auto* next_dzapiinfo = dz_player_event_track_selected_next_track_dzapiinfo( event );

                    std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== QUEUELIST_TRACK_SELECTED for idx: " << idx << " - is_preview: " << is_preview << std::endl;
                    std::cout << "\tcan_pause_unpause:" << can_pause_unpause << " can_seek:" << can_seek << " nb_skip_allowed:" << nb_skip_allowed << std::endl;
                    if ( selected_dzapiinfo )
                    {
                        std::cout << "\tnow:" << selected_dzapiinfo << std::endl;
                        try
                        {
                            using json = nlohmann::json;
                            auto json_infos = json::parse( selected_dzapiinfo );
                            m_current_track_infos.title = json_infos["title"].get<std::string>();
                            m_current_track_infos.artist = json_infos["artist"]["name"].get<std::string>();
                            m_current_track_infos.duration = json_infos["duration"].get<int>();
                            m_current_track_infos.album_title = json_infos["album"]["title"].get<std::string>();
                            m_current_track_infos.cover_art = json_infos["album"]["cover"].get<std::string>();
                        }
                        catch( std::exception& e )
                        {
                            std::cerr << "error parsing track infos : " << e.what() << std::endl;
                        }
                    }
                    if ( next_dzapiinfo )
                        std::cout << "\tnext:" << next_dzapiinfo << std::endl;
                }
                m_ctx.track_played_count++;
                output_event = player_event::queuelist_track_selected;
                break;

            case DZ_PLAYER_EVENT_MEDIASTREAM_DATA_READY:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== MEDIASTREAM_DATA_READY for idx: " << idx << std::endl;
                output_event = player_event::mediastream_data_ready;
                break;

            case DZ_PLAYER_EVENT_MEDIASTREAM_DATA_READY_AFTER_SEEK:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== MEDIASTREAM_DATA_READY_AFTER_SEEK for idx: " << idx << std::endl;
                output_event = player_event::mediastream_data_ready_after_seek;
                break;

            case DZ_PLAYER_EVENT_RENDER_TRACK_START_FAILURE:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== RENDER_TRACK_START_FAILURE for idx: " << idx << std::endl;
                output_event = player_event::render_track_start_failure;
                break;

            case DZ_PLAYER_EVENT_RENDER_TRACK_START:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== RENDER_TRACK_START for idx: " << idx << std::endl;
                output_event = player_event::render_track_start;
                break;

            case DZ_PLAYER_EVENT_RENDER_TRACK_END:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== RENDER_TRACK_END for idx: " << idx << std::endl;
                std::cout << "- track_played_count : " << m_ctx.track_played_count << std::endl;
                // Detect if we come from from playing an ad, if yes restart automatically the playback.
                if ( idx == DZ_INDEX_IN_QUEUELIST_INVALID )
                {
                    std::cout << "NOT VERY SURE ABOUT THIS..." << std::endl;
                    playback_start(); // TODO : not very sure about that...
                }
                output_event = player_event::render_track_end;
                break;

            case DZ_PLAYER_EVENT_RENDER_TRACK_PAUSED:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== RENDER_TRACK_PAUSED for idx: " << idx << std::endl;
                output_event = player_event::render_track_paused;
                break;

            case DZ_PLAYER_EVENT_RENDER_TRACK_UNDERFLOW:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== RENDER_TRACK_UNDERFLOW for idx: " << idx << std::endl;
                output_event = player_event::render_track_underflow;
                break;

            case DZ_PLAYER_EVENT_RENDER_TRACK_RESUMED:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== RENDER_TRACK_RESUMED for idx: " << idx << std::endl;
                output_event = player_event::render_track_resumed;
                break;

            case DZ_PLAYER_EVENT_RENDER_TRACK_SEEKING:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== RENDER_TRACK_SEEKING for idx: " << idx << std::endl;
                output_event = player_event::render_track_seeking;
                break;

            case DZ_PLAYER_EVENT_RENDER_TRACK_REMOVED:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== RENDER_TRACK_REMOVED for idx: " << idx << std::endl;
                output_event = player_event::render_track_removed;
                break;

            case DZ_PLAYER_EVENT_UNKNOWN:
            default:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== UNKNOWN or default (type = " << type << ")" << std::endl;
                output_event = player_event::unknown;
                break;
        }

        if ( m_observer )
            m_observer->on_player_event( output_event );
    }
    static void _static_connect_on_deactivate(  void* delegate,
                                                void* operation_userdata,
                                                dz_error_t status,
                                                dz_object_handle result)
    {
        reinterpret_cast<deezer_wrapper_impl*>( delegate )->_connect_on_deactivate( operation_userdata, status, result );
    }
    void _connect_on_deactivate(    void* operation_userdata,
                                    dz_error_t status,
                                    dz_object_handle result)
    {
        m_ctx.activation_count--;
        std::cout << "CONNECT deactivated - c = " << m_ctx.activation_count << " with status = " << status << std::endl;
    }
    static void _static_player_on_deactivate(   void* delegate,
                                                void* operation_userdata,
                                                dz_error_t status,
                                                dz_object_handle result )
    {
        reinterpret_cast<deezer_wrapper_impl*>( delegate )->_player_on_deactivate( operation_userdata, status, result );
    }
    void _player_on_deactivate( void* operation_userdata,
                                dz_error_t status,
                                dz_object_handle result )
    {
        m_ctx.activation_count--;
        std::cout << "PLAYER deactivated - c = " << m_ctx.activation_count << " with status = " << status << std::endl;
    }
    static void _static_index_progress_callback(    dz_player_handle handle,
                                                    dz_useconds_t progress,
                                                    void* delegate )
    {
        reinterpret_cast<deezer_wrapper_impl*>( delegate )->_index_progress_callback( progress );
    }
    void _index_progress_callback( dz_useconds_t progress )
    {
        //std::cout << "INDEX_PROGRESS " << progress << std::endl;
        if ( m_observer )
            m_observer->on_index_progress( static_cast<int>( progress / 1000 ) );
    }
    static void _static_render_progress_callback(   dz_player_handle handle,
                                                    dz_useconds_t progress,
                                                    void* delegate )
    {
        reinterpret_cast<deezer_wrapper_impl*>( delegate )->_render_progress_callback( progress );
    }
    void _render_progress_callback( dz_useconds_t progress )
    {
        //std::cout << "RENDER_PROGRESS " << progress << std::endl;
        if ( m_observer )
            m_observer->on_render_progress( static_cast<int>( progress / 1000 ) );
    }
    static void _static_metadata_callback(  dz_player_handle handle,
                                            dz_track_metadata_handle metadata,
                                            void* delegate )
    {
        reinterpret_cast<deezer_wrapper_impl*>( delegate )->_metadata_callback( metadata );
    }
    void _metadata_callback( dz_track_metadata_handle metadata )
    {
        auto type = dz_track_metadata_get_type( metadata );
        switch( type )
        {
        case DZ_TRACK_METADATA_UNKNOWN:
            std::cout << "UNKNOWN METADATA" << std::endl;
            break;
        case DZ_TRACK_METADATA_FORMAT_HEADER:
            std::cout << "FORMAT HEADER METADATA" << std::endl;
            break;
        case DZ_TRACK_METADATA_DURATION_MS:
            std::cout << "DURATION MS METADATA" << std::endl;
            if ( m_observer )
                m_observer->on_track_duration( dz_track_metadata_get_duration( metadata ) );
            break;
        }
    }
private:

    deezer_wrapper::observer* m_observer = nullptr;
    deezer_wrapper::track_infos m_current_track_infos = {};

    dz_connect_configuration m_config;
    wrapper_context m_ctx;
};

deezer_wrapper::deezer_wrapper( const std::string& app_id,
                                const std::string& product_id,
                                const std::string& product_build_id,
                                bool print_version  )
    : m_pimpl( std::make_unique<deezer_wrapper_impl>( app_id, product_id, product_build_id, print_version ) )
{
}

deezer_wrapper::~deezer_wrapper()
{
}

void deezer_wrapper::register_observer( deezer_wrapper::observer* observer )
{
    m_pimpl->register_observer( observer );
}

void deezer_wrapper::set_content( const std::string& content )
{
    m_pimpl->set_content( content );
}

void deezer_wrapper::load_content()
{
    m_pimpl->load_content();
}

std::string deezer_wrapper::get_content()
{
    return m_pimpl->get_content();
}

bool deezer_wrapper::active()
{
    return m_pimpl->active();
}

void deezer_wrapper::connect()
{
    m_pimpl->connect();
}

void deezer_wrapper::disconnect()
{
    m_pimpl->disconnect();
}

void deezer_wrapper::playback_start()
{
    m_pimpl->playback_start();
}

void deezer_wrapper::playback_stop()
{
    m_pimpl->playback_stop();
}

void deezer_wrapper::playback_pause()
{
    m_pimpl->playback_pause();
}

void deezer_wrapper::playback_resume()
{
    m_pimpl->playback_resume();
}

void deezer_wrapper::playback_seek( int position_ms )
{
    m_pimpl->playback_seek( position_ms );
}

void deezer_wrapper::playback_toogle_repeat()
{
    m_pimpl->playback_toogle_repeat();
}

void deezer_wrapper::playback_toogle_random()
{
    m_pimpl->playback_toogle_random();
}

void deezer_wrapper::playback_next()
{
    m_pimpl->playback_next();
}

void deezer_wrapper::playback_previous()
{
    m_pimpl->playback_previous();
}

void deezer_wrapper::play_audioads()
{
    m_pimpl->play_audioads();
}

const deezer_wrapper::track_infos& deezer_wrapper::current_track_infos()
{
    return m_pimpl->current_track_infos();
}
