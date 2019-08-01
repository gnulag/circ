#!/bin/sh

CIRC_DEBUG="yes please" \
CIRC_SERVER_NAME=Snoonet \
CIRC_SERVER_HOST=irc.snoonet.org \
CIRC_SERVER_PORT=6697 \
CIRC_SSL=true \
CIRC_NICK=circ \
CIRC_REALNAME=https://github.com/gnulag/circ/ \
CIRC_IDENT=circ \
CIRC_SASL_ENABLED=false \
CIRC_AUTH_USER=circ \
CIRC_AUTH_PASS=circ \
CIRC_CHANNEL=#gnulag \
./circ
