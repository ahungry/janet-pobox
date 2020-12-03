# Janet PObox

Janet uses message passing among threads for it's native concurrency
model.

While I like the Erlang'ish approach, I was missing Clojure atoms, so
I whipped this together.  So far, it seems lossless after using
pthread spinlocks instead of my hand-rolled one (which was still
roughly 99.9% lossless).

```clojure
(import build/pobox :as pobox)

(thread/new (fn [_] (pobox/make :counter 0)))

(map (fn [_] (thread/new (fn [_] (os/sleep 1) (pobox/update :counter inc))))
     (range 100))

(pobox/make :map @{:a 1})

# # Also illustrate some concurrency in just using a common storage area
# # among Janet threads
(thread/new (fn [_] (os/sleep 0.2) (pobox/update :map (fn [m] (put m :b 2)))))
(thread/new (fn [_] (os/sleep 0.2) (pobox/update :map (fn [m] (put m :c 3)))))
(thread/new (fn [_] (os/sleep 0.2) (pobox/update :map (fn [m] (put m :d "woohoo")))))
(thread/new (fn [_] (os/sleep 0.2) (pobox/update :map (fn [m] (put m :b {:x 10 :w 20})))))

# # Give enough sleep to let things finish
(os/sleep 2)

(pp (pobox/get :counter))
(pp (pobox/get :map))

(assert (= 100 (pobox/get :counter)))
(assert (deep= @{:c 3 :a 1 :b 2 :d "woohoo" :b {:x 10 :w 20}} (pobox/get :map)))

(print "All tests passing!")

```

```sh
true
100
@{:a 1 :c 3 :b 2}
janet test.janet  0.23s user 0.15s system 18% cpu 2.058 total
```

# License

Copyright 2020 Matthew Carter <m@ahungry.com> GPLv3 or later
