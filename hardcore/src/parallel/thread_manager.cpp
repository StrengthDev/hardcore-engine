#include <pch.hpp>

#include <parallel/thread_manager.hpp>
#include <parallel/task.hpp>

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
		task_queue immediate_tasks;
		task_queue background_tasks;

		typedef std::uint32_t thread_idx_t;

		thread_idx_t n_immediate_workers = 1;
		std::thread* immediate_workers;
		task_queue* immediate_queues;

		thread_idx_t n_background_workers = 1;
		std::thread* background_workers;
		task_queue* background_queues;

		void master()
		{
			LOG_INTERNAL_INFO("Lauched master thread (ID: " << std::this_thread::get_id() << ")");

			task_queue::task_t task;
			while (run_master.test_and_set())
			{
				if (immediate_tasks.try_pop(task))
				{
					immediate_queues[0].push(std::move(task));
				}
				if (background_tasks.try_pop(task))
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

			LOG_INTERNAL_INFO("Master thread exiting (ID: " << std::this_thread::get_id() << ")");
		}

		void worker(task_queue& queue)
		{
			LOG_INTERNAL_INFO("Lauched worker thread (ID: " << std::this_thread::get_id() << ")");
			
			try
			{
				while (true)
				{
					task_queue::task_t task = queue.pop();
					task();
				}
			}
			catch (const closed_queue&)
			{ }

			queue.~task_queue();

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

			thread_idx_t i;
			immediate_workers = t_malloc<std::thread>(n_immediate_workers);
			immediate_queues = t_malloc<task_queue>(n_immediate_workers);
			for (i = 0; i < n_immediate_workers; i++)
			{
				new (&immediate_queues[i]) task_queue();
				new (&immediate_workers[i]) std::thread(worker, std::ref(immediate_queues[i]));
			}

			background_workers = t_malloc<std::thread>(n_background_workers);
			background_queues = t_malloc<task_queue>(n_background_workers);
			for (i = 0; i < n_background_workers; i++)
			{
				new (&background_queues[i]) task_queue();
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

		void submit_immediate_task(std::function<void()> task)
		{
			std::lock_guard<std::mutex> lock(task_signal_access);
			immediate_tasks.push(std::move(task));
			task_signal.notify_all();
		}

		void submit_background_task(std::function<void()> task)
		{
			std::lock_guard<std::mutex> lock(task_signal_access);
			background_tasks.push(std::move(task));
			task_signal.notify_all();
		}
	}
}
