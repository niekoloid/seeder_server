#include<iostream>
#include<boost/asio.hpp>
#include<boost/enable_shared_from_this.hpp>
#include<boost/bind.hpp>

class Connection: public boost::enable_shared_from_this<Connection>{
    boost::asio::ip::tcp::socket socket;
    boost::asio::ip::tcp::resolver resolver;
    std::vector<char> recvbuf;
    std::vector<char> sendbuf;

    public:
    Connection(boost::asio::io_service& io):socket(io), resolver(io), recvbuf(1024), sendbuf(){}

    boost::asio::ip::tcp::socket& get_socket(){
        return socket;
    }    

    typedef boost::shared_ptr<Connection> pointer;
    
    template<typename T>
    static pointer create(boost::asio::io_service &io){
        return pointer(new T(io));
    }
    
    virtual void on_connect(bool success){}
    virtual void on_recieve(const char *data, size_t recvsize){}
    virtual void on_disconnect(){}
    
    void resolve_handle(boost::system::error_code ec, boost::asio::ip::tcp::resolver::iterator it){
        if(ec){
            on_connect(false);
            return;
        }
        socket.async_connect(*it, boost::bind(&Connection::connect_handle, shared_from_this(), _1, it));
    }

    void connect_handle(boost::system::error_code ec, boost::asio::ip::tcp::resolver::iterator it){
        if(ec){
            boost::asio::ip::tcp::resolver::iterator end;
            if(++it != end){
                socket.async_connect(*it, boost::bind(&Connection::connect_handle, shared_from_this(), _1, it));
            }else{
                // Connection failure.
                on_connect(false);
            }
        }else{
            // Connection success!
            on_connect(true);
            start_recieve();
        }
    }
    
    void receive_handle(boost::system::error_code ec, size_t recvsize){
        if(ec && ec != boost::asio::error::message_size){
            on_disconnect();
            socket.close();
            return;
        }
        on_recieve(recvbuf.data(), recvsize);
        start_recieve();
    }

    void start_recieve(){
        socket.async_receive(boost::asio::buffer(recvbuf), boost::bind(&Connection::receive_handle, shared_from_this(), _1, _2));
    }

    void send_handle(boost::system::error_code ec, size_t sendsize){
        if(ec) return;

        sendbuf.erase(sendbuf.begin(), sendbuf.begin()+sendsize);
        if(!sendbuf.empty()) start_send();
    }
    
    template<typename T>
    void send(const T &buf){
        if(!socket.is_open()) return;
        const char *c = static_cast<const char*>(boost::asio::detail::buffer_cast_helper(buf));
        size_t size = boost::asio::detail::buffer_size_helper(buf);
        bool empty = sendbuf.empty();
        sendbuf.insert(sendbuf.end(), c, c+size);
        if(empty) start_send();
    }

    void start_send(){
        socket.async_send(boost::asio::buffer(sendbuf), boost::bind(&Connection::send_handle, shared_from_this(), _1, _2));
    }

    void open(const std::string &host, const std::string &port){
        boost::asio::ip::tcp::resolver::query q(host, port);
        resolver.async_resolve(q, boost::bind(&Connection::resolve_handle, shared_from_this(), _1, _2));
    }
    
};
