#ifndef QUEUE_H
#define QUEUE_H
#include <string.h>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string>
#include <vector>

// server will call class that implements this
// upon recieve
// return true, to send r_resp as response, or false
// will return { status="NOK" }
class ServerCommand
{
    public:
        virtual bool rxMessage( std::string data, std::string& r_resp ) = 0;
};



class IPCServer
{
    public:
        IPCServer( std::string a_listen, ServerCommand* reciver);
        ~IPCServer();
        int bind();
        int run();
        std::string recieve();
        bool send( std::string msg);
        bool close();
    private:
        zmq::context_t m_ctx;
        zmq::socket_t m_link;
        std::string m_urn;
        bool m_run;
    private:
        ServerCommand* m_reciever;
        std::vector<char> m_rx;
};

class IPCBroadcaster
{
    public:
        IPCBroadcaster( std::string a_listen );
        ~IPCBroadcaster();
        int bind();
        int run();
        bool send( std::string topic, std::string msg);
        bool close();
    private:
        zmq::context_t m_ctx;
        zmq::socket_t m_link;
        std::string m_urn;
        bool m_run;
};

class IPCReciever
{
    public:
        IPCReciever( std::string a_listen );
        ~IPCReciever();
        bool recieve( std::string topic );
        bool close();
        int run();
		virtual void onMessage( std::string topic, std::string data );
    private:
        zmq::context_t m_ctx;
        zmq::socket_t m_link;
        std::string m_urn;
        std::string m_topic;
};



class IPCClient
{
    public:
        IPCClient( std::string a_urn );
        ~IPCClient();
        bool connect();
        int send( std::string msg );
        bool recieve(std::string& r_msg);
        bool recieve( std::string& , long timeoutMs );
        bool close();
    private:
        zmq::context_t m_ctx;
        zmq::socket_t m_link;
        std::string m_urn;
    private:
        std::vector<char> m_rx;
};




#endif
