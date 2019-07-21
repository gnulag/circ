#! /usr/bin/racket
#lang racket/base
(require json)

(define configuration #hasheq())

(define (process-message message)
  (format "~a ~a! back at you: ~a"
	  (hash-ref configuration 'prefix "no u")
	  (hash-ref message 'nick)
	  (hash-ref message 'text)))

(define (create-answer str)
  (hash 'request "send-message"
	'payload `(,(hash 'text str))))

(let loop ()
  (let* ([message (read-json)]
	 [command (hash-ref message 'cmd)]
	 [payload (car (hash-ref message 'payload))])
    (case command
      [("configure")
       (set! configuration payload)]
      [("message")
       (let ([result (process-message payload)])
	 (begin
           (write-json (create-answer result))
           (flush-output)))]))
  (loop))

