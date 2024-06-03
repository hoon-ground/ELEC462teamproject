###Makefile for mafia_serv & mafia_clnt
###written by kyw_2019114270

all: mafia_serv mafia_clnt

mafia_serv: mafia_serv.c
	gcc -o mafia_serv mafia_serv.c -lpthread

mafia_clnt: mafia_clnt.c
	gcc -o mafia_clnt mafia_clnt.c -lpthread
