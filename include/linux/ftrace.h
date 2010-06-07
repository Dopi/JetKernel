#ifndef _LINUX_FTRACE_H
#define _LINUX_FTRACE_H

#include <linux/linkage.h>
#include <linux/fs.h>
#include <linux/ktime.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/bitops.h>
#include <linux/sched.h>

#ifdef CONFIG_FUNCTION_TRACER

extern int ftrace_enabled;
extern int
ftrace_enable_sysctl(struct ctl_table *table, int write,
		     struct file *filp, void __user *buffer, size_t *lenp,
		     loff_t *ppos);

typedef void (*ftrace_func_t)(unsigned long ip, unsigned long parent_ip);

struct ftrace_ops {
	ftrace_func_t	  func;
	struct ftrace_ops *next;
};

extern int function_trace_stop;

/*
 * Type of the current tracing.
 */
enum ftrace_tracing_type_t {
	FTRACE_TYPE_ENTER = 0, /* Hook the call of the function */
	FTRACE_TYPE_RETURN,	/* Hook the return of the function */
};

/* Current tracing type, default is FTRACE_TYPE_ENTER */
extern enum ftrace_tracing_type_t ftrace_tracing_type;

/**
 * ftrace_stop - stop function tracer.
 *
 * A quick way to stop the function tracer. Note this an on off switch,
 * it is not something that is recursive like preempt_disable.
 * This does not disable the calling of mcount, it only stops the
 * calling of functions from mcount.
 */
static inline void ftrace_stop(void)
{
	function_trace_stop = 1;
}

/**
 * ftrace_start - start the function tracer.
 *
 * This function is the inverse of ftrace_stop. This does not enable
 * the function tracing if the function tracer is disabled. This only
 * sets the function tracer flag to continue calling the functions
 * from mcount.
 */
static inline void ftrace_start(void)
{
	function_trace_stop = 0;
}

/*
 * The ftrace_ops must be a static and should also
 * be read_mostly.  These functions do modify read_mostly variables
 * so use them sparely. Never free an ftrace_op or modify the
 * next pointer after it has been registered. Even after unregistering
 * it, the next pointer may still be used internally.
 */
int register_ftrace_function(struct ftrace_ops *ops);
int unregister_ftrace_function(struct ftrace_ops *ops);
void clear_ftrace_function(void);

extern void ftrace_stub(unsigned long a0, unsigned long a1);

#else /* !CONFIG_FUNCTION_TRACER */
# define register_ftrace_function(ops) do { } while (0)
# define unregister_ftrace_function(ops) do { } while (0)
# define clear_ftrace_function(ops) do { } while (0)
static inline void ftrace_kill(void) { }
static inline void ftrace_stop(void) { }
static inline void ftrace_start(void) { }
#endif /* CONFIG_FUNCTION_TRACER */

#ifdef CONFIG_STACK_TRACER
extern int stack_tracer_enabled;
int
stack_trace_sysctl(struct ctl_table *table, int write,
		   struct file *file, void __user *buffer, size_t *lenp,
		   loff_t *ppos);
#endif

#ifdef CONFIG_DYNAMIC_FTRACE
/* asm/ftrace.h must be defined for archs supporting dynamic ftrace */
#include <asm/ftrace.h>

enum {
	FTRACE_FL_FREE		= (1 << 0),
	FTRACE_FL_FAILED	= (1 << 1),
	FTRACE_FL_FILTER	= (1 << 2),
	FTRACE_FL_ENABLED	= (1 << 3),
	FTRACE_FL_NOTRACE	= (1 << 4),
	FTRACE_FL_CONVERTED	= (1 << 5),
	FTRACE_FL_FROZEN	= (1 << 6),
};

struct dyn_ftrace {
	struct list_head	list;
	unsigned long		ip; /* address of mcount call-site */
	unsigned long		flags;
	struct dyn_arch_ftrace	arch;
};

int ftrace_force_update(void);
void ftrace_set_filter(unsigned char *buf, int len, int reset);

