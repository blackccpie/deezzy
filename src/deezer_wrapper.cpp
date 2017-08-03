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

class deezer_wrapper_impl
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
    deezer_wrapper_impl( bool print_version )
    {
        if ( print_version )
        {
            std::cout << "<-- Deezer native SDK Version : " << dz_connect_get_build_id() << std::endl;
        }
    }
    void set_content( const std::string& content )
    {
        app_ctxt->sz_content_url = content;
        std::cout << "LOAD => " << app_ctxt->sz_content_url << std::endl;
        dz_player_load( app_ctxt->dzplayer, nullptr, nullptr,
                        app_ctxt->sz_content_url.c_str() );
    }
    void connect( const dz_connect_configuration& config )
    {
        dz_error_t dzerr = DZ_ERROR_NO_ERROR;

        std::cout << "--> Application ID : " << config.app_id << std::endl;
        std::cout << "--> Product ID : " << config.product_id << std::endl;
        std::cout << "--> Product BUILD ID : " << config.product_build_id << std::endl;
        std::cout << "--> User Profile Path : " << config.user_profile_path << std::endl;

        m_ctx->dzconnect = dz_connect_new( &config );

        if ( m_ctx->dzconnect == nullptr )
        {
            std::cout << "dzconnect null" << std::endl;
            return -1;
        }

        if ( print_device_id )
        {
            std::cout << "Device ID : " << dz_connect_get_device_id( m_ctx->dzconnect ) << std::endl;
        }

        dzerr = dz_connect_debug_log_disable( m_ctx->dzconnect );
        if ( dzerr != DZ_ERROR_NO_ERROR )
        {
            std::cout << "dz_connect_debug_log_disable error" << std::endl;
            return -1;
        }

        dzerr = dz_connect_activate( m_ctx->dzconnect, m_ctx );
        if ( dzerr != DZ_ERROR_NO_ERROR )
        {
            std::cout << "dz_connect_activate error" << std::endl;
            return -1;
        }
        m_ctx->activation_count++;

        /* Calling dz_connect_cache_path_set()
         * is mandatory in order to have the attended behavior */
        dz_connect_cache_path_set( m_ctx->dzconnect, nullptr, nullptr, USER_CACHE_PATH );

        m_ctx->dzplayer = dz_player_new( m_ctx->dzconnect );
        if ( m_ctx->dzplayer == nullptr ) {
            std::cout << "dzplayer null" << std::endl;
            return -1;
        }

        dzerr = dz_player_activate( m_ctx->dzplayer, m_ctx );
        if ( dzerr != DZ_ERROR_NO_ERROR )
        {
            std::cout << "dz_player_activate error" << std::endl;
            return -1;
        }
        m_ctx->activation_count++;

        dzerr = dz_player_set_event_cb( m_ctx->dzplayer, std::bind( &deezer_wrapper_impl::_player_callback, this ) );
        if (dzerr != DZ_ERROR_NO_ERROR) {
            std::cout << "dz_player_set_event_cb error" << std::endl;
            return -1;
        }

        dzerr = dz_player_set_output_volume( m_ctx->dzplayer, nullptr, nullptr, 20 );
        if (dzerr != DZ_ERROR_NO_ERROR) {
            std::cout << "dz_player_set_output_volume error" << std::endl;
            return -1;
        }

        dzerr = dz_player_set_crossfading_duration( m_ctx->dzplayer, nullptr, nullptr, 3000 );
        if (dzerr != DZ_ERROR_NO_ERROR) {
            std::cout << "dz_player_set_crossfading_duration error" << std::endl;
            return -1;
        }

        m_ctx->repeat_mode = DZ_QUEUELIST_REPEAT_MODE_OFF;
        m_ctx->is_shuffle_mode = false;

        dzerr = dz_connect_set_access_token( m_ctx->dzconnect, nullptr, nullptr, USER_ACCESS_TOKEN );
        if (dzerr != DZ_ERROR_NO_ERROR) {
            std::cout << "dz_connect_set_access_token error" << std::endl;
            return -1;
        }

        /* Calling dz_connect_offline_mode(FALSE) is mandatory to force the login */
        dzerr = dz_connect_offline_mode( m_ctx->dzconnect, nullptr, nullptr, false );
        if (dzerr != DZ_ERROR_NO_ERROR) {
            std::cout << "dz_connect_offline_mode error" << std::endl;
            return -1;
        }
    }
    void disconnect()
    {
        if ( m_ctx->dzplayer )
        {
            std::cout << "-- DEACTIVATE & RELEASE PLAYER @" << m_ctx->dzplayer << " --" << std::endl;
            dz_player_deactivate( app_ctxt->dzplayer, dz_player_on_deactivate, nullptr );
            dz_object_release( static_cast<dz_object_handle>( m_ctx->dzplayer ) );
            m_ctx->dzplayer = nullptr;
        }

        if ( m_ctx->dzconnect )
        {
            std::cout << "-- DEACTIVATE & RELEASE CONNECT @" << m_ctx->dzconnect << " --" << std::endl;
            dz_connect_deactivate( app_ctxt->dzconnect, dz_connect_on_deactivate, nullptr );
            dz_object_release( static_cast<dz_object_handle>( m_ctx->dzconnect ) );
            m_ctx->dzconnect = nullptr;
        }
    }
private:
    // callback for dzconnect events
    void app_connect_callback(  dz_connect_handle handle,
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
    // callback for dzplayer events
    void app_player_callback( dz_player_handle handle,
                                dz_player_event_handle event,
                                void* supervisor );
private:
    wrapper_context m_ctx;
};

deezer_wrapper::deezer_wrapper( bool print_version )
    : m_pimpl( std::make_unique<deezer_wrapper_impl>( print_version ) )
{
}

void deezer_wrapper::set_content( const std::string& content )
{
    m_pimpl->set_content( content );
}

void deezer_wrapper::connect( const dz_connect_configuration& config )
{
    m_pimpl->connect( config );
}

void deezer_wrapper::disconnect()
{
    m_pimpl->disconnect();
}
