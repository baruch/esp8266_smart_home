#include "CooperativeThread.h"
#include <Arduino.h>

#define TIME_PASSED(next) (next == 0 || ( ((long)(millis()-next)) >= 0 ))

static CoopThread *thread_to_run;

void thread_run_trampoline(void)
{
  thread_to_run->user_thread();
}

CoopThread::CoopThread()
{
  m_sleep_to = 0;
  m_suspend = false;
  cont_init(&cont);
}

void CoopThread::thread_run(void)
{
  if (m_suspend || !TIME_PASSED(m_sleep_to))
    return;

  thread_to_run = this;
  cont_run(&cont, thread_run_trampoline);
}

void CoopThread::thread_sleep(unsigned long msec)
{
  m_sleep_to = millis() + msec;
  cont_yield(&cont);
}

void CoopThread::thread_suspend(void)
{
  m_suspend = true;
}

void CoopThread::thread_resume(void)
{
  m_suspend = false;
}
