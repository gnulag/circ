(load "scheme_libs/privmsg.scm")
(import (chibi sqlite3))

(define db (sqlite3-open (get-db-path)))
(sqlite3-do db "CREATE TABLE IF NOT EXISTS irc_logs (time TIMESTAMP DEFAULT CURRENT_TIMESTAMP, server TEXT, channel TEXT, nick TEXT, message TEXT, action INTEGER)")

(define (insert-log)
  (let*
      ((server (get-server-name))
       (channel (get-channel))
       (nick (get-nick))
       (message (get-text))
       (action? (and
                 (equal? #\x01 (string-ref message 0))
                 (equal? #\x01 (string-ref message (- (string-length message) 1)))
                 (equal? "ACTION" (substring message 1 7)))))
    (sqlite3-do db
                "INSERT INTO irc_logs (server, channel, nick, message, action) VALUES (?, ?, ?, ?, ?)"
                server channel nick
                (if action?
                    (substring message 8 (- (string-length message) 1))
                    message)
                action?)))

(register-hook "PRIVMSG" insert-log)
