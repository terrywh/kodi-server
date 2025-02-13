.PHONY: kodi_server_go kodi_server_cpp kodi_server_js

kodi_server_go: go/*.go
# 	go run go/*.go
	go build -o kodi_server_go go/*.go
#	./kodi_server_go

kodi_server_cpp: cpp/*.cpp
	g++ -std=c++20 -g -O0 -DBOOST_ASIO_HAS_IO_URING cpp/main.cpp cpp/content_type.cpp -I/data/vendor/boost/include -L/data/vendor/boost/lib -lboost_log -lboost_date_time -lboost_url -lboost_filesystem -lboost_thread -lboost_system -luring -pthread -o kodi_server_cpp
#   ./kodi_server_cpp

kodi_server_js:
	bun run js/main.js


