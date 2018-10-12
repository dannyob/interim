#define CODESZ 8192
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

Cell* execute_jitted(void* binary) {
  return (Cell*)((funcptr)binary)(0);
}

int compile_for_platform(Cell* expr, Cell** res) {
  code = mmap(0, CODESZ, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, 0, 0);
  memset(code, 0, CODESZ);
  jit_init(0x400);

  register void* sp asm ("sp");
  Frame* empty_frame = malloc(sizeof(Frame)); // FIXME leak
  empty_frame->f=NULL;
  empty_frame->sp=0;
  empty_frame->locals=0;
  empty_frame->stack_end=sp;
  empty_frame->parent_frame=NULL;

  Cell* success = compile_expr(expr, empty_frame, prototype_any);
  jit_ret();
  char* defsym = "anon";

  if (!success) {
    printf("<compile_expr failed: %p>\r\n",success);
  }

  // disassemble
#ifdef DEBUG
  system("arm-linux-gnueabihf-objdump -D -b binary -marmv5 /tmp/test >/tmp/disasm");
  int fd = open("/tmp/disasm",O_RDONLY);
  char buf[1024];
  int buflen;
  while((buflen = read(fd, buf, 1024)) > 0)
  {
    write(1, buf, buflen);
  }
  close(fd);
#endif

  int mp_res = mprotect(code, CODESZ, PROT_EXEC|PROT_READ);

  if (!mp_res) {
    *res = execute_jitted(code);
  } else {
    printf("<mprotect result: %d\n>",mp_res);
    *res = NULL;
    success = 0;
  }
  
  //printf("pointer result: %p\n",*res);
  //printf("pointer value: %p\n",((Cell*)*res)->value);

  return !!success;
}
