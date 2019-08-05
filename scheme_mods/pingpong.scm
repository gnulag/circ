(define (pong)
  (reply "pong"))

(define (ping)
  (reply "ping"))

(register-command "ping" pong)
(register-command "pong" ping)
