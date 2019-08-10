(load "scheme_libs/privmsg.scm")
(import (chibi string))

(define (intensify)
  (let*
    ((text (cadr (get-message-params)))
     (inner-text (string-trim-left
		  (string-trim-right text #\])
		  #\[)))
     (reply (string-map char-upcase
	      (string-append "\x02" inner-text " intensifies" "\x02")))))

(register-match "^\\[.+\\]$" intensify)
