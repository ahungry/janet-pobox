(defn hi [] (pp "hi"))

(defn make [k v]
  (cmake k (marshal v)))

(defn update [k f]
  (cupdate k (fn [x]
               (marshal (f (unmarshal x))))))

(defn get [k]
  (if (cget k)
    (unmarshal (cget k))
    nil))
