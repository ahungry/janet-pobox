#include <janet.h>
#include <unistd.h>
#include <pthread.h>

pthread_spinlock_t lock;
int pshared = PTHREAD_PROCESS_SHARED;
int ret;

JanetTable * office = NULL;
Janet office_keep;

void
ensure_office ()
{
  if (NULL == office)
    {
      office = janet_table (0);
      office_keep = janet_wrap_table (office);
      janet_gcroot (office_keep);

      ret = pthread_spin_init(&lock, pshared);
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
}

static Janet
get_wrapped (int32_t argc, Janet *argv)
{
  janet_fixarity (argc, 1);

  return get (argv[0]);
}

static Janet
update_wrapped (int32_t argc, Janet *argv)
{
  janet_fixarity (argc, 2);

  JanetFunction *f = janet_getfunction (argv, 1);
  Janet k = argv[0];

  pthread_spin_lock (&lock);
  Janet call_args[] = { janet_table_get (office, k) };
  Janet v = janet_call (f, 1, call_args);

  janet_table_put (office, k, v);
  pthread_spin_unlock (&lock);

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