/* defined in arch */
extern int ftrace_ip_converted(unsigned long ip);
extern int ftrace_dyn_arch_init(void *data);
extern int ftrace_update_ftrace_func(ftrace_func_t func);
extern void ftrace_caller(void);
extern void ftrace_call(void);
extern void mcount_call(void);
#ifdef CONFIG_FUNCTION_GRAPH_TRACER
extern void ftrace_graph_caller(void);
extern int ftrace_enable_ftrace_graph_caller(void);
extern int ftrace_disable_ftrace_graph_caller(void);
#else
static inline int ftrace_enable_ftrace_graph_caller(void) { return 0; }
static inline int ftrace_disable_ftrace_graph_caller(void) { return 0; }
#endif

/**
 * ftrace_make_nop - convert code into top
 * @mod: module structure if called by module load initialization
 * @rec: the mcount call site record
 * @addr: the address that the call site should be calling
 *
 * This is a very sensitive operation and great care needs
 * to be taken by the arch.  The operation should carefully
 * read the location, check to see if what is read is indeed
 * what we expect it to be, and then on success of the compare,
 * it should write to the location.
 *
 * The code segment at @rec->ip should be a caller to @addr
 *
 * Return must be:
 *  0 on success
 *  -EFAULT on error reading the location
 *  -EINVAL on a failed compare of the contents
 *  -EPERM  on error writing to the location
 * Any other value will be considered a failure.
 */
extern int ftrace_make_nop(struct module *mod,
			   struct dyn_ftrace *rec, unsigned long addr);

/**
 * ftrace_make_call - convert a nop call site into a call to addr
 * @rec: the mcount call site record
 * @addr: the address that the call site should call
 *
 * This is a very sensitive operation and great care needs
 * to be taken by the arch.  The operation should carefully
 * read the location, check to see if what is read is indeed
 * what we expect it to be, and then on success of the compare,
 * it should write to the location.
 *
 * The code segment at @rec->ip should be a nop
 *
 * Return must be:
 *  0 on success
 *  -EFAULT on error reading the location
 *  -EINVAL on a failed compare of the contents
 *  -EPERM  on error writing to the location
 * Any other value will be considered a failure.
 */
extern int ftrace_make_call(struct dyn_ftrace *rec, unsigned long addr);


/* May be defined in arch */
extern int ftrace_arch_read_dyn_info(char *buf, int size);

extern int skip_trace(unsigned long ip);

extern void ftrace_release(void *start, unsigned long size);

extern void ftrace_disable_daemon(void);
extern void ftrace_enable_daemon(void);
#else
# define skip_trace(ip)				({ 0; })
# define ftrace_force_update()			({ 0; })
# define ftrace_set_filter(buf, len, reset)	do { } while (0)
# define ftrace_disable_daemon()		do { } while (0)
# define ftrace_enable_daemon()			do { } while (0)
static inline void ftrace_release(void *start, unsigned long size) { }
#endif /* CONFIG_DYNAMIC_FTRACE */

/* totally disable ftrace - can not re-enable after this */
void ftrace_kill(void);

static inline void tracer_disable(void)
{
#ifdef CONFIG_FUNCTION_TRACER
	ftrace_enabled = 0;
#endif
}

/*
 * Ftrace disable/restore without lock. Some synchronization mechanism
 * must be used to prevent ftrace_enabled to be changed between
 * disable/restore.
 */
static inline int __ftrace_enabled_save(void)
{
#ifdef CONFIG_FUNCTION_TRACER
	int saved_ftrace_enabled = ftrace_enabled;
	ftrace_enabled = 0;
	return saved_ftrace_enabled;
#else
	return 0;
#endif
}

static inline void __ftrace_enabled_restore(int enabled)
{
#ifdef CONFIG_FUNCTION_TRACER
	ftrace_enabled = enabled;
#endif
}

