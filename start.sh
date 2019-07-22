#!/bin/sh

CIRC_DEBUG="yes please" \
CIRC_SERVER=localhost \
CIRC_PORT=6667 \
CIRC_SSL=false \
CIRC_NICK=circ \
CIRC_REALNAME=https://github.com/gnulag/circ/ \
CIRC_IDENT=circ \
CIRC_SASL_ENABLED=false \
CIRC_AUTH_USER=circ \
CIRC_AUTH_PASS=circ \
CIRC_CHANNEL=#gnulag \
./build/circ
