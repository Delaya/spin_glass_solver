#ifndef __CUSTOM_SCHEDULER_H__
#define __CUSTOM_SCHEDULER_H__

#include <hpx/hpx.hpp>
//
#include <hpx/runtime/threads/detail/thread_pool.hpp>
#include <hpx/runtime/threads/detail/create_thread.hpp>
#include <hpx/runtime/threads/detail/create_work.hpp>
//
#include <hpx/runtime/threads/policies/callback_notifier.hpp>
//
#include <hpx/runtime/threads/policies/static_queue_scheduler.hpp>
//
#undef RDMAHELPER_DISABLE_LOGGING
#include "RdmaLogging.h"
//
using namespace hpx;
using namespace hpx::threads::policies;
//
class custom_scheduler {
public:
//    typedef static_queue_scheduler<
//                boost::mutex, lockfree_fifo, lockfree_fifo, lockfree_lifo>
//                scheduling_policy_type;
    typedef static_queue_scheduler<> scheduling_policy_type;

    // The thread pool
    hpx::threads::detail::thread_pool<scheduling_policy_type> pool_;
    // The scheduler
    scheduling_policy_type scheduler_;
    // Callback notifier (?)
    callback_notifier notifier_;
    //
    scheduling_policy_type::init_parameter init_param_;

    custom_scheduler() :
        init_param_(),
        scheduler_(init_param_),
        pool_(scheduler_, notifier_, "custom-pool")
    {
        LOG_DEBUG_MSG("Creating custom scheduler");
        std::cout << "Creating custom scheduler\n";
    }

    static inline threads::thread_state_enum thread_function_nullary(
        util::unique_function_nonser<void()> func)
    {
        // execute the actual thread function
        func();

        // Verify that there are no more registered locks for this
        // OS-thread. This will throw if there are still any locks
        // held.
        util::force_error_on_lock();

        return threads::terminated;
    }


    hpx::threads::thread_id_type register_thread_nullary(
        util::unique_function_nonser<void()> && func, char const* desc,
        threads::thread_state_enum initial_state, bool run_now,
        threads::thread_priority priority, std::size_t os_thread,
        threads::thread_stacksize stacksize, error_code& ec)
    {
        std::cout << "Here 1 in register_thread_nullary" << std::endl;
        LOG_DEBUG_MSG("Here 1 in register_thread_nullary");

        hpx::applier::applier* app = hpx::applier::get_applier_ptr();
        if (NULL == app)
        {
            HPX_THROWS_IF(ec, invalid_status,
                    "hpx::applier::register_thread_nullary",
                    "global applier object is not accessible");
            return threads::invalid_thread_id;
        }

        std::cout << "Here 2 in register_thread_nullary" << std::endl;
        LOG_DEBUG_MSG("Here 2 in register_thread_nullary");

        hpx::threads::thread_init_data data(
                util::bind(util::one_shot(&thread_function_nullary), std::move(func)),
                desc ? desc : "<unknown>", 0, priority, os_thread,
                        threads::get_stack_size(stacksize));

        std::cout << "Here 3 in register_thread_nullary" << std::endl;
        LOG_DEBUG_MSG("Here 3 in register_thread_nullary");

        hpx::threads::thread_id_type id = threads::invalid_thread_id;
        hpx::threads::detail::create_work(&scheduler_, data, initial_state, ec);

        std::cout << "Here 4 in register_thread_nullary" << std::endl;
        LOG_DEBUG_MSG("Here 4 in register_thread_nullary");

        //        policy_.create_thread(data, id, initial_state,  run_now, ec, data.num_os_thread);

        std::cout << "created a thread and got id " << id << std::endl;
        return id;
    }

};

#endif
