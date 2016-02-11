GCC_OPTS  = -std=gnu11
GCC_OPTS += -Wall -Wextra -Werror
GCC_OPTS += -fdiagnostics-show-option 
GCC_OPTS += -Wcast-qual -Wconversion -Wno-sign-conversion -Wfloat-equal -Wno-missing-field-initializers

GCC_OPTS += -O2

all:
	gcc $(GCC_OPTS) src/stripzip_app.c -o stripzip

clean:
	rm *o stripzip

