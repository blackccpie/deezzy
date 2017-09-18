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

#define DEEZZY_APPLICATION_ID      "247082"	// SET YOUR APPLICATION ID
#define DEEZZY_APPLICATION_NAME    "Deezzy" // SET YOUR APPLICATION NAME
#define DEEZZY_APPLICATION_VERSION "00001"	// SET YOUR APPLICATION VERSION

class TrackInfos : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString artist READ artist NOTIFY artistChanged)
    Q_PROPERTY(int duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(QString albumTitle READ albumTitle NOTIFY albumTitleChanged)
    Q_PROPERTY(QString coverArtUrl READ coverArtUrl NOTIFY coverArtUrlChanged)
public:
    TrackInfos( QObject* parent ) : QObject( parent ) {}
    QString title() { return m_title; }
    QString artist() { return m_artist; }
    int duration() { return m_duration; }
    QString albumTitle() { return m_albumTitle; }
    QString coverArtUrl() { return m_coverArtUrl; }
signals:
	void titleChanged();
    void artistChanged();
    void durationChanged();
	void albumTitleChanged();
	void coverArtUrlChanged();
public:
    QString m_title;
    QString m_artist;
    int m_duration;
    QString m_albumTitle;
    QString m_coverArtUrl;
};

class DeezzyApp :   public QObject,
                    public deezer_wrapper::observer
{
    Q_OBJECT
    Q_ENUMS(PlaybackState)
    Q_PROPERTY(PlaybackState playbackState READ playbackState)
    Q_PROPERTY(QString content READ content WRITE setContent)
    Q_PROPERTY(TrackInfos* trackInfos READ trackInfos NOTIFY trackInfosChanged)
public:
    enum class PlaybackState
    {
        Stopped,
        Playing,
        Paused
    };
public:
    DeezzyApp() :   m_current_track_infos( new TrackInfos( this ) ),
                    m_deezer_wrapper( std::make_shared<deezer_wrapper>( DEEZZY_APPLICATION_ID,
                                                                        DEEZZY_APPLICATION_NAME,
                                                                        DEEZZY_APPLICATION_VERSION,
                                                                        true /*print_version*/ ) )
    {
    }

    void setPlaylist( QString playlist )
    {
        emit playlistChanged( playlist );
    }

    /************ Q_INVOKABLEs ************/

    Q_INVOKABLE QString defaultPlaylist()
    {
        return "dzradio:///user-" + QString::fromStdString( m_deezer_wrapper->user_id() );
    }
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
        m_deezer_wrapper->playback_resume();
        m_playback_state = PlaybackState::Playing;

        return true;
    }
    Q_INVOKABLE bool seek( int progress )
    {
        if ( m_current_track_infos )
        {
            auto position_ms = 10 * progress * m_current_track_infos->m_duration;
            m_deezer_wrapper->playback_seek( position_ms );
        }

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
    Q_INVOKABLE bool like()
    {
        m_deezer_wrapper->playback_like();

        return true;
    }
    Q_INVOKABLE bool dislike()
    {
        m_deezer_wrapper->playback_dislike();

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

    TrackInfos* trackInfos() const
    {
        return m_current_track_infos;
    }

signals:
    void paused();
    void playing();
    void stopped();
    void loggedIn();
    void error();
    void seeking();
    void bufferProgress( float progress );
    void renderProgress( float progress );
    void trackDuration( int duration_ms );
    void playlistChanged( QString playlist );

	void trackInfosChanged();

private:
    void update_current_track_infos()
    {
        auto& _track_infos = m_deezer_wrapper->current_track_infos();
        m_current_track_infos->m_title = QString::fromStdString( _track_infos.title );
        m_current_track_infos->m_artist = QString::fromStdString( _track_infos.artist );
        m_current_track_infos->m_duration = _track_infos.duration;
        m_current_track_infos->m_albumTitle = QString::fromStdString( _track_infos.album_title );
        m_current_track_infos->m_coverArtUrl = QString::fromStdString( _track_infos.cover_art );
    }
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
                emit loggedIn();
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
                update_current_track_infos();
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
                emit stopped();
                break;
            case deezer_wrapper::player_event::render_track_paused:
                emit paused();
                break;
            case deezer_wrapper::player_event::render_track_seeking:
                emit seeking();
                break;
            case deezer_wrapper::player_event::render_track_underflow:
                break;
            case deezer_wrapper::player_event::render_track_resumed:
                emit playing();
                break;
            case deezer_wrapper::player_event::render_track_removed:
                emit stopped();
                break;
            default:
                break;
        }
    }
    void on_index_progress( int progress_ms ) final override
    {
        if ( m_current_track_infos )
        {
            auto p = progress_ms / ( 10.f * m_current_track_infos->m_duration );
            bufferProgress( std::min( p, 100.f ) );
        }
    }
    void on_render_progress( int progress_ms ) final override
    {
        if ( m_current_track_infos )
        {
            auto p = progress_ms / ( 10.f * m_current_track_infos->m_duration );
            renderProgress( std::min( p, 100.f ) );
        }
    }
    void on_track_duration( int duration_ms ) final override
    {
        trackDuration( duration_ms );
    }
private:

    TrackInfos* m_current_track_infos = nullptr;
    PlaybackState m_playback_state = PlaybackState::Stopped;

    std::shared_ptr<deezer_wrapper> m_deezer_wrapper;
};
