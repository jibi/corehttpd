#ifdef DEBUG
#define dprintk printk
#else
#define dprintk(...) {}
#endif

#define check_ret_value(what, x) { \
	if (x < 0) { \
		printk(KERN_INFO "%s failed: %d", what, x); \
		return NULL; \
	} else { \
		dprintk(KERN_INFO "%s returned: %d", what, x); \
	} \
}

