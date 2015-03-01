LDFLAGS=-pthread -ldl -lphidget21 -lm

all:
	$(CC) src/homeautomation.c src/mongoose.c $(LDFLAGS) -o homeautomation
