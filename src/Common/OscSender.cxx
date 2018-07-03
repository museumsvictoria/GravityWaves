//
//  OscSender.cxx
//  GravityWaves
//
//  Created by Andrew Wright on 16/4/18.
//

#include <Common/OscSender.h>

using namespace ci;

OscSender::OscSender ( u32 port, const std::string& host )
: _ioService ( std::make_unique<asio::io_service>() )
, _work ( std::make_unique<asio::io_service::work>( *_ioService ) )
, _sender ( 10000, host, port, asio::ip::udp::v4(), *_ioService )
{
    _sender.bind ( );
    _thread = std::thread( std::bind([]( asio::io_service * service )
    {
        service->run();
    }, _ioService.get() ));
}

void OscSender::Send ( const std::string& message )
{
    InternalSend ( osc::Message{ message } );
}

void OscSender::Send ( const std::string& message, float value )
{
    osc::Message msg{ message };
    msg.append(value);

    InternalSend(msg);
}

void OscSender::Send ( const std::vector<std::pair<std::string, float>>& messages )
{
    std::lock_guard<std::mutex> lock{ _sendLock };
    for (auto& p : messages)
    {
        osc::Message msg { p.first };
        msg.append ( p.second );
        _sender.send ( msg );
    }
}

void OscSender::SendPacked ( const std::string& prefix, const std::vector<std::pair<std::string, float>>& messages )
{
    std::lock_guard<std::mutex> lock{ _sendLock };
    osc::Message msg { prefix + "blackholes" };

    for ( auto& m : messages ) msg.append ( m.second );

    _sender.send ( msg );
}

void OscSender::InternalSend ( const ci::osc::Message& message )
{
    std::lock_guard<std::mutex> lock { _sendLock };
    _sender.send ( message );
}

OscSender::~OscSender ( )
{
    {
        std::lock_guard<std::mutex> lock { _sendLock };
        _sender.close();
    }
    
    _work.reset();
    _ioService->stop();
    if ( _thread.joinable() ) _thread.join();

}
