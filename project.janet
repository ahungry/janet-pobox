(declare-project
  :name "pobox"
  :description "STM (software transactional memory) for Janet"
  :author "Matthew Carter"
  :license "MIT"
  :url "https://github.com/ahungry/janet-pobox/"
  :repo "git+https://github.com/ahungry/janet-pobox.git")

(declare-native
  :name "pobox"
  :cflags ["-Wall" "-Wextra"]
  :lflags ["-pthread"]
  :embedded @["pobox_lib.janet"]
  :source @["pobox.c"])
