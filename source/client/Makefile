# Name
OUTPUT 				= client# executable
RESULT_FILE 		= result.txt# test result output file
ERROR_FILE			= error.txt# error output file

# Run param
HOST				= 192.168.0.44# host address
PORT				= 7000# server port 
SEND_COUNT			= 100# number of send per client
MSG 				= "Hello, World!"# sending message
DELAY				= 10000# max 999999999

# Testing param
N_CLIENT_TICK		= 10 
N_TICK_FIXED		= 10#test-a loop count 
SPAWN_DELAY			= 0

$(OUTPUT):
	gcc -Wall $(OUTPUT).c -o $(OUTPUT) -fopenmp

all: clean $(OUTPUT) run

run:
	./$(OUTPUT) --host $(HOST) --port $(PORT) --message $(MSG) --count $(SEND_COUNT) \
	--delay $(DELAY) >> $(RESULT_FILE)

clean:
	rm -f $(OUTPUT)

test-a:
	ulimit -n 120000
	A=;
	i=0
	echo "" > $(RESULT_FILE) ; \
	echo "" > $(ERROR_FILE) ; \
	for number in `seq 1 $(N_TICK_FIXED)` ; do \
		for number in `seq 1 $(N_CLIENT_TICK)` ; do \
			./$(OUTPUT) --host $(HOST) --port $(PORT) --message $(MSG) --count $(SEND_COUNT) \
			--delay $(DELAY) 1>> $(RESULT_FILE) 2>> $(ERROR_FILE) & \
			sleep $(SPAWN_DELAY) ; \
			A="$$A$$!:"; \
		done ; \
		((i+=$(N_CLIENT_TICK))); \
		echo $$i client; \
	done ; \
	while [ -n "$${A}" ]; do \
		I="$${A%%:*}"; \
		wait $${I}; \
		A="$${A#*:}"; \
	done

test-b:
	ulimit -n 120000
	i=0
	echo "" > $(RESULT_FILE) ; \
	while true; do \
		for number in `seq 1 $(N_CLIENT_TICK)` ; do \
			./$(OUTPUT) --host $(HOST) --port $(PORT) --message $(MSG) --count $(SEND_COUNT) \
			--delay $(DELAY) >> $(RESULT_FILE) & \
			sleep $(SPAWN_DELAY) ; \
		done ; \
		((i+=$(N_CLIENT_TICK))); \
		echo $$i client; \
	done ; \

test-clean: # Kill zombie process
	kill $$(pgrep $(OUTPUT))

watch:
	while true; do \
		netstat -anp --ip ; \
		sleep 1 & \
		wait $$i ; \
	done ; \

# Notes
# 1. if you send to background a process you do not need to use ; as the shell 
# is already ready to accept new command
#
# 2. Run this command to kill zombie client
# kill $(pgrep client)
# use top to view zombie process

# Old version testing, create fix number of client instantaneously
# test:
# 	A=;
# 	echo "" > $(RESULT_FILE)
# 	for number in `seq 1 $(N_CLIENT)` ; do \
#     	./$(OUTPUT) --host $(HOST) --port $(PORT) --message $(MSG) --count $(SEND_COUNT) \
# 		--delay $(DELAY) >> $(RESULT_FILE) & \
# 		A="$$A$$!:"; \
# 	done ; \
# 	while [ -n "$${A}" ]; do \
# 		I="$${A%%:*}"; \
# 		wait $${I}; \
# 		A="$${A#*:}"; \
# 	done