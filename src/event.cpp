#include <coop/event.hpp>

#include <atomic>
#include <coop/scheduler.hpp>
#include <cstddef>
#include <err.h>
#include <sys/event.h>
#include <utility>

#if defined(_WIN32)
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#elif defined(__linux__)
#    include <pthread.h>
#elif (__APPLE__)
#endif

using namespace coop;

event_ref_t::wait_result_t
event_ref_t::wait_many(event_ref_t* events, uint32_t count)
{
#if defined(_WIN32)
    static_assert(sizeof(event_ref_t) == sizeof(HANDLE));
    uint32_t result = WaitForMultipleObjects(
        count, reinterpret_cast<HANDLE*>(events), false, INFINITE);

    assert(result != WAIT_FAILED && "Failed to await events");

    if (result == WAIT_FAILED)
    {
        return {status_e::failed};
    }
    else if (result == WAIT_TIMEOUT)
    {
        return {status_e::timeout};
    }

    if (result < WAIT_ABANDONED_0)
    {
        return {status_e::normal, result - WAIT_OBJECT_0};
    }
    else
    {
        return {status_e::abandoned, result - WAIT_OBJECT_0};
    }

#elif defined(__linux__)
    // TODO: Android/Linux implementation
#elif (__APPLE__)
    // TODO: MacOS/iOS implementation

    // dont need since kqeueue manages event multiplexing directly in the
    // scheduler loop

    // kqueue remembers all of the events registered through init (using ident
    // to id them), and its always tracked by kqueue,
#endif
}

void event_ref_t::init(bool manual_reset, char const* label)
{
#if defined(_WIN32)
    handle_ = CreateEventA(nullptr, manual_reset, false, label);
#elif defined(__linux__)
    // TODO: Android/Linux implementation
#elif (__APPLE__)
    // TODO: MacOS/iOS implementation

    struct kevent sev;

    event_state_t* state = new event_state_t{.manual_reset = manual_reset};
    // taking the address of the pointer
    uintptr_t ident = reinterpret_cast<uintptr_t>(state);
    state->ident    = ident;

    handle_ = state;
    EV_SET(&sev, state->ident, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, nullptr);

    int ret = kevent(scheduler_t::instance().kq_, &sev, 1, nullptr, 0, nullptr);

    if (ret == -1)
        err(EXIT_FAILURE, "kevent register");

#endif
}

bool event_ref_t::is_signaled() const
{
#if defined(_WIN32)
    uint32_t status = WaitForSingleObject(handle_, 0);
    return status == WAIT_OBJECT_0;
#elif defined(__linux__)
    // TODO: Android/Linux implementation
#elif (__APPLE__)
    // TODO: MacOS/iOS implementation

    event_state_t* handle_state = reinterpret_cast<event_state_t*>(handle_);

    return handle_state ? handle_state->signaled.load() : false;

#endif
}

bool event_ref_t::wait() const
{
#if defined(_WIN32)
    uint32_t status = WaitForSingleObject(handle_, INFINITE);
    return status == WAIT_OBJECT_0;
#elif defined(__linux__)
    // TODO: Android/Linux implementation
#elif (__APPLE__)
    // TODO: MacOS/iOS implementation

    event_state_t* handle_state = reinterpret_cast<event_state_t*>(handle_);

    // handler doesnt exist
    if (!handle_state)
    {
        return false;
    }

    // not signaled, want to block
    while (!handle_state->signaled.load())
    {
        handle_state->signaled.wait(false);
    }

    return true;
#endif
}

void event_ref_t::signal()
{
#if defined(_WIN32)
    SetEvent(handle_);
#elif defined(__linux__)
    // TODO: Android/Linux implementation
#elif (__APPLE__)
    // TODO: MacOS/iOS implementation
    struct kevent sev;
    int ret                     = -1;
    event_state_t* handle_state = reinterpret_cast<event_state_t*>(handle_);

    EV_SET(
        &sev, handle_state->ident, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, nullptr);

    if (handle_state && !handle_state->signaled.exchange(true))
    {
        ret = kevent(scheduler_t::instance().kq_, &sev, 1, nullptr, 0, nullptr);
        if (ret == -1)
            err(EXIT_FAILURE, "kevent register");
    }
#endif
}

void event_ref_t::reset()
{
#if defined(_WIN32)
    ResetEvent(handle_);
#elif defined(__linux__)
    // TODO: Android/Linux implementation
#elif (__APPLE__)
    // TODO: MacOS/iOS implementation

    event_state_t* handle_state = reinterpret_cast<event_state_t*>(handle_);

    handle_state->signaled.exchange(false);

#endif
}

void event_t::await_suspend(std::coroutine_handle<> coroutine) noexcept
{
    // Enqueue coroutine for resumption when this event transitions to the
    // signaled state
    scheduler_t::instance().schedule(coroutine, ref(), cpu_affinity_, priority_);
}

event_t::~event_t() noexcept
{
#if defined(_WIN32)
    if (handle_)
    {
        CloseHandle(handle_);
    }
#elif defined(__linux__)
    // TODO: Android/Linux implementation
#elif (__APPLE__)
    // TODO: MacOS/iOS implementation

    event_state_t* handle_state = reinterpret_cast<event_state_t*>(handle_);

    if (handle_state)
    {
        delete handle_state;
        handle_state = nullptr;
    }

#endif
}

event_t::event_t(event_t&& other) noexcept
{
    *this = std::move(other);
}

event_t& event_t::operator=(event_t&& other) noexcept
{
    if (this != &other)
    {
#if defined(_WIN32)
        std::swap(handle_, other.handle_);
#elif defined(__linux__)
        // TODO: Android/Linux implementation
#elif (__APPLE__)
        // TODO: MacOS/iOS implementation

        // just swapping pointer addresses
        std::swap(handle_, other.handle_);
#endif
    }
    return *this;
}

event_ref_t event_t::ref() const noexcept
{
#if defined(_WIN32) || (__APPLE__)
    event_ref_t out;
    out.handle_ = handle_;
    return out;
#elif defined(__linux__)
    // TODO: Android/Linux implementation
#endif
}
