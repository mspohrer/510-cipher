// 5M: Maniacal Mark's Magical Message Manipulator
// example:
// $ ./cipher -e -i ClearText > CipherText
// $ ./cipher -d -i CipherText > foo
// The original clear text (plus padding) is now in file foo
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <error.h>

#define ABORT(a) \
  ({error(EXIT_FAILURE, 0, "Usage: %s [-e|-d] -i <file>", a);})
#define ZERO 0x30
#define SPACE 0x20
#define MAPPING "64752031"
#define GROUPSIZE 8  // strlen(MAPPING);

enum OPERATION {ENCODE, DECODE};

typedef struct layout {
  int  op;
  char *fname;
  long int bufSize;
  char *buffer;
  char clear[GROUPSIZE];
  char coded[GROUPSIZE];
  char *pmap;  // permutation map
} data_t;

static void
Fopen(char *name, char *mode, FILE **fp)
{
  *fp = fopen(name, mode);
  if (*fp == NULL)
    error(EXIT_FAILURE, errno, "\"%s\"", name);
}

static void
Fstat(int fd, struct stat *st)
{
  if (fstat(fd, st) == -1)
    error(EXIT_FAILURE, errno, "stat failed");
}

static void
Malloc(char **buf, uint size)
{
  *buf = malloc(size);
  if (buf == NULL)
    error(EXIT_FAILURE, 0, "malloc failed");
}

static long int
readFile(data_t *d)
{
  long int count;
  FILE *fp;
  struct stat st;

  Fopen(d->fname, "r", &fp);
  Fstat(fileno(fp), &st);
  Malloc(&d->buffer, st.st_size + 1);
  memset(d->buffer, 0, st.st_size + 1);

  if (d->op == ENCODE) {
    count = fread(d->buffer, sizeof(char), st.st_size, fp);
    fclose(fp);
    if (count != st.st_size) {
      fprintf(stderr, "In %s: asked for %ld and got %ld\n",
          __FUNCTION__, st.st_size, count);
      return -1;
    }
  }
  else {  // DECODE
    int v;
    count = -1;
    while ((fscanf(fp, "%x", &v) != EOF) && (count <= st.st_size))
      d->buffer[++count] = v;
    fclose(fp);
  }
  return count;
}

static void
swap(data_t *d)
{
  int i;

  for (i=0; i<GROUPSIZE; i++)
    if (d->op == ENCODE)
      d->coded[d->pmap[i]-ZERO] = d->clear[i];
    else  // decode
      d->clear[i] = d->coded[d->pmap[i]-ZERO];
  return;
}

static void
encode(data_t *d)
{
  int j, fixup;
  long int i;

  d->bufSize = readFile(d);
  if (d->bufSize == -1)
    error(EXIT_FAILURE, 0, "Unable to encode.");
  fixup = d->bufSize % GROUPSIZE;  // last group may be partial
  for (i=0; i<(d->bufSize-fixup); i+=GROUPSIZE) {
    strncpy(d->clear, d->buffer+i, GROUPSIZE);
    swap(d);
    for (j=0; j<GROUPSIZE; j++)
       printf("%02x ", d->coded[j]);
  }
  memset(d->clear, SPACE, GROUPSIZE);  // pad
  strncpy(d->clear, d->buffer+i, fixup);
  swap(d);
  for (j=0; j<GROUPSIZE; j++)
    printf("%02x ", d->coded[j]);
  free(d->buffer);
  return;
}

// Note: encode may pad so decode may produce extra spaces. sigh.
static void
decode(data_t *d)
{
  int j;
  long int i;

  d->bufSize = readFile(d);
  if (d->bufSize == -1)
    error(EXIT_FAILURE, 0, "Unable to decode.\n");
  for (i=0; i<d->bufSize; i+=GROUPSIZE) {
    strncpy(d->coded, d->buffer+i, GROUPSIZE);
    swap(d);
    for (j=0; j<GROUPSIZE; j++) {
      printf("%c", d->clear[j]);
    }
  }
  printf("\n");
  free(d->buffer);
  return;
}

int
main(int argc, char *argv[])
{
  int c;
  data_t data;

  if (argc != 4)
    ABORT(argv[0]);

  // getopt() is GNU so not portable to other compilers
  while ((c = getopt (argc, argv, "dei:")) != -1)
    switch (c) {
      case 'd':
        data.op = DECODE;
        break;
      case 'e':
        data.op = ENCODE;
        break;
      case 'i':
        data.fname = optarg;
        break;
      default:
        ABORT(argv[0]);
    }
  data.pmap = MAPPING;
  if (data.op == ENCODE)
    encode(&data);
  else
    decode(&data);
  exit(EXIT_SUCCESS);
}
