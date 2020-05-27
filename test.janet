(import build/pobox :as pobox)

(pp (pobox/make :counter 0))
(pp (pobox/get :counter))
(pp (pobox/update :counter (fn [x] (inc x))))
(pp (pobox/get :counter))
(pp (pobox/get-all))

# Will result in an error (one call per)
(pp (pobox/make "Goodbye" "World"))
