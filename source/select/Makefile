OUTPUT=server
PORT=7000
MAX_SOCKET_SIZE=1024
LIB=./wrapper/record.c
$(OUTPUT):
	gcc -Wall $(OUTPUT).c $(LIB) -o $(OUTPUT)
all: clean $(OUTPUT) run

run:
	ulimit -n 120000
	./$(OUTPUT) --port $(PORT)

clean:
	rm -f $(OUTPUT)

watch:
	while true; do \
		netstat -anp --ip ; \
		sleep 0.5 & \
		wait $$! ; \
	done ; \