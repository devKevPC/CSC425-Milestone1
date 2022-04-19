all : cproxy sproxy

cproxy :
        gcc -Wall cproxy.c   -o cproxy

sproxy :
         gcc -Wall sproxy.c  -o sproxy
