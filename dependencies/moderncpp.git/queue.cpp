/*
    NOTE:
        there seems to be a minimum size limit, < 4 bytes puts some junk in message

    g++ queue.cpp  -lczmq -DUNITTEST -lzmq
*/
#include <iostream>
#include <string.h>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string>
#include <vector>
#include "queue.h"
#include <nlohmann/json.hpp>

IPCServer::IPCServer( std::string a_listen, ServerCommand* reciever ):
    m_urn(a_listen),
    m_run(true) ,
    m_reciever(reciever)
{
    m_ctx =  {};
}

IPCServer::~IPCServer() {
    close();
}

int IPCServer::bind() {
    printf("Bind %s\n", m_urn.c_str() );
    m_link =  { m_ctx, zmq::socket_type::rep};
    m_link.bind( m_urn);
    return 0;
}

int IPCServer::run() {
    printf("startup\n");
    bind();
    while (m_run) {
        std::string msg = recieve();
        std::string resp="";
        if( m_reciever->rxMessage( (char*)msg.c_str(), resp ) ) {
            send( resp );
        } else {
            send( "{'status':'NOK'}");
        }
    }
    printf("service stopped\n");
    return 0;
};

std::string IPCServer::recieve() {
    zmq::message_t req{};
    zmq::recv_result_t stat=m_link.recv(req, zmq::recv_flags::none );
    std::string requestStr( (char *) req.data(), req.size()) ;
    return requestStr;
}

bool IPCServer::send( std::string msg) {
    zmq::message_t buffer{ 
        msg.cbegin(),
        msg.cend()     
    };
   zmq::send_result_t status = m_link.send( buffer,  zmq::send_flags::none);
   return true;
}

bool IPCServer::close() {
    m_link.close();
    return true;
}


/*
 *
 *      Client side
 *
 * */

IPCClient::IPCClient( std::string a_urn ):
    m_urn(a_urn) 
{
    m_ctx =  {};
    m_link =  { m_ctx, zmq::socket_type::req};
}

IPCClient::~IPCClient() {
    close();
}

bool IPCClient::connect() {
    m_link.connect( m_urn);
    return true;
}

int IPCClient::send( std::string msg ) {
    //    printf("TX: %s\n", msg.c_str() );
    zmq::message_t buffer { 
        msg.cbegin(),
            msg.cend()
    };
    try {
        zmq::send_result_t status = m_link.send( buffer,zmq::send_flags::none );
    }catch( std::exception e ) {
        printf("Failed: %s\n", e.what() );
        return -1;
    }
    return 0;
}

bool IPCClient::recieve(std::string& r_msg) {
    zmq::pollitem_t items [] = {
        { m_link, 0, ZMQ_POLLIN, 0 }
    };
#if 0 
    zmq::message_t buffer;
    zmq::recv_result_t stat=m_link.recv( buffer, zmq::recv_flags::none );
    r_msg = std::string( (char*)buffer.data(), buffer.size() );
#endif
    long timeout = 1000;
    zmq::recv_result_t  rc = 0;
    zmq::message_t msg;
    zmq::poll (&items [0], sizeof(items)/sizeof(zmq::pollitem_t), timeout);
    if (items [0].revents & ZMQ_POLLIN) {
        rc = m_link.recv( msg,   zmq::recv_flags::none ) ;
        if( rc.has_value() && (EAGAIN != rc.value()) ) {
            r_msg = std::string( (char*)msg.data(), msg.size() );
            return true;
        }
    } 
    printf("MQ: No data in %ld\n", timeout );
    return false;
}

bool IPCClient::recieve(std::string& r_msg, long timeoutMs) {
    zmq::pollitem_t items [] = {
        { m_link, 0, ZMQ_POLLIN, 0 }
    };
#if 0 
    zmq::message_t buffer;
    zmq::recv_result_t stat=m_link.recv( buffer, zmq::recv_flags::none );
    r_msg = std::string( (char*)buffer.data(), buffer.size() );
#endif
    long timeout = timeoutMs;
    zmq::recv_result_t  rc = 0;
    zmq::message_t msg;
    zmq::poll (&items [0], sizeof(items)/sizeof(zmq::pollitem_t), timeout);
    if (items [0].revents & ZMQ_POLLIN) {
        rc = m_link.recv( msg,   zmq::recv_flags::none ) ;
        if( rc.has_value() && (EAGAIN != rc.value()) ) {
            r_msg = std::string( (char*)msg.data(), msg.size() );
            return true;
        }
    } 
    printf("MQ: No data in %ld\n", timeout );
    return false;
}

bool IPCClient::close() {
    m_link.close();
    return true;
}



using json = nlohmann::json;
class MyReciver: public ServerCommand
{
    public:
        bool rxMessage( std::string msg, std::string& r_resp ) {
            json j;
            j["status"] = "Yeah";
            r_resp = j.dump();
            return true;
        }
};


