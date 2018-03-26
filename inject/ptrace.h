#ifdef ARM
	#define REG_TYPE user_regs
#else
	#define REG_TYPE user_regs_struct
#endif

int ptrace_attach(pid_t target);
int ptrace_detach(pid_t target);
int ptrace_getregs(pid_t target, struct REG_TYPE* regs);
int ptrace_cont(pid_t target);
int ptrace_setregs(pid_t target, struct REG_TYPE* regs);
siginfo_t ptrace_getsiginfo(pid_t target);
int ptrace_read(int pid, unsigned long addr, void *vptr, size_t len);
int ptrace_write(int pid, unsigned long addr, void *vptr, size_t len);
int checktargetsig(int pid);
int restoreStateAndDetach(pid_t target, unsigned long addr, void* backup, size_t datasize, struct REG_TYPE oldregs);
