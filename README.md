# Janet PObox

Janet uses message passing among threads for it's native concurrency
model.

While I like the Erlang'ish approach, I was missing Clojure atoms, so
I whipped this together.  So far, it seems lossless after using
pthread spinlocks instead of my hand-rolled one (which was still
roughly 99.9% lossless).

```clojure
(import build/pobox :as pobox)

# Apparently the resolution for :kw or 'sym is not 'same' in threads
# While it is for string keys and perhaps others (numbers?)

(pp (pobox/make "counter" 0))

(map (fn [_] (thread/new (fn [_] (os/sleep 1) (pobox/update "counter" inc))))
     (range 10000))


(pobox/make "map" @{:a 1})
(thread/new (fn [_] (os/sleep 0.2) (pobox/update "map" (fn [m] (put m :b 2)))))
(thread/new (fn [_] (os/sleep 0.2) (pobox/update "map" (fn [m] (put m :c 3)))))

# Give enough sleep to let things finish
(os/sleep 2)

# Equals 1000 as
(pp (pobox/get "counter"))
(pp (pobox/get "map"))

```

```sh
true
10000
@{:a 1 :b 2 : 3}
janet test.janet  52.69s user 7.11s system 562% cpu 10.639 total
```

# License

Copyright 2020 Matthew Carter <m@ahungry.com> GPLv3 or later
