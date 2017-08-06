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

#include <QtQml>
#include <QQmlApplicationEngine>
#include <QGuiApplication>

#include <iostream>

#define DEEZZY_APPLICATION_ID      "247082"	// SET YOUR APPLICATION ID
#define DEEZZY_APPLICATION_NAME    "Deezzy" 	// SET YOUR APPLICATION NAME
#define DEEZZY_APPLICATION_VERSION "00001"	// SET YOUR APPLICATION VERSION

class DeezzyApp :   public QObject,
                    public deezer_wrapper::observer
{
    Q_OBJECT
    Q_ENUMS(PlaybackState)
    Q_PROPERTY(PlaybackState playbackState READ playbackState)
    Q_PROPERTY(QString content READ content WRITE setContent)
public:
    enum class PlaybackState
    {
        Stopped,
        Playing,
        Paused
    };
public:
    DeezzyApp() : m_deezer_wrapper( std::make_shared<deezer_wrapper>(   DEEZZY_APPLICATION_ID,
                                                                        DEEZZY_APPLICATION_NAME,
                                                                        DEEZZY_APPLICATION_VERSION,
                                                                        true /*print_version*/ ) )
    {
    }

    /************ Q_INVOKABLEs ************/

    Q_INVOKABLE bool connect()
    {
        m_deezer_wrapper->register_observer( this );
        m_deezer_wrapper->connect();

        return true;
    }
    Q_INVOKABLE bool disconnect()
    {
        m_deezer_wrapper->register_observer( nullptr );
        m_deezer_wrapper->disconnect();

        return true;
    }
    Q_INVOKABLE bool play()
    {
        m_deezer_wrapper->load_content();
        m_playback_state = PlaybackState::Playing;

        return true;
    }
    Q_INVOKABLE bool stop()
    {
        m_deezer_wrapper->playback_stop();
        m_playback_state = PlaybackState::Stopped;

        return true;
    }
    Q_INVOKABLE bool pause()
    {
        m_deezer_wrapper->playback_pause();
        m_playback_state = PlaybackState::Paused;

        return true;
    }
    Q_INVOKABLE bool resume()
    {
        m_deezer_wrapper->playback_start();
        m_playback_state = PlaybackState::Playing;

        return true;
    }
    Q_INVOKABLE bool next()
    {
        m_deezer_wrapper->playback_next();

        return true;
    }
    Q_INVOKABLE bool previous()
    {
        m_deezer_wrapper->playback_previous();

        return true;
    }

    /************ Q_PROPERTYs ************/

    PlaybackState playbackState() const {
        return m_playback_state;
    }

    QString content()
    {
        return QString::fromStdString( m_deezer_wrapper->get_content() );
    }

    void setContent( const QString& content )
    {
        m_deezer_wrapper->set_content( content.toStdString() );
    }

signals:
    void paused();
    void playing();
    void stopped();
    void error();

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
                m_deezer_wrapper->playback_start();
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
                emit playing();
                break;
            case deezer_wrapper::player_event::render_track_end:
                break;
            case deezer_wrapper::player_event::render_track_paused:
                emit paused();
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
private:

    PlaybackState m_playback_state = PlaybackState::Stopped;

    std::shared_ptr<deezer_wrapper> m_deezer_wrapper;
};
Q_DECLARE_METATYPE(DeezzyApp::PlaybackState)

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<DeezzyApp>("Native.DeezzyApp", 1, 0, "DeezzyApp");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/Deezzy.qml")));

    return app.exec();

    // try
    // {
    //     std::shared_ptr<deezzy_observer> dzr_observer = std::make_shared<deezzy_observer>();
    //     dzr_wrapper.register_observer( dzr_observer );
    //     dzr_wrapper.set_content( argv[1] );
    //     dzr_wrapper.connect();
    //
    //     while ( ( dzr_wrapper.active() ) )
    //     {
    //         // Get the next user action only if not shutting down.
    //         if ( !is_shutting_down ) {
    //             app_commands_get_next();
    //         }
    //     }
    //
    //     std::cout << "-- shutdowned --" << std::endl;
    // }
    // catch( deezer_wrapper_exception& e )
    // {
    //     std::cerr << "-- runtime error : " << e.what() << " --" << std::endl;
    // }
    // catch(...)
    // {
    //     std::cerr << "-- unknown runtime error --" << std::endl;
    // }
}

#include "main.moc"
