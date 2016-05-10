#ifndef SEMAPHORE_H
#define SEMAPHORE_H

class Semaphore {
public:
  Semaphore() : m_count(0) {}
  bool locked(void) { return m_count > 0; }
  void lock_take(void) { m_count++; }
  void lock_release(void) { m_count--; }
private:
  int m_count;
};

class SemKeep {
public:
  SemKeep(Semaphore &sem) : m_sem(sem), m_taken(false) {}
  void take(void) { if (!m_taken) { m_taken = true; m_sem.lock_take(); } }
  void release(void) { if (m_taken) { m_taken = false; m_sem.lock_release(); }}
private:
  Semaphore &m_sem;
  bool m_taken;
};

#endif
