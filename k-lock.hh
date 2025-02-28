#ifndef CHICKADEE_K_LOCK_HH
#define CHICKADEE_K_LOCK_HH
#include <atomic>
#include <utility>
#include "x86-64.h"
inline void adjust_this_cpu_spinlock_depth(int delta);
#define LOCK_DEBUG_PAUSE 1


struct irqstate {
    irqstate()
        : flags_(0) {
    }
    irqstate(irqstate&& x)
        : flags_(x.flags_) {
        x.flags_ = 0;
    }
    irqstate(const irqstate&) = delete;
    irqstate& operator=(const irqstate&) = delete;
    ~irqstate() {
        assert(!flags_ && "forgot to unlock a spinlock");
    }

    static irqstate get() {
        irqstate s;
        s.flags_ = rdeflags();
        return s;
    }
    void restore() {
        if (flags_ & EFLAGS_IF) {
            sti();
        }
        flags_ = 0;
    }
    void clear() {
        flags_ = 0;
    }

    void operator=(irqstate&& s) {
        assert(flags_ == 0);
        flags_ = s.flags_;
        s.flags_ = 0;
    }

    uint64_t flags_;
};


struct spinlock {
    spinlock() {
        f_.clear();
    }

    irqstate lock() {
        irqstate irqs = irqstate::get();
        cli();
        lock_noirq();
        adjust_this_cpu_spinlock_depth(1);
        return irqs;
    }
    bool trylock(irqstate& irqs) {
        irqs = irqstate::get();
        cli();
        bool r = trylock_noirq();
        if (r) {
            adjust_this_cpu_spinlock_depth(1);
        } else {
            irqs.restore();
        }
        return r;
    }
    void unlock(irqstate& irqs) {
        adjust_this_cpu_spinlock_depth(-1);
        unlock_noirq();
        irqs.restore();
    }

    inline void debug_pause() {
#if LOCK_DEBUG_PAUSE
        pause();
#endif
    }

    void lock_noirq() {
        debug_pause();
        while (f_.test_and_set()) {
            pause();
        }
    }
    bool trylock_noirq() {
        debug_pause();
        return !f_.test_and_set();
    }
    void unlock_noirq() {
        f_.clear();
        debug_pause();
    }

    void clear() {
        f_.clear();
    }

    bool is_locked() const {
        return f_.test(std::memory_order_relaxed);
    }

private:
    std::atomic_flag f_;
};


struct spinlock_guard {
    explicit spinlock_guard(spinlock& lock)
        : lock_(lock), irqs_(lock_.lock()), locked_(true) {
    }
    ~spinlock_guard() {
        if (locked_) {
            lock_.unlock(irqs_);
        }
    }
    NO_COPY_OR_ASSIGN(spinlock_guard);

    void unlock() {
        assert(locked_);
        lock_.unlock(irqs_);
        locked_ = false;
    }
    void lock() {
        assert(!locked_);
        irqs_ = lock_.lock();
        locked_ = true;
    }

    constexpr bool is_locked() {
        return locked_;
    }


    spinlock& lock_;
    irqstate irqs_;
    bool locked_;
};


template <typename T>
struct ref_ptr {
    using element_type = T;
    using pointer = T*;

    T* e_;


    ref_ptr()
        : e_(nullptr) {
    }

    ref_ptr(T* e)
        : e_(e) {
    }

    ref_ptr(ref_ptr<T>&& x)
        : e_(x.e_) {
        x.e_ = nullptr;
    }

    ref_ptr(const ref_ptr<T>&) = delete;

    ~ref_ptr() {
        reset();
    }


    T* get() const noexcept {
        return e_;
    }

    explicit constexpr operator bool() {
        return e_ != nullptr;
    }


    T& operator*() const noexcept {
        assert(e_);
        return *e_;
    }

    T* operator->() const noexcept {
        assert(e_);
        return e_;
    }


    T* release() {
        T* e = e_;
        e_ = nullptr;
        return e;
    }

    ref_ptr<T>& operator=(ref_ptr<T>&& x) {
        if (x.e_ != e_) {
            reset(x.e_);
            x.e_ = nullptr;
        }
        return *this;
    }

    void reset(T* x = nullptr) noexcept {
        if (e_) {
            e_->decrement_reference_count();
        }
        e_ = x;
    }


    ref_ptr<T>& operator=(const ref_ptr<T>&) = delete;
};

#endif
