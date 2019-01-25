#include<iostream>
#include<vector>
#include<boost/asio.hpp>
#include <boost/thread.hpp>
#include "Connection.cpp"

boost::asio::io_service io;
Connection::pointer client;

bool ping_toggle = true;

// clients that a client will connect to.
std::string peer_list = "";

class Client: public Connection{
    friend class Connection;    
    Client(boost::asio::io_service &io): Connection(io){}

    public:
    void on_connect(bool success){
        if(success){
            std::string msg="HELLO";
            send(boost::asio::buffer(msg));
        }
    }

    void on_recieve(const char *data, size_t recvsize){
        std::string msg = std::string(data,recvsize);
        std::cout << msg << std::endl;
        if (msg.find("ALL_AVAILABLE_NODES") == 0){
            peer_list = msg;
        }
    }

    void on_disconnect(){
        std::cerr << "Connection closed." << std::endl;
    }
};

void main_thread() {
    std::string msg;
    int option;

    while(true) {

        if (io.stopped()) io.reset();
        io.poll();   
        
        std::cout   << "Enter the request number:\n"
                    << "(0) TOGGLE_PING (ON/OFF)\n"
                    << "(1) ALL_AVAILABLE_NODES\n"
                    << "(2) AVAILABLE_FOR_ONE_HOUR\n"
                    << "(3) AVAILABLE_FOR_TWO_HOUR\n"
                    << "(4) AVAILABLE_FOR_ONE_DAY\n" 
                    << "(5) SHOW_YOUR_PEER_LIST"
                    << std::endl;

        std::cout << "> ";
        std::cin >> option;

        switch(option) {
            case 0: // TOGGLE_PING
                ping_toggle = ping_toggle ? 0:1;
                break;
            case 1:
                msg = "ALL_AVAILABLE_NODES";
                client->send(boost::asio::buffer(msg));
                break;
            case 2:
                msg = "AVAILABLE_FOR_ONE_HOUR";
                client->send(boost::asio::buffer(msg));
                break;
            case 3:
                msg = "AVAILABLE_FOR_TWO_HOUR";
                client->send(boost::asio::buffer(msg));
                break;
            case 4:
                msg = "AVAILABLE_FOR_ONE_DAY";
                client->send(boost::asio::buffer(msg));
                break;
            case 5: // SHOW_PEER_LIST
                std::cout << "Your peers:\n"
                          << peer_list
                          << std::endl;
                break;
            default:
                std::cout << "Invalid request sent." << std::endl;
                break;
        }
    }
}

void ping_thread() {
  while(true) {
    if(ping_toggle) {
        if (io.stopped()) io.reset();
        io.poll();   
        boost::this_thread::sleep(boost::posix_time::millisec(2000));
        client->send(boost::asio::buffer("PING"));
    }
  }
}

int main() {
    client = Connection::create<Client>(io);
    client->open("127.0.0.1","8333");
    io.run_one();

    boost::thread_group g;
    g.create_thread(main_thread);
    g.create_thread(ping_thread);
    g.join_all();
}