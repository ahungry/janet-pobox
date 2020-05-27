(import build/pobox :as pobox)

# Apparently the resolution for :kw or 'sym is not 'same' in threads
# While it is for string keys and perhaps others (numbers?)
(pp (pobox/make "counter" 0))

# Increment each thing in the set - the result should be 10,000
(map (fn [_] (thread/new (fn [_] (os/sleep 1) (pobox/update "counter" inc))))
     (range 10000))

# Also illustrate some concurrency in just using a common storage area
# among Janet threads
(pobox/make "map" @{:a 1})
(thread/new (fn [_] (os/sleep 0.2) (pobox/update "map" (fn [m] (put m :b 2)))))
(thread/new (fn [_] (os/sleep 0.2) (pobox/update "map" (fn [m] (put m :c 3)))))

# Give enough sleep to let things finish
(os/sleep 2)

# Equals 10000 as expected
(pp (pobox/get "counter"))
(pp (pobox/get "map"))
