/*
 * newlib 시스템 콜 — nosys.specs 대신 여기서 만든다.
 *
 *   _sbrk   : 링커 스크립트의 .heap (_sheap ~ _eheap) 안에서만 늘린다.
 *             yss 는 스레드 스택까지 new(→ malloc) 로 받으므로 경계를 넘으면 바로 실패시켜야
 *             다른 메모리를 덮지 않는다.
 *
 * _write 는 콘솔 장치와 함께 src/dev/driver/dev_console.cpp 에 있다.
 */
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>


extern char _sheap;
extern char _eheap;

static char *heap_end = 0;


void *_sbrk(ptrdiff_t incr)
{
  char *prev;

  if (heap_end == 0)
    heap_end = &_sheap;

  if (heap_end + incr > &_eheap)
  {
    errno = ENOMEM;
    return (void *)-1;
  }

  prev = heap_end;
  heap_end += incr;
  return prev;
}

int _read(int file, char *ptr, int len)
{
  (void)file;
  (void)ptr;
  (void)len;
  return 0;
}

int _close(int file)
{
  (void)file;
  return -1;
}

int _fstat(int file, struct stat *st)
{
  (void)file;
  st->st_mode = S_IFCHR;
  return 0;
}

int _isatty(int file)
{
  (void)file;
  return 1;
}

int _lseek(int file, int ptr, int dir)
{
  (void)file;
  (void)ptr;
  (void)dir;
  return 0;
}

int _getpid(void)
{
  return 1;
}

int _kill(int pid, int sig)
{
  (void)pid;
  (void)sig;
  errno = EINVAL;
  return -1;
}

void _exit(int status)
{
  (void)status;
  while (1)
  {
  }
}