IPCBroadcaster::IPCBroadcaster( std::string a_listen ):
    m_urn(a_listen),
    m_run(true) 
{
    m_ctx =  {};
}

IPCBroadcaster::~IPCBroadcaster() {
    close();
}

int IPCBroadcaster::bind() {
    printf("Bind %s\n", m_urn.c_str() );
    m_link =  { m_ctx, zmq::socket_type::pub };
    m_link.bind( m_urn);
    return 0;
}

int IPCBroadcaster::run() {
    printf("startup\n");
    bind();
    return 0;
};


bool IPCBroadcaster::send( std::string a_topic, std::string msg) {
    zmq::message_t buffer{ 
        msg.cbegin(),
        msg.cend()     
    };
    zmq::message_t topic{ 
        a_topic.cbegin(),
        a_topic.cend()     
    };

    memcpy(topic.data(), a_topic.data(), a_topic.size()); 
    try {
        m_link.send( topic,   zmq::send_flags::sndmore);
        m_link.send( buffer,  zmq::send_flags::none);
    } catch(zmq::error_t &e) {
        std::cout << e.what() << std::endl;
    }
    return true;
}

bool IPCBroadcaster::close() {
    m_link.close();
    return true;
}


IPCReciever::IPCReciever( std::string a_listen ):
    m_urn(a_listen)
{
    m_ctx =  {};
    m_link =  { m_ctx, zmq::socket_type::sub};

}

IPCReciever::~IPCReciever() {
    close();
}


bool IPCReciever::recieve( std::string a_topic ) {
    m_topic = a_topic;
    printf("subscribe %s\n",a_topic.c_str() );
    m_link.setsockopt(ZMQ_SUBSCRIBE, a_topic.c_str(), a_topic.length()); 
    return true;
}

bool IPCReciever::close() {
    m_link.close();
    return true;
}

int IPCReciever::run()
{
    zmq::pollitem_t items [] = {
        { m_link, 0, ZMQ_POLLIN, 0 }
 //     , { killer_socket, 0, ZMQ_POLLIN, 0 }
    };
    m_link.connect( m_urn);
    printf("reciver start\n");
    while(true)
    {
        zmq::recv_result_t  rc = 0;
        zmq::message_t msg,rxtopic;
        zmq::poll (&items [0], sizeof(items)/sizeof(zmq::pollitem_t), -1);
        if (items [0].revents & ZMQ_POLLIN) {
            rc = m_link.recv( rxtopic,   zmq::recv_flags::none ) ;
            rc = m_link.recv( msg,   zmq::recv_flags::none ) ;
            if(rc > 0) {
				onMessage(m_topic, std::string(static_cast<char*>(msg.data()), msg.size()) );
            }
        } /*else if (items [1].revents & ZMQ_POLLIN) {
            if(s_interrupted == 1) {
                std::cout << "break" << std::endl;
                break;
            }
        }*/
    }
}

void IPCReciever::onMessage( std::string topic, std::string data )
{
		std::cout << topic << "= "<< data << std::endl;
}




#ifdef UNITTEST
//g++ queue.cpp  -lzmq
int main (int argc,char*argv[])
{
    if(argc < 2 ) {
        printf("Missing args\n");
        exit(0);
    }
    if( argv[1][0] == 's' ) {
        ServerCommand* rx=new MyReciver();
        IPCServer srv("tcp://*:5555", rx);
        srv.run();
    } else if( argv[1][0] == 'b' ) {
        IPCBroadcaster srv("tcp://*:6666" );
        srv.run();
        while(true) {
            printf("Broadcast message\n");
            srv.send("/test", "message!");
            sleep(1);
        }
    } else if( argv[1][0] == 'r' ) {
        IPCReciever rx("tcp://localhost:6666");
        if( argc == 3 ) {
            printf("Topic %s provided\n", argv[2] );
            rx.recieve(argv[2] );
        }else {
            rx.recieve("/test");
        }
        rx.run();
    } else {
        IPCClient cli("tcp://localhost:5555");
        if( cli.connect() ) {
            json j;
            j["command"] = argv[2];
            cli.send( j.dump() );
            {
                std::string msg;
                cli.recieve(msg);
                json resp;
                resp = json::parse(msg);
                try {
#if 0
                    for (auto it = resp.begin(); it != resp.end(); ++it) {
                        if( it.key() == "status") {
                            std::cout << "key: " << it.key() << ", value:" << it.value() << '\n';
                        }
                    }
#endif
                    std::string v =resp.find("status").value();
                    printf("=> %s\n", v.c_str() );
                } catch( ...) {
                    printf("Got no status code\n");
                    printf("Resp:%s | %s\n", resp.dump().c_str(), msg.c_str() );
                }
            }
        } else {
            printf("No conn\n");
        }
    }
    return 0;
}
#endif
