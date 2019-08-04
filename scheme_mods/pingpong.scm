(define (pong)
  (reply "pong"))

(register-command "ping" pong)
