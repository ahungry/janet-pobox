#include <janet.h>
#include <unistd.h>
#include <pthread.h>

pthread_spinlock_t lock;
int pshared = PTHREAD_PROCESS_SHARED;
int ret;

typedef struct office_s {
  char *k;
  uint8_t *v;
  int vlen;
  struct office_s * next;
} office_t;

office_t * office = NULL;

void
ensure_office ()
{
  if (NULL == office)
    {
      office = malloc (sizeof (office_t));
      office->k = "root";
      office->v = (uint8_t*)"root";
      office->next = NULL;
      ret = pthread_spin_init (&lock, pshared);
    }
}

office_t *
table_get (office_t * t, char * k)
{
  office_t * node = t;

  do {
    if (0 == strcmp (node->k, k))
      {
        return node;
      }
  } while ((node = node->next) != NULL);

  return NULL;
}

void
table_put (office_t * t, char * k, uint8_t * v, int len)
{
  office_t * node = t;
  office_t * last_node = NULL;
  int found = 0;

  do {
    last_node = node;

    if (0 == strcmp (node->k, k))
      {
        found = 1;
        break;
      }
  } while ((node = node->next) != NULL);

  if (found)
    {
      node->v = malloc (sizeof (uint8_t) * len);
      memcpy (node->v, v, len);
      node->vlen = len;
    }
  else
    {
      office_t *nnode = malloc (sizeof (office_t));
      last_node->next = nnode;

      nnode->k = malloc (sizeof (char) * strlen (k));
      memcpy (nnode->k, k, strlen (k));
      nnode->k[strlen (k)] = '\0';
      nnode->v = malloc (sizeof (uint8_t) * len);
      memcpy (nnode->v, v, len);
      nnode->vlen = len;
      nnode->next = NULL;
    }
}

office_t *
get (char * k)
{
  return table_get (office, k);
}

void
make_box (char * k, uint8_t * v, int len)
{
  table_put (office, k, v, len);
}

static Janet
get_wrapped (int32_t argc, Janet *argv)
{
  janet_fixarity (argc, 1);

  pthread_spin_lock (&lock);

  const char *k = janet_getcstring (argv, 0);
  int klen = strlen (k);
  char *key = malloc (sizeof (char) * klen + sizeof (char));
  memcpy (key, k, klen);
  key[klen] = '\0';

  office_t * t = get (key);

  if (NULL == t)
    {
      return janet_wrap_nil ();
    }

  uint8_t *v = t->v;
  int len = t->vlen;

  JanetBuffer *buf = janet_buffer (sizeof (const uint8_t) * len);
  janet_buffer_push_bytes (buf, (const uint8_t *) v, len);
  pthread_spin_unlock (&lock);

  return janet_wrap_buffer (buf);
}

static Janet
update_wrapped (int32_t argc, Janet *argv)
{
  pthread_spin_lock (&lock);

  janet_fixarity (argc, 2);

  JanetFunction *f = janet_getfunction (argv, 1);
  const char *k = janet_getcstring (argv, 0);

  office_t * t = get ((char*)k);
  uint8_t * varg = t->v;
  int len = t->vlen;

  JanetBuffer *buf = janet_buffer (sizeof (const uint8_t) * len);
  janet_buffer_push_bytes (buf, (const uint8_t *) varg, len);
  Janet jb = janet_wrap_buffer (buf);

  Janet call_args[] = { jb };
  Janet res = janet_call (f, 1, call_args);

  JanetBuffer *vb = janet_unwrap_buffer (res);

  uint8_t * v = malloc (sizeof (uint8_t) * vb->count);
  memcpy (v, vb->data, vb->count);

  make_box ((char*)k, v, vb->count);
  pthread_spin_unlock (&lock);

  return janet_wrap_true ();
}

static Janet
make_wrapped (int32_t argc, Janet *argv)
{
  janet_fixarity (argc, 2);

  ensure_office ();
  pthread_spin_lock (&lock);
  const char * k = janet_getcstring (argv, 0);
  JanetBuffer * vb = janet_getbuffer (argv, 1);

  // Ensure GC doesn't screw us over
  int klen = strlen (k);
  char * key = malloc (sizeof (char) * klen + sizeof (char));
  memcpy (key, k, klen);
  key[klen] = '\0';
  uint8_t * v = malloc (sizeof (uint8_t) * vb->count);
  memcpy (v, vb->data, vb->count);

  make_box (key, v, vb->count);
  pthread_spin_unlock (&lock);

  return janet_wrap_true ();
}

static const JanetReg
pobox_cfuns[] = {
  {"cmake", make_wrapped, "(box key val)\nCreate a box of initial val, val in slot key."},
  {"cget", get_wrapped, ""},
  {"cupdate", update_wrapped, ""},
  {NULL,NULL,NULL}
};

extern const unsigned char *pobox_lib_embed;
extern size_t pobox_lib_embed_size;

JANET_MODULE_ENTRY (JanetTable *env) {
  janet_cfuns (env, "pobox", pobox_cfuns);
  janet_dobytes(env,
                pobox_lib_embed,
                pobox_lib_embed_size,
                "pobox_lib.janet",
                NULL);
}
