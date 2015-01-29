
/*
 *  MovieImpl.cpp
 *  sfeMovie project
 *
 *  Copyright (C) 2010-2014 Lucas Soltic
 *  lucas.soltic@orange.fr
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include "MovieImpl.hpp"
#include "Demuxer.hpp"
#include "Timer.hpp"
#include "Log.hpp"
#include "Utilities.hpp"
#include <cmath>
#include <iostream>

#define LAYOUT_DEBUGGER_ENABLED 0

namespace sfe
{
    MovieImpl::MovieImpl(sf::Transformable& movieView) :
    m_movieView(movieView),
    m_demuxer(nullptr),
    m_timer(nullptr),
    m_videoSprite()
    {
    }
    
    MovieImpl::~MovieImpl()
    {
    }
    
    bool MovieImpl::openFromFile(const std::string& filename)
    {
        try
        {
            m_timer = std::make_shared<Timer>();
            m_demuxer = std::make_shared<Demuxer>(filename, m_timer, *this);
            m_videoStreamsDesc = m_demuxer->computeStreamDescriptors(Video);
            
            std::set< std::shared_ptr<Stream> > videoStreams = m_demuxer->getStreamsOfType(Video);
            
            m_demuxer->selectFirstVideoStream();
            
            if (!videoStreams.size())
            {
                sfeLogError("Movie::openFromFile() - No supported audio or video stream in this media");
                return false;
            }
            else
            {
                if (videoStreams.size())
                {
                    sf::Vector2f size = getSize();
                    m_displayFrame = sf::FloatRect(0, 0, size.x, size.y);
                }
                
                return true;
            }
        }
        catch (std::runtime_error& e)
        {
            sfeLogError(e.what());
            return false;
        }
    }
    
    const Streams& MovieImpl::getStreams(MediaType type) const
    {
        switch (type)
        {
            case Video: return m_videoStreamsDesc;
            default: CHECK(false, "Movie::getStreams() - Unknown stream type:" + mediaTypeToString(type));
        }
    }
    
    bool MovieImpl::selectStream(const StreamDescriptor& streamDescriptor)
    {
        if (!m_demuxer || !m_timer)
        {
            sfeLogError("Movie::selectStream() - cannot select a stream with no opened media");
            return false;
        }
        
        if (m_timer->getStatus() != Stopped)
        {
            sfeLogError("Movie::selectStream() - cannot select a stream while media is not stopped");
            return false;
        }
        
        std::map<int, std::shared_ptr<Stream> > streams = m_demuxer->getStreams();
        std::map<int, std::shared_ptr<Stream> >::iterator it = streams.find(streamDescriptor.identifier);
        std::shared_ptr<Stream>  streamToSelect = nullptr;
        
        if (it != streams.end())
        {
            streamToSelect = it->second;
        }
        
        switch (streamDescriptor.type)
        {
            case Video:
                m_demuxer->selectVideoStream(std::dynamic_pointer_cast<VideoStream>(streamToSelect));
                return true;
            default:
                sfeLogWarning("Movie::selectStream() - stream activation for stream of kind "
                              + mediaTypeToString(it->second->getStreamKind()) + " is not supported");
                return false;
        }
    }
    
    void MovieImpl::play()
    {
        if (m_demuxer && m_timer)
        {
            if (m_timer->getStatus() == Playing)
            {
                sfeLogError("Movie::play() - media is already playing");
                return;
            }
            
            m_timer->play();
            update();
        }
        else
        {
            sfeLogError("Movie::play() - No media loaded, cannot play");
        }
    }
    
    void MovieImpl::pause()
    {
        if (m_demuxer && m_timer)
        {
            if (m_timer->getStatus() == Paused)
            {
                sfeLogError("Movie::pause() - media is already paused");
                return;
            }
            
            m_timer->pause();
            update();
        }
        else
        {
            sfeLogError("Movie::pause() - No media loaded, cannot pause");
        }
    }
    
    void MovieImpl::stop()
    {
        if (m_demuxer && m_timer)
        {
            if (m_timer->getStatus() == Stopped)
            {
                sfeLogError("Movie::stop() - media is already stopped");
                return;
            }
            
            m_timer->stop();
            update();
        }
        else
        {
            sfeLogError("Movie::stop() - No media loaded, cannot stop");
        }
    }
    
    void MovieImpl::update()
    {
        if (m_demuxer && m_timer)
        {
            m_demuxer->update();
            
            if (getStatus() == Stopped && m_timer->getStatus() != Stopped)
            {
                m_timer->stop();
            }
            
            // Enable smoothing when the video is scaled
            std::shared_ptr<VideoStream> vStream = m_demuxer->getSelectedVideoStream();
            if (vStream)
            {
                sf::Vector2f movieScale = m_movieView.getScale();
                sf::Vector2f subviewScale = m_videoSprite.getScale();
                
                if (std::fabs(movieScale.x - 1.f) < 0.00001 &&
                    std::fabs(movieScale.y - 1.f) < 0.00001 &&
                    std::fabs(subviewScale.x - 1.f) < 0.00001 &&
                    std::fabs(subviewScale.y - 1.f) < 0.00001)
                {
                    vStream->getVideoTexture().setSmooth(false);
                }
                else
                {
                    vStream->getVideoTexture().setSmooth(true);
                }
            }
        }
        else
        {
            sfeLogWarning("Movie::update() - No media loaded, nothing to update");
        }
    }
    
    sf::Time MovieImpl::getDuration() const
    {
        if (m_demuxer && m_timer)
        {
            return m_demuxer->getDuration();
        }
        
        sfeLogError("Movie::getDuration() - No media loaded, cannot return a duration");
        return sf::Time::Zero;
    }
    
    sf::Vector2f MovieImpl::getSize() const
    {
        if (m_demuxer && m_timer)
        {
            std::shared_ptr<VideoStream> videoStream = m_demuxer->getSelectedVideoStream();
            
            if (videoStream)
            {
				return sf::Vector2f(videoStream->getFrameSize());
            }
        }
        sfeLogError("Movie::getSize() called but there is no active video stream");
        return sf::Vector2f(0, 0);
    }
    
    void MovieImpl::fit(float x, float y, float width, float height, bool preserveRatio)
    {
        fit(sf::FloatRect(x, y, width, height), preserveRatio);
    }
    
    void MovieImpl::fit(sf::FloatRect frame, bool preserveRatio)
    {
        sf::Vector2f movie_size = getSize();
        
        if (movie_size.x == 0 || movie_size.y == 0)
        {
            sfeLogError("Movie::fit() called but the video frame size is (0, 0)");
            return;
        }
        
        sf::Vector2f wanted_size = sf::Vector2f(frame.width, frame.height);
        sf::Vector2f new_size;
        
        if (preserveRatio)
        {
            sf::Vector2f target_size = movie_size;
            
            float source_ratio = movie_size.x / movie_size.y;
            float target_ratio = wanted_size.x / wanted_size.y;
            
            if (source_ratio > target_ratio)
            {
                target_size.x = movie_size.x * wanted_size.x / movie_size.x;
                target_size.y = movie_size.y * wanted_size.x / movie_size.x;
            }
            else
            {
                target_size.x = movie_size.x * wanted_size.y / movie_size.y;
                target_size.y = movie_size.y * wanted_size.y / movie_size.y;
            }
            
            new_size = target_size;
        }
        else
        {
            new_size = wanted_size;
        }
        
        m_videoSprite.setPosition((wanted_size.x - new_size.x) / 2.,
                                  (wanted_size.y - new_size.y) / 2.);
        m_movieView.setPosition(frame.left, frame.top);
        m_videoSprite.setScale((float)new_size.x / movie_size.x, (float)new_size.y / movie_size.y);
        m_displayFrame = frame;        
    }
    
    float MovieImpl::getFramerate() const
    {
        if (m_demuxer && m_timer)
        {
            std::shared_ptr<VideoStream> videoStream = m_demuxer->getSelectedVideoStream();
            
            if (videoStream)
                return videoStream->getFrameRate();
        }
        
        sfeLogError("Movie::getFramerate() - No selected video stream or no media loaded, cannot return a frame rate");
        return 0;
    }
    
    Status MovieImpl::getStatus() const
    {
        Status st = Stopped;
        
        if (m_demuxer)
        {
            std::shared_ptr<VideoStream> videoStream = m_demuxer->getSelectedVideoStream();
            Status vStatus = videoStream ? videoStream->getStatus() : Stopped;
            
            if (vStatus == Playing)
            {
                st = Playing;
            }
            else if (vStatus == Paused)
            {
                st = Paused;
            }
        }
        
        return st;
    }
    
    sf::Time MovieImpl::getPlayingOffset() const
    {
        if (m_demuxer && m_timer)
        {
            return m_timer->getOffset();
        }
        
        sfeLogError("Movie::getPlayingOffset() - No media loaded, cannot return a playing offset");
        return sf::Time::Zero;
    }
    
    const sf::Texture& MovieImpl::getCurrentImage() const
    {
        static sf::Texture emptyTexture;
        
        if (m_videoSprite.getTexture())
        {
            return * m_videoSprite.getTexture();
        }
        else
        {
            return emptyTexture;
        }
    }
    
    float MovieImpl::getVideoRotation() const
    {
        if (auto videoStream = m_demuxer->getSelectedVideoStream()) {
            return videoStream->getVideoRotation();
        }
        
        return 0.0f;
    }
    
    void MovieImpl::draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
        target.draw(m_videoSprite, states);
    }
    
    void MovieImpl::didUpdateVideo(const VideoStream& sender, const sf::Texture& image)
    {
        if (m_videoSprite.getTexture() != &image)
            m_videoSprite.setTexture(image);
    }
}
