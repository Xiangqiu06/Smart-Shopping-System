OBJ:=a.out

OBJS+=main.o
OBJS+=webserver.o
OBJS+=sqlite3.o
OBJS+=operation.o

LDFLAGS:=-lpthread -ldl

$(OBJ):$(OBJS)
	gcc $^ -o $@ $(LDFLAGS)
%.o:%.c 
	gcc -c $^ -o $@

.PHONY:
clean:
	rm $(OBJ)