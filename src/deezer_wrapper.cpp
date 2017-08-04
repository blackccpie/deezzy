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

#include <deezer-connect.h>
#include <deezer-player.h>

#include <iostream>

using namespace std::placeholders;

class deezer_wrapper::deezer_wrapper_impl
{
private:
    struct wrapper_context
    {
        int track_played_count = 0;
        int activation_count = 0;
        bool is_shuffle_mode = false;
        bool is_playing = false;
        std::string content_url;
        dz_connect_handle dzconnect = nullptr;
        dz_player_handle dzplayer = nullptr;
        dz_queuelist_repeat_mode_t repeat_mode = DZ_QUEUELIST_REPEAT_MODE_OFF;
    };
public:
    deezer_wrapper_impl(    const std::string& app_id,
                            const std::string& product_id,
                            const std::string& product_build_id,
                            bool print_version ) : m_config{0}
    {
        m_config.app_id            = app_id.c_str();
        m_config.product_id        = product_id.c_str();
        m_config.product_build_id  = product_build_id.c_str();
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
        dz_connect_event_t type = dz_connect_event_get_type( event );

        switch (type)
        {
            case DZ_CONNECT_EVENT_USER_OFFLINE_AVAILABLE:
                std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ USER_OFFLINE_AVAILABLE" << std::endl;
                break;

            case DZ_CONNECT_EVENT_USER_ACCESS_TOKEN_OK:
                {
                    const char* szAccessToken;
                    szAccessToken = dz_connect_event_get_access_token( event );
                    std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ USER_ACCESS_TOKEN_OK Access_token : " << szAccessToken << std::endl;
                }
                break;

            case DZ_CONNECT_EVENT_USER_ACCESS_TOKEN_FAILED:
                std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ USER_ACCESS_TOKEN_FAILED" << std::endl;
                break;

            case DZ_CONNECT_EVENT_USER_LOGIN_OK:
                std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ USER_LOGIN_OK" << std::endl;
                // TODO app_load_content();
                break;

            case DZ_CONNECT_EVENT_USER_NEW_OPTIONS:
                std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ USER_NEW_OPTIONS" << std::endl;
                break;

            case DZ_CONNECT_EVENT_USER_LOGIN_FAIL_NETWORK_ERROR:
                std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ USER_LOGIN_FAIL_NETWORK_ERROR" << std::endl;
                break;

            case DZ_CONNECT_EVENT_USER_LOGIN_FAIL_BAD_CREDENTIALS:
                std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ USER_LOGIN_FAIL_BAD_CREDENTIALS" << std::endl;
                break;

            case DZ_CONNECT_EVENT_USER_LOGIN_FAIL_USER_INFO:
                std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ USER_LOGIN_FAIL_USER_INFO" << std::endl;
                break;

            case DZ_CONNECT_EVENT_USER_LOGIN_FAIL_OFFLINE_MODE:
                std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ USER_LOGIN_FAIL_OFFLINE_MODE" << std::endl;
                break;

            case DZ_CONNECT_EVENT_ADVERTISEMENT_START:
                std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ ADVERTISEMENT_START" << std::endl;
                break;

            case DZ_CONNECT_EVENT_ADVERTISEMENT_STOP:
                std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ ADVERTISEMENT_STOP" << std::endl;
                break;

            case DZ_CONNECT_EVENT_UNKNOWN:
            default:
                std::cout << "(App:" << &m_ctx << ") ++++ CONNECT_EVENT ++++ UNKNOWN or default (type = " << type << ")" << std::endl;
                break;
        }
    }
    // callback for dzplayer events
    static void _static_player_callback(   dz_player_handle handle,
                                    dz_player_event_handle event,
                                    void* delegate )
    {
        reinterpret_cast<deezer_wrapper_impl*>( delegate )->_player_callback( handle, event );
    }
    void _player_callback(  dz_player_handle handle,
                            dz_player_event_handle event )
    {
        dz_streaming_mode_t   streaming_mode;
        dz_index_in_queuelist idx;

        auto type = dz_player_event_get_type(event);

        if ( !dz_player_event_get_queuelist_context( event, &streaming_mode, &idx ) )
        {
            streaming_mode = DZ_STREAMING_MODE_ONDEMAND;
            idx = DZ_INDEX_IN_QUEUELIST_INVALID;
        }

        switch( type )
        {
            case DZ_PLAYER_EVENT_LIMITATION_FORCED_PAUSE:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== LIMITATION_FORCED_PAUSE for idx: " << idx << std::endl;
                break;

            case DZ_PLAYER_EVENT_QUEUELIST_LOADED:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== QUEUELIST_LOADED for idx: " << idx << std::endl;
                //app_playback_start_or_stop();
                break;

            case DZ_PLAYER_EVENT_QUEUELIST_NO_RIGHT:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== QUEUELIST_NO_RIGHT for idx: " << idx << std::endl;
                break;

            case DZ_PLAYER_EVENT_QUEUELIST_NEED_NATURAL_NEXT:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== QUEUELIST_NEED_NATURAL_NEXT for idx: " << idx << std::endl;
                break;

            case DZ_PLAYER_EVENT_QUEUELIST_TRACK_NOT_AVAILABLE_OFFLINE:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== QUEUELIST_TRACK_NOT_AVAILABLE_OFFLINE for idx: " << idx << std::endl;
                break;

            case DZ_PLAYER_EVENT_QUEUELIST_TRACK_RIGHTS_AFTER_AUDIOADS:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== QUEUELIST_TRACK_RIGHTS_AFTER_AUDIOADS for idx: " << idx << std::endl;
                dz_player_play_audioads( m_ctx.dzplayer, nullptr, nullptr );
                break;

            case DZ_PLAYER_EVENT_QUEUELIST_SKIP_NO_RIGHT:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== QUEUELIST_SKIP_NO_RIGHT for idx: " << idx << std::endl;
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

                    std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== QUEUELIST_TRACK_SELECTED for idx: " << idx << " - is_preview: " << is_preview << std::endl;
                    std::cout << "\tcan_pause_unpause:" << can_pause_unpause << " can_seek:" << can_seek << " nb_skip_allowed:" << nb_skip_allowed << std::endl;
                    if (selected_dzapiinfo)
                        std::cout << "\tnow:" << selected_dzapiinfo << std::endl;
                    if (next_dzapiinfo)
                        std::cout << "\tnext:" << next_dzapiinfo << std::endl;
                }
                m_ctx.track_played_count++;
                break;

            case DZ_PLAYER_EVENT_MEDIASTREAM_DATA_READY:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== MEDIASTREAM_DATA_READY for idx: " << idx << std::endl;
                break;

            case DZ_PLAYER_EVENT_MEDIASTREAM_DATA_READY_AFTER_SEEK:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== MEDIASTREAM_DATA_READY_AFTER_SEEK for idx: " << idx << std::endl;
                break;

            case DZ_PLAYER_EVENT_RENDER_TRACK_START_FAILURE:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== RENDER_TRACK_START_FAILURE for idx: " << idx << std::endl;
                m_ctx.is_playing = false;
                break;

            case DZ_PLAYER_EVENT_RENDER_TRACK_START:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== RENDER_TRACK_START for idx: " << idx << std::endl;
                m_ctx.is_playing = true;
                break;

            case DZ_PLAYER_EVENT_RENDER_TRACK_END:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== RENDER_TRACK_END for idx: " << idx << std::endl;
                m_ctx.is_playing = false;
                std::cout << "- track_played_count : " << m_ctx.track_played_count << std::endl;
                // Detect if we come from from playing an ad, if yes restart automatically the playback.
                if (idx == DZ_INDEX_IN_QUEUELIST_INVALID)
                {
                    // TODO app_playback_start_or_stop();
                }
                break;

            case DZ_PLAYER_EVENT_RENDER_TRACK_PAUSED:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== RENDER_TRACK_PAUSED for idx: " << idx << std::endl;
                m_ctx.is_playing = false;
                break;

            case DZ_PLAYER_EVENT_RENDER_TRACK_UNDERFLOW:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== RENDER_TRACK_UNDERFLOW for idx: " << idx << std::endl;
                m_ctx.is_playing = false;
                break;

            case DZ_PLAYER_EVENT_RENDER_TRACK_RESUMED:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== RENDER_TRACK_RESUMED for idx: " << idx << std::endl;
                m_ctx.is_playing = true;
                break;

            case DZ_PLAYER_EVENT_RENDER_TRACK_SEEKING:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== RENDER_TRACK_SEEKING for idx: " << idx << std::endl;
                m_ctx.is_playing = false;
                break;

            case DZ_PLAYER_EVENT_RENDER_TRACK_REMOVED:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== RENDER_TRACK_REMOVED for idx: " << idx << std::endl;
                m_ctx.is_playing = false;
                break;

            case DZ_PLAYER_EVENT_UNKNOWN:
            default:
                std::cout << "(App:" << &m_ctx << ") ==== PLAYER_EVENT ==== UNKNOWN or default (type = " << type << ")" << std::endl;
                break;
        }
    }
    static void _static_connect_on_deactivate( void* delegate,
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
    static void _static_player_on_deactivate(  void* delegate,
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
private:
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

void deezer_wrapper::set_content( const std::string& content )
{
    m_pimpl->set_content( content );
}

void deezer_wrapper::load_content()
{
    m_pimpl->load_content();
}

void deezer_wrapper::connect()
{
    m_pimpl->connect();
}

void deezer_wrapper::disconnect()
{
    m_pimpl->disconnect();
}
