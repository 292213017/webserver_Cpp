server:main.cpp config.cpp webserver.cpp ./http/http_conn.cpp ./log/log.cpp ./CGImysql/sql_connection_pool.cpp ./timer/lst_timer.cpp
	g++ $^ -o $@ -lpthread -lmysqlclient

clean:
	rm -r server