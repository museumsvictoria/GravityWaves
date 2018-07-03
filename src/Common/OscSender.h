//
//  OscSender.h
//  GravityWaves
//
//  Created by Andrew Wright on 16/4/18.
//

#ifndef OscSender_h
#define OscSender_h

#include <Common/Osc.h>
#include <Common/Types.h>

using OscSenderRef = std::unique_ptr<class OscSender>;
class OscSender
{
public:
    
    OscSender                               ( u32 port, const std::string& address = "0.0.0.0" );
    
    void                                    Send        ( const std::string& message );
    void                                    Send        ( const std::string& message, float value );
    void                                    Send        ( const std::vector<std::pair<std::string, float>>& messages );
    void                                    SendPacked  ( const std::string& prefix, const std::vector<std::pair<std::string, float>>& messages );
    
    ~OscSender                              ( );
    
protected:
    
    void                                    InternalSend ( const ci::osc::Message& message );
    
    std::thread                             _thread;
    std::unique_ptr<asio::io_service>       _ioService;
    std::unique_ptr<asio::io_service::work> _work;
    ci::osc::SenderUdp                      _sender;
    std::mutex                              _sendLock;
};

#endif /* OscSender_h */
