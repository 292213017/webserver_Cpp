server:main.cpp config.cpp webserver.cpp ./http/http_conn.cpp ./log/log.cpp
	g++ $^ -o $@ -lpthread

clean:
	rm -r server