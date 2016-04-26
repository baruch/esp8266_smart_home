#ifndef COOPERATIVE_THREAD_H
#define COOPERATIVE_THREAD_H

extern "C" {
#include "cont.h"
}

class CoopThread {
  public:
    CoopThread();

    /* Runs a cycle of the thread until the thread goes to sleep or exits. */
    void thread_run(void);

    /* Should only be used inside the user thread to put the thread to sleep. */
    void thread_sleep(unsigned long msec);

    /* Yield for a single iteration, equivalent to thread_sleep(0) */
    void thread_yield(void) { thread_sleep(0); }

    /* Suspend a thread, useful for cases where we need to wait for an
     * interrupt or callback to wake us up, use thread_resume() in that
     * interrupt or callback.
     */
    void thread_suspend(void);

    /* Wakeup a thread that was previously suspended. It will not immediately run the thread, this will happen on the next call to thread_run. */
    void thread_resume(void);

  protected:
    /* This is the user implemented thread function, it will be called once
     * thread_run is called and then resumed from the same spot inside it when
     * it wakes up from a sleep.
     * 
     * NOTE: Do not call this function directly, it will break the flow and things will go bad quickly.
     */
    virtual void user_thread(void) = 0;
    friend void thread_run_trampoline(void);

  private:
    cont_t cont;
    unsigned long m_sleep_to;
    bool m_suspend;
};

#endif
