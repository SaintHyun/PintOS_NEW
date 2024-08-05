#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "filesys/file.h"

struct file
{
  struct inode *inode; /* File's inode. */
  off_t pos;           /* Current position. */
  bool deny_write;     /* Has file_deny_write() been called? */
};

struct semaphore reader, writer;
int readcount;

static void syscall_handler(struct intr_frame *);

void syscall_init(void)
{
  sema_init(&reader, 1);
  sema_init(&writer, 1);
  readcount = 0;
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void check_address(void *addr)
{
  if (!is_user_vaddr(addr))
    exit(-1);
}

static void
syscall_handler(struct intr_frame *f)
{
  check_address(f->esp);
  switch (*(uint32_t *)f->esp)
  {
  case SYS_HALT:
    shutdown_power_off();
    break;
  case SYS_EXIT:
    check_address(f->esp + 4);
    exit((int)*(uint32_t *)(f->esp + 4));
    break;
  case SYS_EXEC:
    check_address(f->esp + 4);
    f->eax = exec((const char *)*(uint32_t *)(f->esp + 4));
    break;
  case SYS_WAIT:
    check_address(f->esp + 4);
    f->eax = wait((pid_t) * (uint32_t *)(f->esp + 4));
    break;
  case SYS_CREATE:
    check_address(f->esp + 4);
    check_address(f->esp + 8);
    f->eax = create((const char *)*(uint32_t *)(f->esp + 4), (unsigned)*(uint32_t *)(f->esp + 8));
    break;
  case SYS_REMOVE:
    check_address(f->esp + 4);
    f->eax = remove((const char *)*(uint32_t *)(f->esp + 4));
    break;
  case SYS_OPEN:
    check_address(f->esp + 4);
    f->eax = open((const char *)*(uint32_t *)(f->esp + 4));
    break;
  case SYS_FILESIZE:
    check_address(f->esp + 4);
    f->eax = filesize((const char *)*(uint32_t *)(f->esp + 4));
    break;
  case SYS_READ:
    check_address(f->esp + 4);
    check_address(f->esp + 8);
    check_address(f->esp + 12);
    f->eax = read((int)*(uint32_t *)(f->esp + 4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*(uint32_t *)(f->esp + 12));
    break;
  case SYS_WRITE:
    check_address(f->esp + 4);
    check_address(f->esp + 8);
    check_address(f->esp + 12);
    f->eax = write((int)*(uint32_t *)(f->esp + 4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*(uint32_t *)(f->esp + 12));
    break;
  case SYS_SEEK:
    check_address(f->esp + 4);
    check_address(f->esp + 8);
    seek((int)*(uint32_t *)(f->esp + 4), (unsigned)*(uint32_t *)(f->esp + 8));
    break;
  case SYS_TELL:
    check_address(f->esp + 4);
    f->eax = tell((int)*(uint32_t *)(f->esp + 4));
    break;
  case SYS_CLOSE:
    check_address(f->esp + 4);
    close((int)*(uint32_t *)(f->esp + 4));
    break;
  }
}

void exit(int status)
{
  thread_current()->exit_status = status;
  printf("%s: exit(%d)\n", thread_current()->name, status);
  thread_exit();
}

pid_t exec(const char *cmd_line)
{
  return process_execute(cmd_line);
}

int wait(pid_t pid)
{
  return process_wait(pid);
}

bool create(const char *file, unsigned initial_size)
{
  if (file == NULL)
    exit(-1);
  return filesys_create(file, initial_size);
}

bool remove(const char *file)
{
  if (file == NULL)
    exit(-1);
  return filesys_remove(file);
}

int open(const char *file)
{
  check_address(file);
  if (file == NULL)
    exit(-1);

  sema_down(&reader);
  readcount++;
  if (readcount == 1)
    sema_down(&writer);
  sema_up(&reader);

  struct file *f = filesys_open(file);
  if (f == NULL)
  {
    sema_down(&reader);
    readcount--;
    if (readcount == 0)
      sema_up(&writer);
    sema_up(&reader);
    return -1;
  }

  for (int i = 3; i < 128; i++)
  {
    if (thread_current()->fd[i] == NULL)
    {
      if (strcmp(thread_current()->name, file) == 0)
        file_deny_write(f);
      thread_current()->fd[i] = f;

      sema_down(&reader);
      readcount--;
      if (readcount == 0)
        sema_up(&writer);
      sema_up(&reader);

      return i;
    }
  }
  sema_down(&reader);
  readcount--;
  if (readcount == 0)
    sema_up(&writer);
  sema_up(&reader);

  return -1;
}

int filesize(int fd)
{
  if (thread_current()->fd[fd] == NULL)
    exit(-1);
  return file_length(thread_current()->fd[fd]);
}

int read(int fd, void *buffer, unsigned size)
{
  check_address(buffer);
  int read_byte = -1;

  sema_down(&reader);
  readcount++;
  if (readcount == 1)
    sema_down(&writer);
  sema_up(&reader);

  if (fd == 0)
  {
    for (read_byte = 0; read_byte < size; read_byte++)
    {
      ((char *)buffer)[read_byte] = input_getc();
      if (((char *)buffer)[read_byte] == '\0')
      {
        break;
      }
    }
  }
  else if (fd >= 2)
  {
    if (thread_current()->fd[fd] == NULL)
    {
      sema_down(&reader);
      readcount--;
      if (readcount == 0)
        sema_up(&writer);
      sema_up(&reader);
      exit(-1);
    }
    read_byte = file_read(thread_current()->fd[fd], buffer, size);
  }

  sema_down(&reader);
  readcount--;
  if (readcount == 0)
    sema_up(&writer);
  sema_up(&reader);

  return read_byte;
}

int write(int fd, const void *buffer, unsigned size)
{
  check_address(buffer);

  int write_byte = -1;

  sema_down(&writer);

  if (fd == 1)
  {
    putbuf(buffer, size);
    write_byte = size;
  }
  else if (fd >= 2)
  {
    if (thread_current()->fd[fd] == NULL)
    {
      sema_up(&writer);
      exit(-1);
    }
    if (thread_current()->fd[fd]->deny_write)
      file_deny_write(thread_current()->fd[fd]);
    write_byte = file_write(thread_current()->fd[fd], buffer, size);
  }
  sema_up(&writer);
  return write_byte;
}

void seek(int fd, unsigned position)
{
  if (thread_current()->fd[fd] == NULL)
    exit(-1);
  file_seek(thread_current()->fd[fd], position);
}

unsigned tell(int fd)
{
  if (thread_current()->fd[fd] == NULL)
    exit(-1);
  return file_tell(thread_current()->fd[fd]);
}

void close(int fd)
{
  if (thread_current()->fd[fd] == NULL)
    exit(-1);
  file_close(thread_current()->fd[fd]);
  thread_current()->fd[fd] = NULL;
  return;
}
