#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
void argument_stack(char **argv, int argc, void **esp);

struct thread *get_child_process(int pid);
void remove_child_process(struct thread *child);

#endif /* userprog/process.h */
