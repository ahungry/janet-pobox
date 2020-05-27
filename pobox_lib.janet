(defn hi [] (pp "hi"))

(defn make [k v]
  (cmake (marshal k) (marshal v)))

(defn update [k f]
  (cupdate (marshal k) (fn [x]
                         (marshal (f (unmarshal x))))))

(defn get [k]
  (def maybe (cget (marshal k)))
  (if maybe
    (unmarshal maybe)
    nil))
