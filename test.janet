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
