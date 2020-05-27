(import build/pobox :as pobox)

(pp (pobox/make :counter 0))

(map (fn [_] (thread/new (fn [_] (os/sleep 1) (pobox/update :counter inc))))
     (range 100))

(pobox/make :map @{:a 1})

# # Also illustrate some concurrency in just using a common storage area
# # among Janet threads
(thread/new (fn [_] (os/sleep 0.2) (pobox/update :map (fn [m] (put m :b 2)))))
(thread/new (fn [_] (os/sleep 0.2) (pobox/update :map (fn [m] (put m :c 3)))))

# # Give enough sleep to let things finish
(os/sleep 2)

(pp (pobox/get :counter))
(pp (pobox/get :map))
