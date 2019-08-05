(define (echo)
  (reply (cadr (get-message-params))))

(register-command "echo" echo)