#ifdef CONFIG_FRAME_POINTER
/* TODO: need to fix this for ARM */
# define CALLER_ADDR0 ((unsigned long)__builtin_return_address(0))
# define CALLER_ADDR1 ((unsigned long)__builtin_return_address(1))
# define CALLER_ADDR2 ((unsigned long)__builtin_return_address(2))
# define CALLER_ADDR3 ((unsigned long)__builtin_return_address(3))
# define CALLER_ADDR4 ((unsigned long)__builtin_return_address(4))
# define CALLER_ADDR5 ((unsigned long)__builtin_return_address(5))
# define CALLER_ADDR6 ((unsigned long)__builtin_return_address(6))
#else
# define CALLER_ADDR0 ((unsigned long)__builtin_return_address(0))
# define CALLER_ADDR1 0UL
# define CALLER_ADDR2 0UL
# define CALLER_ADDR3 0UL
# define CALLER_ADDR4 0UL
# define CALLER_ADDR5 0UL
# define CALLER_ADDR6 0UL
#endif

#ifdef CONFIG_IRQSOFF_TRACER
  extern void time_hardirqs_on(unsigned long a0, unsigned long a1);
  extern void time_hardirqs_off(unsigned long a0, unsigned long a1);
#else
# define time_hardirqs_on(a0, a1)		do { } while (0)
# define time_hardirqs_off(a0, a1)		do { } while (0)
#endif

#ifdef CONFIG_PREEMPT_TRACER
  extern void trace_preempt_on(unsigned long a0, unsigned long a1);
  extern void trace_preempt_off(unsigned long a0, unsigned long a1);
#else
# define trace_preempt_on(a0, a1)		do { } while (0)
# define trace_preempt_off(a0, a1)		do { } while (0)
#endif

#ifdef CONFIG_TRACING
extern int ftrace_dump_on_oops;

extern void tracing_start(void);
extern void tracing_stop(void);
extern void ftrace_off_permanent(void);

extern void
ftrace_special(unsigned long arg1, unsigned long arg2, unsigned long arg3);

/**
 * ftrace_printk - printf formatting in the ftrace buffer
 * @fmt: the printf format for printing
 *
 * Note: __ftrace_printk is an internal function for ftrace_printk and
 *       the @ip is passed in via the ftrace_printk macro.
 *
 * This function allows a kernel developer to debug fast path sections
 * that printk is not appropriate for. By scattering in various
 * printk like tracing in the code, a developer can quickly see
 * where problems are occurring.
 *
 * This is intended as a debugging tool for the developer only.
 * Please refrain from leaving ftrace_printks scattered around in
 * your code.
 */
# define ftrace_printk(fmt...) __ftrace_printk(_THIS_IP_, fmt)
extern int
__ftrace_printk(unsigned long ip, const char *fmt, ...)
	__attribute__ ((format (printf, 2, 3)));
