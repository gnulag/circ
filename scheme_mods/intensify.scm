(import (chibi string))

(define (intensify)
  (let*
    ((text (cadr (get-message-params)))
     (inner-text (string-trim-left
		  (string-trim-right text #\])
		  #\[)))
     (reply (string-map char-upcase
	      (string-append "" inner-text " intensifies" "")))))

(register-match "^\\[.+\\]$" intensify)
