(load "scheme_libs/privmsg.scm")

(define (pong)
  (reply (string-append (get-cmd-prefix) "pong")))

(define (ping)
  (reply (string-append (get-cmd-prefix) "ping")))

(register-command "ping" pong)
(register-command "pong" ping)