extern void ftrace_dump(void);
#else
static inline void
ftrace_special(unsigned long arg1, unsigned long arg2, unsigned long arg3) { }
static inline int
ftrace_printk(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

static inline void tracing_start(void) { }
static inline void tracing_stop(void) { }
static inline void ftrace_off_permanent(void) { }
static inline int
ftrace_printk(const char *fmt, ...)
{
	return 0;
}
static inline void ftrace_dump(void) { }
#endif

#ifdef CONFIG_FTRACE_MCOUNT_RECORD
extern void ftrace_init(void);
extern void ftrace_init_module(struct module *mod,
			       unsigned long *start, unsigned long *end);
#else
static inline void ftrace_init(void) { }
static inline void
ftrace_init_module(struct module *mod,
		   unsigned long *start, unsigned long *end) { }
#endif

enum {
	POWER_NONE = 0,
	POWER_CSTATE = 1,
	POWER_PSTATE = 2,
};

struct power_trace {
#ifdef CONFIG_POWER_TRACER
	ktime_t			stamp;
	ktime_t			end;
	int			type;
	int			state;
#endif
};

#ifdef CONFIG_POWER_TRACER
extern void trace_power_start(struct power_trace *it, unsigned int type,
					unsigned int state);
extern void trace_power_mark(struct power_trace *it, unsigned int type,
					unsigned int state);
extern void trace_power_end(struct power_trace *it);
#else
static inline void trace_power_start(struct power_trace *it, unsigned int type,
					unsigned int state) { }
static inline void trace_power_mark(struct power_trace *it, unsigned int type,
					unsigned int state) { }
static inline void trace_power_end(struct power_trace *it) { }
#endif


/*
 * Structure that defines an entry function trace.
 */
struct ftrace_graph_ent {
	unsigned long func; /* Current function */
	int depth;
};

/*
 * Structure that defines a return function trace.
 */
struct ftrace_graph_ret {
	unsigned long func; /* Current function */
	unsigned long long calltime;
	unsigned long long rettime;
	/* Number of functions that overran the depth limit for current task */
	unsigned long overrun;
	int depth;
};

#ifdef CONFIG_FUNCTION_GRAPH_TRACER

/*
 * Sometimes we don't want to trace a function with the function
 * graph tracer but we want them to keep traced by the usual function
 * tracer if the function graph tracer is not configured.
 */
#define __notrace_funcgraph		notrace

/*
 * We want to which function is an entrypoint of a hardirq.
 * That will help us to put a signal on output.
 */
#define __irq_entry		 __attribute__((__section__(".irqentry.text")))

/* Limits of hardirq entrypoints */
extern char __irqentry_text_start[];
extern char __irqentry_text_end[];

#define FTRACE_RETFUNC_DEPTH 50
#define FTRACE_RETSTACK_ALLOC_SIZE 32
/* Type of the callback handlers for tracing function graph*/
typedef void (*trace_func_graph_ret_t)(struct ftrace_graph_ret *); /* return */
typedef int (*trace_func_graph_ent_t)(struct ftrace_graph_ent *); /* entry */

extern int register_ftrace_graph(trace_func_graph_ret_t retfunc,
				trace_func_graph_ent_t entryfunc);

extern void ftrace_graph_stop(void);

/* The current handlers in use */
extern trace_func_graph_ret_t ftrace_graph_return;
extern trace_func_graph_ent_t ftrace_graph_entry;

extern void unregister_ftrace_graph(void);

extern void ftrace_graph_init_task(struct task_struct *t);
extern void ftrace_graph_exit_task(struct task_struct *t);

static inline int task_curr_ret_stack(struct task_struct *t)
{
	return t->curr_ret_stack;
}

static inline void pause_graph_tracing(void)
{
	atomic_inc(&current->tracing_graph_pause);
}

static inline void unpause_graph_tracing(void)
{
	atomic_dec(&current->tracing_graph_pause);
}
#else

#define __notrace_funcgraph
#define __irq_entry

static inline void ftrace_graph_init_task(struct task_struct *t) { }
static inline void ftrace_graph_exit_task(struct task_struct *t) { }

static inline int task_curr_ret_stack(struct task_struct *tsk)
{
	return -1;
}

static inline void pause_graph_tracing(void) { }
static inline void unpause_graph_tracing(void) { }
#endif

#ifdef CONFIG_TRACING
#include <linux/sched.h>

/* flags for current->trace */
enum {
	TSK_TRACE_FL_TRACE_BIT	= 0,
	TSK_TRACE_FL_GRAPH_BIT	= 1,
};
enum {
	TSK_TRACE_FL_TRACE	= 1 << TSK_TRACE_FL_TRACE_BIT,
	TSK_TRACE_FL_GRAPH	= 1 << TSK_TRACE_FL_GRAPH_BIT,
};

static inline void set_tsk_trace_trace(struct task_struct *tsk)
{
	set_bit(TSK_TRACE_FL_TRACE_BIT, &tsk->trace);
}

static inline void clear_tsk_trace_trace(struct task_struct *tsk)
{
	clear_bit(TSK_TRACE_FL_TRACE_BIT, &tsk->trace);
}

static inline int test_tsk_trace_trace(struct task_struct *tsk)
{
	return tsk->trace & TSK_TRACE_FL_TRACE;
}

static inline void set_tsk_trace_graph(struct task_struct *tsk)
{
	set_bit(TSK_TRACE_FL_GRAPH_BIT, &tsk->trace);
}

static inline void clear_tsk_trace_graph(struct task_struct *tsk)
{
	clear_bit(TSK_TRACE_FL_GRAPH_BIT, &tsk->trace);
}

static inline int test_tsk_trace_graph(struct task_struct *tsk)
{
	return tsk->trace & TSK_TRACE_FL_GRAPH;
}

#endif /* CONFIG_TRACING */

#endif /* _LINUX_FTRACE_H */
