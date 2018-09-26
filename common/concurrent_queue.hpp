/****************************************************************************

Git <https://github.com/sniper00/MoonNetLua>
E-Mail <hanyongtao@live.com>
Copyright (c) 2015-2017 moon
Licensed under the MIT License <http://opensource.org/licenses/MIT>.

****************************************************************************/

#pragma once
#include <mutex>
#include <condition_variable>
#include <vector>
#include <cassert>
#include <atomic>
#include <type_traits>

namespace moon
{
	template<bool v>
	struct queue_block_empty :std::false_type {};

	template<>
	struct queue_block_empty<true> :std::true_type
	{
		std::condition_variable notempty_;
		template<typename TLock,typename TCond>
		void check(TLock& lock, TCond&& cond)
		{
			notempty_.wait(lock, std::forward<TCond>(cond));
		}

		void notify_one()
		{
			notempty_.notify_one();
		}

		void notify_all()
		{
			notempty_.notify_all();
		}
	};

	template<bool v>
	struct queue_block_full:std::false_type {};

	template<>
	struct queue_block_full<true>:std::true_type
	{
		std::condition_variable notfull_;

		template<typename TLock,typename TCond>
		void check(TLock& lock, TCond&& cond)
		{
			notfull_.wait(lock, std::forward<TCond>(cond));
		}

		void notify_one()
		{
			notfull_.notify_one();
		}

		void notify_all()
		{
			notfull_.notify_all();
		}
	};

	template<class T,typename LockType = std::mutex,template <class> class Container = std::vector, bool BlockEmpty = false,bool BlockFull = false>
	class concurrent_queue :public queue_block_empty<BlockEmpty>,public queue_block_full<BlockFull>
	{
	public:
		using container_type = Container<T>;
		using lock_t = LockType;
		using block_empty = queue_block_empty<BlockEmpty>;
		using block_full = queue_block_full<BlockFull>;

		concurrent_queue()
			:exit_(false)
			,max_size_(std::numeric_limits<size_t>::max())
		{
		}

		concurrent_queue(const concurrent_queue& t) = delete;
		concurrent_queue& operator=(const concurrent_queue& t) = delete;

		void set_max_size(size_t max_size) const
		{
			max_size_ = max_size;
		}

		template<typename TData>
		size_t push_back(TData&& x)
		{		
			std::unique_lock<lock_t> lck(mutex_);

			if constexpr (std::is_same_v< typename block_full::type, std::true_type>)
			{
				block_full::check(lck, [this] {
					return (queue_.size() < max_size_) || exit_;
				});
			}

			queue_.push_back(std::forward<TData>(x));

			if constexpr (std::is_same_v< typename block_empty::type, std::true_type>)
			{
				block_empty::notify_one();
			}
            return queue_.size();
		}

		bool try_pop(T& t)
		{
			std::unique_lock<lock_t> lck(mutex_);
			if constexpr (std::is_same_v< typename block_empty::type, std::true_type>)
			{
				block_empty::check(lck, [this] {
					return (queue_.size() > 0) || exit_;
				});
			}
			if (queue_.empty())
			{
				return false;
			}		
            t = queue_.front();
			queue_.pop_front();
			if constexpr (std::is_same_v< typename block_full::type, std::true_type>)
			{
				block_full::notify_one();
			}
			return true;
		}

		size_t size() const
		{
			std::unique_lock<lock_t> lck(mutex_);
			return queue_.size();
		}

		void  swap(container_type& other)
		{
			std::unique_lock<lock_t> lck(mutex_);
			if constexpr (std::is_same_v< typename block_empty::type, std::true_type>)
			{
				block_empty::check(lck, [this] {
					return (queue_.size() > 0) || exit_;
				});
			}
			queue_.swap(other);
			if constexpr (std::is_same_v< typename block_full::type, std::true_type>)
			{
				block_full::notify_one();
			}
		}

		void exit()
		{
            std::unique_lock<lock_t> lck(mutex_);
            exit_ = true;
			if constexpr (std::is_same_v< typename block_full::type, std::true_type>)
			{
				block_full::notify_all();
			}
			if constexpr (std::is_same_v< typename block_empty::type, std::true_type>)
			{
				block_empty::notify_all();
			}
		}

	private:
		mutable lock_t mutex_;
		container_type queue_;
		std::atomic_bool exit_;
		size_t max_size_;
	};

}
