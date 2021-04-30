OUTPUT=server
PORT=7000
MAX_CORES=2
LIB=./wrapper/record.c

$(OUTPUT):
	gcc -Wall $(OUTPUT).c $(LIB) -o $(OUTPUT) -fopenmp
all: clean $(OUTPUT) run

run:
	ulimit -n 120000
	./$(OUTPUT) --port $(PORT) --max-core $(MAX_CORES)

clean:
	rm -f $(OUTPUT)

watch:
	while true; do \
		netstat -anp --ip ; \
		sleep 1 & \
		wait $$! ; \
	done ; \