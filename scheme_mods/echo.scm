(import (chibi string))

(define (echo)
  (let*
    ((text (cadr (get-message-params)))
     (words (string-split text))
     (rest (string-join (cdr words) " ")))
    (reply rest)))

(register-command "echo" echo)
