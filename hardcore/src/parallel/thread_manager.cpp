#include <pch.hpp>

#include <parallel/thread_manager.hpp>
#include <parallel/task.hpp>
#include <parallel/concurrent_queue.hpp>

#include <debug/log_internal.hpp>

namespace ENGINE_NAMESPACE
{
	namespace parallel
	{
		const unsigned int num_base_threads = 6; //main + renderer + network + logger + physics + master

		std::thread renderer; //TODO
		std::thread network; //TODO
		std::thread logger; //TODO
		std::thread physics; //TODO

		std::thread task_master;
		std::atomic_flag run_master;
		std::condition_variable task_signal;
		std::mutex task_signal_access;

		struct task
		{
			void(*aux)(void*, void*);
			void* func_ptr;
			void* promise_ptr;
		};
		
		typedef concurrent_queue<task> task_queue_t;

		task_queue_t immediate_tasks;
		task_queue_t background_tasks;

		typedef u32 thread_idx_t;

		thread_idx_t n_immediate_workers = 1;
		std::thread* immediate_workers;
		task_queue_t* immediate_queues;

		thread_idx_t n_background_workers = 1;
		std::thread* background_workers;
		task_queue_t* background_queues;

		void master()
		{
			LOG_INTERNAL_INFO("Lauched master thread (ID: " << std::this_thread::get_id() << ")");

			task t;
			while (run_master.test_and_set())
			{
				if (immediate_tasks.try_pop(t))
				{
					immediate_queues[0].push(std::move(t));
				}
				if (background_tasks.try_pop(t))
				{

				}
				std::unique_lock<std::mutex> lock(task_signal_access);
				if (!(immediate_tasks.unsafe_size() || background_tasks.unsafe_size()))
				{
					task_signal.wait(lock);
				}
			}

			thread_idx_t i;
			for (i = 0; i < n_immediate_workers; i++)
			{
				immediate_queues[i].close();
			}
			for (i = 0; i < n_background_workers; i++)
			{
				background_queues[i].close();
			}

			immediate_tasks.close();
			background_tasks.close();

			LOG_INTERNAL_INFO("Master thread exiting (ID: " << std::this_thread::get_id() << ")");
		}

		void worker(task_queue_t& queue)
		{
			LOG_INTERNAL_INFO("Lauched worker thread (ID: " << std::this_thread::get_id() << ")");
			
			try
			{
				while (true)
				{
					task t = queue.pop();
					t.aux(t.func_ptr, t.promise_ptr);
				}
			}
			catch (const exception::closed_queue&)
			{ }

			queue.~concurrent_queue();

			LOG_INTERNAL_INFO("Worker thread exiting (ID: " << std::this_thread::get_id() << ")");
		}

		void launch_threads()
		{
			unsigned int cores = std::thread::hardware_concurrency();

			if (cores > num_base_threads + 2)
			{
				unsigned int free_cores = cores - num_base_threads;
				n_immediate_workers = (free_cores / 2) + (free_cores % 2);
				n_background_workers = free_cores / 2;
			}

			LOG_INTERNAL_INFO("Maximum concurrent threads available: " << cores << " -> Launching " << n_immediate_workers << " immediate worker threads and " << n_background_workers << " background worker threads");

			run_master.test_and_set();
			task_master = std::thread(master);

			logger = std::thread(log::run);

			thread_idx_t i;
			immediate_workers = t_malloc<std::thread>(n_immediate_workers);
			immediate_queues = t_malloc<task_queue_t>(n_immediate_workers);
			for (i = 0; i < n_immediate_workers; i++)
			{
				new (&immediate_queues[i]) task_queue_t();
				new (&immediate_workers[i]) std::thread(worker, std::ref(immediate_queues[i]));
			}

			background_workers = t_malloc<std::thread>(n_background_workers);
			background_queues = t_malloc<task_queue_t>(n_background_workers);
			for (i = 0; i < n_background_workers; i++)
			{
				new (&background_queues[i]) task_queue_t();
				new (&background_workers[i]) std::thread(worker, std::ref(background_queues[i]));
			}
		}

		void terminate_threads()
		{
			task_signal_access.lock();
			run_master.clear();
			task_signal.notify_all();
			task_signal_access.unlock();
			task_master.join();

			thread_idx_t i;
			for (i = 0; i < n_immediate_workers; i++)
			{
				immediate_workers[i].join();
				immediate_workers[i].~thread();
			}
			for (i = 0; i < n_background_workers; i++)
			{
				background_workers[i].join();
				background_workers[i].~thread();
			}
			std::free(immediate_workers);
			std::free(immediate_queues);
			std::free(background_workers);
			std::free(background_queues);
		}

		void logger_wait()
		{
			logger.join();
		}

		namespace internal
		{
			void submit_immediate_task(void(*aux)(void*, void*), void* func_ptr, void* promise_ptr)
			{
				std::lock_guard<std::mutex> lock(task_signal_access);
				immediate_tasks.push({ aux, func_ptr, promise_ptr });
				task_signal.notify_all();
			}

			void submit_background_task(void(*aux)(void*, void*), void* func_ptr, void* promise_ptr)
			{
				std::lock_guard<std::mutex> lock(task_signal_access);
				background_tasks.push({ aux, func_ptr, promise_ptr });
				task_signal.notify_all();
			}
		}
	}
}
