#include "userprog/syscall.h"
#include <stdint.h>
#include <syscall-nr.h>
#include "devices/shutdown.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "stdio.h"
#include <string.h>
#include "threads/vaddr.h"
#include "userprog/pagedir.h"

static void syscall_handler (struct intr_frame *);
static int get_sys_number (const struct intr_frame *f);
static void sys_exit (int status) NO_RETURN;

static uint32_t copy_in_u32 (const void *uaddr);
static void validate_user_range (const void *uaddr, unsigned size);
int syscall_write (int fd, const void *buffer, unsigned size);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
validate_user_range (const void *uaddr, unsigned size)
{
  const uint8_t *p = (const uint8_t *) uaddr;
  struct thread *t = thread_current ();
  unsigned i;

  if (uaddr == NULL)
    sys_exit (-1);

  for (i = 0; i < size; i++)
    {
      const void *a = p + i;
      if (!is_user_vaddr (a) || pagedir_get_page (t->pagedir, a) == NULL)
        sys_exit (-1);
    }
}

static uint32_t
copy_in_u32 (const void *uaddr)
{
  uint32_t v;
  validate_user_range (uaddr, sizeof v);
  memcpy (&v, uaddr, sizeof v);
  return v;
}

static int
get_sys_number (const struct intr_frame *f)
{
  return (int) copy_in_u32 (f->esp);
}

static void
sys_exit (int status)
{
  thread_current ()->exit_status = status;
  thread_exit ();
}

static void
syscall_handler (struct intr_frame *f)
{
  int sys_number = get_sys_number (f);

  switch (sys_number)
    {
      case SYS_HALT:
        shutdown_power_off ();
        break;

      case SYS_EXIT:
        sys_exit ((int) copy_in_u32 (f->esp + 4));
        break;
      
      case SYS_WRITE:
      {
        int fd = (int) copy_in_u32 (f->esp + 4);
        const void *buffer = (const void *) copy_in_u32 (f->esp + 8);
        unsigned size = (unsigned) copy_in_u32 (f->esp + 12);
        validate_user_range (buffer, size); /* buffer must be readable */
        f->eax = syscall_write (fd, buffer, size);
        break;
      }

      default:
        sys_exit (-1);
        break;
    }
}

int
syscall_write (int fd, const void *buffer, unsigned size)
{
  /* For simplicity, we only handle writing to stdout (fd = 1). */
  if (fd != 1)
  return -1;
  /* Write to console output. */
  putbuf (buffer, size);
  return size;

}