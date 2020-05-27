#include <janet.h>
#include <unistd.h>
#include <pthread.h>

pthread_spinlock_t lock;
int pshared = PTHREAD_PROCESS_SHARED;
int ret;

typedef struct office_s {
  char *k;
  char *v;
  int klen;
  int vlen;
  struct office_s * next;
} office_t;

office_t * office = NULL;

char *
janet_buffer_to_cstring (JanetBuffer *buf)
{
  char * v = malloc (sizeof (uint8_t) * buf->count);
  memcpy (v, buf->data, buf->count);
  v[buf->count] = '\0';

  return v;
}

void
ensure_office ()
{
  if (NULL == office)
    {
      office = malloc (sizeof (office_t));
      office->k = (uint8_t*)"root";
      office->v = (uint8_t*)"root";
      office->next = NULL;
      ret = pthread_spin_init (&lock, pshared);
    }
}

office_t *
table_get (JanetBuffer * kb)
{
  office_t * node = office;
  char * k = janet_buffer_to_cstring (kb);

  do {
    if (0 == strcmp ((char*)node->k, (char*)k))
      {
        return node;
      }
  } while ((node = node->next) != NULL);

  return NULL;
}

void
table_put (JanetBuffer * kb, JanetBuffer * kv)
{
  char * k = janet_buffer_to_cstring (kb);
  char * v = janet_buffer_to_cstring (kv);
  int klen = kb->count;
  int vlen = kv->count;

  office_t * node = office;
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
      node->v = malloc (sizeof (uint8_t) * vlen);
      memcpy (node->v, v, vlen);
      node->vlen = vlen;
    }
  else
    {
      office_t *nnode = malloc (sizeof (office_t));
      last_node->next = nnode;

      nnode->k = malloc (sizeof (char) * klen);
      memcpy (nnode->k, k, klen);
      nnode->k[klen] = '\0';
      nnode->klen = klen;

      nnode->v = malloc (sizeof (char) * vlen);
      memcpy (nnode->v, v, vlen);
      nnode->v[vlen] = '\0';
      nnode->vlen = vlen;

      nnode->next = NULL;
    }
}

office_t *
get (JanetBuffer * kb)
{
  return table_get (kb);
}

void
make_box (JanetBuffer * kb, JanetBuffer * vb)
{
  table_put (kb, vb);
}

static Janet
get_wrapped (int32_t argc, Janet *argv)
{
  janet_fixarity (argc, 1);

  pthread_spin_lock (&lock);

  JanetBuffer *kb = janet_getbuffer (argv, 0);
  office_t * t = get (kb);

  if (NULL == t)
    {
      return janet_wrap_nil ();
    }

  char *v = t->v;
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
  JanetBuffer *kb = janet_getbuffer (argv, 0);

  office_t * t = get (kb);
  char * varg = t->v;
  int len = t->vlen;

  JanetBuffer *buf = janet_buffer (sizeof (const uint8_t) * len);
  janet_buffer_push_bytes (buf, (const uint8_t *) varg, len);
  Janet jb = janet_wrap_buffer (buf);

  Janet call_args[] = { jb };
  Janet res = janet_call (f, 1, call_args);

  JanetBuffer *vb = janet_unwrap_buffer (res);

  make_box (kb, vb);
  pthread_spin_unlock (&lock);

  return janet_wrap_true ();
}

static Janet
make_wrapped (int32_t argc, Janet *argv)
{
  janet_fixarity (argc, 2);

  ensure_office ();
  pthread_spin_lock (&lock);

  JanetBuffer * kb = janet_getbuffer (argv, 0);
  JanetBuffer * vb = janet_getbuffer (argv, 1);

  make_box (kb, vb);

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
