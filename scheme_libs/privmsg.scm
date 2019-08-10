(import (chibi string))

(define (send-privmsg target text)
  (send-raw
   (string-append "PRIVMSG " target " :" text "\r\n")))

(define (send-action target text)
  (send-raw
   (string-append "PRIVMSG " target " :" "\x01" "ACTION " text "\x01" "\r\n")))

(define (reply text)
  (send-privmsg (get-channel) text))

(define (reply-action text)
  (send-action (get-channel) text))

(define (get-channel)
  (car (get-message-params)))

(define (get-text)
  (cadr (get-message-params)))

(define (get-nick)
  (car (string-split (get-message-source) #\!)))

(define (get-ident)
  (car
   (string-split
    (cadr
     (string-split (get-message-source) #\!))
    #\@)))

(define (get-host)
  (cadr (string-split (get-message-source) #\@)))
