#define DEBUG

#ifdef DEBUG
#define dprintk printk
#else
#define dprintk(...) {}
#endif

#define check_ret_value(what, to_ret) { \
	if (_ret < 0) { \
		printk(KERN_INFO "%s failed: %d", what, _ret); \
		return to_ret; \
	} else { \
		dprintk(KERN_INFO "%s returned: %d", what, _ret); \
	} \
}

