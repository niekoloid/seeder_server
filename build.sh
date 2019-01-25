
rm -fr server client
g++ Server.cpp -lboost_system -lboost_thread -lpthread -lboost_date_time -std=c++14 -o server
g++ Client.cpp -lboost_system -lboost_thread -lpthread -std=c++14 -o client
