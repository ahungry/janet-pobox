#include <janet.h>
#include <unistd.h>

int lock_singleton = 0;
JanetTable * office = NULL;
JanetTable * lock = NULL;
JanetTable * check = NULL;
Janet office_keep;
Janet lock_keep;
Janet check_keep;

void
ensure_office ()
{
  if (NULL == office)
    {
      office = janet_table (0);
      office_keep = janet_wrap_table (office);
      janet_gcroot (office_keep);

      lock = janet_table (0);
      lock_keep = janet_wrap_table (lock);
      janet_gcroot (lock_keep);

      check = janet_table (0);
      check_keep = janet_wrap_table (check);
      janet_gcroot (check_keep);
    }
}

Janet
get (Janet k)
{
  return janet_table_get (office, k);
}

void
make_box (Janet k, Janet v)
{
  janet_table_put (office, k, v);
  janet_table_put (lock, k, janet_wrap_integer (0));
  janet_table_put (check, k, janet_wrap_integer (0));
}

static Janet
get_wrapped (int32_t argc, Janet *argv)
{
  janet_fixarity (argc, 1);

  return get (argv[0]);
}

int
is_locked (Janet k)
{
  if (lock_singleton)
    {
      return 1;
    }

  Janet v = janet_table_get (lock, k);

  return janet_unwrap_integer (v);
}

int
acquire_lock (Janet k)
{
  if (is_locked (k))
    {
      return 0;
    }

  // Ensure the check value is the same before and after
  int ocv = janet_unwrap_integer (janet_table_get (check, k));
  janet_table_put (lock, k, janet_wrap_integer (1));
  int ncv = janet_unwrap_integer (janet_table_get (check, k));

  // If the update count is the same before and after, we're good.
  if (ocv == ncv)
    {
      lock_singleton = 1;
      janet_table_put (check, k, janet_wrap_integer (ncv + 1));

      return 1;
    }
  else
    {
      return 0;
    }
}

void
release_lock (Janet k)
{
  janet_table_put (lock, k, janet_wrap_integer (0));
  lock_singleton = 0;
}

static Janet
update_wrapped (int32_t argc, Janet *argv)
{
  janet_fixarity (argc, 2);

  JanetFunction *f = janet_getfunction (argv, 1);
  Janet k = argv[0];

  // Acquire the lock
  while (acquire_lock (k)) { sleep(0.01); }

  Janet call_args[] = { janet_table_get (office, k) };
  Janet v = janet_call (f, 1, call_args);

  janet_table_put (office, k, v);

  release_lock (k);

  return get (k);
}

static Janet
get_all_wrapped (int32_t argc, __attribute__((__unused__)) Janet *argv)
{
  janet_fixarity (argc, 0);

  return janet_wrap_table (office);
}

static Janet
make_wrapped (int32_t argc, Janet *argv)
{
  janet_fixarity (argc, 2);

  ensure_office ();
  make_box (argv[0], argv[1]);

  return janet_wrap_true ();
}

static const JanetReg
pobox_cfuns[] = {
  {"make", make_wrapped, "(box key val)\nCreate a box of initial val, val in slot key."},
  {"get", get_wrapped, ""},
  {"get-all", get_all_wrapped, ""},
  {"update", update_wrapped, ""},
  {NULL,NULL,NULL}
};

JANET_MODULE_ENTRY (JanetTable *env) {
  janet_cfuns (env, "pobox", pobox_cfuns);
}
