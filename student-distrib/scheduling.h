/* scheduling.h - 
 * vim:ts=4 noexpandtab
 */
#include "lib.h"
#include "system_calls.h"
#include "paging.h"
#include "i8259.h"
#include "terminal.h"

void task_switcher(int32_t old_process, int32_t new_process);
void scheduler();
void check_task_list();
int32_t add_task_list_entry(int32_t process_num);
void delete_task_list_entry(int32_t process_num);
void pit_interrupt_handler(void);
void pit_init(void);
