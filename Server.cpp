#include<iostream>
#include<boost/asio.hpp>
#include<boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "Connection.cpp"

boost::asio::io_service io;

struct node {
    std::string ep;
    boost::posix_time::ptime t_added;
    boost::posix_time::ptime t_ping;
};

std::vector<node> nodes;

template <typename T>
class Server{
    boost::asio::ip::tcp::acceptor acceptor_;

    public:
    Server(boost::asio::io_service &io_service, int port): acceptor_(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)){
        start_accept();
    }

    void accept_handle(Connection::pointer tcp, const boost::system::error_code& ec){
        if(ec) return;
        tcp->on_connect(true);
        tcp->start_recieve();
        start_accept();
    }

    void start_accept(){
        Connection::pointer tcp = Connection::create<T> (acceptor_.get_io_service());
        acceptor_.async_accept(tcp->get_socket(), boost::bind(&Server::accept_handle, this, tcp, _1));
    }

    void close(){
        acceptor_.close();
    }
};

class Seeder: public Connection{
    friend class Connection;
    Seeder(boost::asio::io_service& io):Connection(io){}

    public:
    static pointer create(boost::asio::io_service& io){
        return pointer(new Seeder(io));
    }
    
    void on_connect(bool success){
        if(success){
            std::string ep = get_endpoint();
            std::cout << ep << std::endl;

            boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
            node n = {ep, now, now};
            nodes.push_back(n);

            send(boost::asio::buffer("HELLO ACK from Server:" + ep));
        }
    }    

    void on_recieve(const char *data, size_t recvsize){
        std::string req = std::string(data, recvsize);
        std::string ep = get_endpoint();
        std::cout << req << " (" << ep << ") " << std::endl;

        if (req.find("PING") == 0) {
            for(int i = 0; i < nodes.size(); i++){
                 if (nodes[i].ep == ep) {
                     nodes[i].t_ping = boost::posix_time::microsec_clock::local_time();
                 }
            }
        }else if (req.find("ALL_AVAILABLE_NODES") == 0) {
            list_nodes(boost::posix_time::hours(0).total_milliseconds(), true);
        }else if (req.find("AVAILABLE_FOR_ONE_HOUR") == 0) {
            list_nodes(boost::posix_time::hours(1).total_milliseconds(), false);;
        }else if (req.find("AVAILABLE_FOR_TWO_HOUR") == 0) {
            list_nodes(boost::posix_time::hours(2).total_milliseconds(), false);
        }else if (req.find("AVAILABLE_FOR_ONE_DAY") == 0) {
            list_nodes(boost::posix_time::hours(24).total_milliseconds(), false);
        }
    }

    void list_nodes(long long t_duration, bool all_flag){
        std::string nodes_;
        if(all_flag) nodes_ = "ALL_AVAILABLE_NODES\n";
        for(int i = 0; i < nodes.size(); i++){
            long long t_diff = (nodes[i].t_ping - nodes[i].t_added).total_milliseconds();
            if(t_diff >= t_duration){
                nodes_ += nodes[i].ep
                    + " Added at " 
                    + boost::posix_time::to_simple_string(nodes[i].t_added)
                    + " Last Ping at "
                    + boost::posix_time::to_simple_string(nodes[i].t_ping);
                    + "\n\n";
            }
        }
        std::cout << nodes_ << std::endl;
        send(boost::asio::buffer(nodes_));
    }

    void on_disconnect(){
        std::string closed_ep = get_endpoint();
        std::cout << closed_ep << std::endl;
        for(int i = 0; i < nodes.size(); i++){
            if(nodes[i].ep == closed_ep){
                std::cout << nodes[i].ep + " has been removed because the connection is closed." << std::endl;
                nodes.erase(nodes.begin() + i);
            }
        }
    }

    std::string get_endpoint() {
        boost::asio::ip::tcp::socket& socket = get_socket();
        return (socket.remote_endpoint().address().to_string() + ":" + std::to_string(socket.remote_endpoint().port()));
    }
};

void main_thread() {
    while(true) {
        if (io.stopped()) io.reset();
        io.poll();
    }
}

void monitor_thread() {
    while(true) {
        boost::this_thread::sleep(boost::posix_time::millisec(5000));
        boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
        for(int i = 0; i < nodes.size(); i++){
            long long t_diff = (now - nodes[i].t_ping).total_milliseconds();
            if(t_diff > 10000){
                std::cout << nodes[i].ep + " has been removed because of timeout." << std::endl;
                nodes.erase(nodes.begin() + i);
            }
        }
  }
}

int main(){
    Server<Seeder> Seeder(io, 8333);
    std::cout << "Seeder server is running." << std::endl;

    boost::thread_group g;
    g.create_thread(main_thread);
    g.create_thread(monitor_thread);
    g.join_all();
}