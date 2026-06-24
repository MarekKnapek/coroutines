#include <cassert> /* assert */
#include <coroutine> /* std::coroutine_handle std::suspend_always */
#include <cstddef> /* std::max_align_t */
#include <exception> /* std::terminate */
#include <new> /* std::nothrow_t */
#include <utility> /* std::move std::forward */

namespace mk
{
	namespace lib
	{

		template<typename t>
		[[nodiscard]] constexpr t round_up(t const& value, t const& align) noexcept
		{
			return ((value + (align - 1)) / align) * align;
		}

		template<std::size_t n>
		struct storage_t
		{
			alignas(alignof(max_align_t)) unsigned char m_buf[n];
		};

		template<std::size_t n>
		struct allocator_t
		{
			std::size_t m_offset;
			storage_t<n> m_storage;
			[[nodiscard]] void* allocate(std::size_t const& len) noexcept
			{
				std::size_t amount;
				unsigned char* ptr;
				void* ret;
				assert(len <= n);
				assert(len <= n - m_offset);
				amount = round_up(len, alignof(std::max_align_t));
				assert(amount <= n - m_offset);
				ptr = &m_storage.m_buf[m_offset];
				m_offset += amount;
				ret = ptr;
				return ret;
			}
			void deallocate(void* const& ptr, std::size_t const& len) noexcept
			{
				((void)(ptr));
				((void)(len));
			}
		};

		static allocator_t<1 * 1024> g_allocator;

		template<typename t>
		class generator
		{
		private:
			generator(void) noexcept = delete;
			generator(mk::lib::generator<t> const&) noexcept = delete;
			mk::lib::generator<t>& operator=(mk::lib::generator<t> const&) noexcept = delete;
			template<typename u>
			generator(u&& coroutine_handle) :
				m_coroutine_handle(std::forward<u>(coroutine_handle))
			{
			}
		public:
			generator(mk::lib::generator<t>&& other) noexcept :
				m_coroutine_handle(std::move(other.m_coroutine_handle))
			{
				other.m_coroutine_handle = nullptr;
			}
			mk::lib::generator<t>& operator=(mk::lib::generator<t>&& other) noexcept
			{
				m_coroutine_handle = std::move(other.m_coroutine_handle); other.m_coroutine_handle = nullptr;
				return *this;
			}
			~generator() noexcept
			{
				if(m_coroutine_handle)
				{
					m_coroutine_handle.destroy();
				}
			}
		public:
			[[nodiscard]] bool run_a_bit(void) noexcept
			{
				bool is_runnable;
				if(m_coroutine_handle)
				{
					m_coroutine_handle.resume();
					is_runnable = !m_coroutine_handle.done();
				}
				else
				{
					is_runnable = false;
				}
				return is_runnable;
			}
			[[nodiscard]] bool is_runnable(void) noexcept
			{
				bool is_runnable;
				if(m_coroutine_handle)
				{
					is_runnable = !m_coroutine_handle.done();
				}
				else
				{
					is_runnable = false;
				}
				return is_runnable;
			}
			[[nodiscard]] t const& get_current_value(void) const& noexcept
			{
				return m_coroutine_handle.promise().getter();
			}
			[[nodiscard]] t&& get_current_value(void)&& noexcept
			{
				return std::move(m_coroutine_handle.promise()).getter();
			}
		public:
			class promise_type
			{
			private:
				promise_type(mk::lib::generator<t>::promise_type const&) noexcept = delete;
				mk::lib::generator<t>::promise_type& operator=(mk::lib::generator<t>::promise_type const&) noexcept = delete;
			public:
				template<typename... ts>
				[[nodiscard]] static void* operator new(std::size_t const len, ts&&... args) noexcept
				{
					//void* ptr = ::operator new(len, std::nothrow_t{});
					void* ptr = g_allocator.allocate(len);
					return ptr;
				}
				static void operator delete(void* const ptr, std::size_t const len) noexcept
				{
					//::operator delete(ptr, len);
					g_allocator.deallocate(ptr, len);
				}
			public:
				promise_type() noexcept :
					m_current_value()
				{
				}
				promise_type(mk::lib::generator<t>::promise_type&& other) noexcept :
					m_current_value(std::move(other.m_current_value))
				{
				}
				mk::lib::generator<t>::promise_type& operator=(mk::lib::generator<t>::promise_type&& other) noexcept
				{
					m_current_value = std::move(other.m_current_value);
					return *this;
				}
				~promise_type() noexcept
				{
				}
			public:
				[[nodiscard]] static mk::lib::generator<t> get_return_object_on_allocation_failure(void) noexcept
				{
					return mk::lib::generator<t>{nullptr};
				}
				[[nodiscard]] mk::lib::generator<t> get_return_object(void) noexcept
				{
					return mk::lib::generator<t>{std::coroutine_handle<mk::lib::generator<t>::promise_type>::from_promise(*this)};
				}
				[[nodiscard]] constexpr std::suspend_always initial_suspend(void) const noexcept
				{
					return std::suspend_always{};
				}
				[[nodiscard]] constexpr std::suspend_always final_suspend(void) const noexcept
				{
					return std::suspend_always{};
				}
				[[noreturn]] void unhandled_exception(void) const noexcept
				{
					std::terminate();
				}
				template<typename u>
				[[nodiscard]] std::suspend_always yield_value(u&& value) noexcept
				{
					m_current_value = std::forward<u>(value);
					return std::suspend_always{};
				}
				template<typename u>
				void return_value(u&& value) noexcept
				{
					m_current_value = std::forward<u>(value);
				}
				/*void return_void(void) noexcept
				{
				}*/
				[[nodiscard]] t const& getter(void) const& noexcept
				{
					return m_current_value;
				}
				[[nodiscard]] t&& getter(void)&& noexcept
				{
					return std::move(m_current_value);
				}
			private:
				t m_current_value;
			};
		private:
			std::coroutine_handle<mk::lib::generator<t>::promise_type> m_coroutine_handle;
		};

	}
}

#include <string> /* std::string std::to_string */

namespace mk
{
	namespace app
	{
		mk::lib::generator<std::string> looper(char const& filler) noexcept
		{
			for(int i = 0; i != 10; ++i)
			{
				co_yield std::string(20, filler) + std::to_string(i);
			}
			co_return "=== done ===";
		}
	}
}

#include <iostream> /* std::cout std::endl */

int main(void) noexcept
{
	auto looper_a = mk::app::looper('-');
	auto looper_b = mk::app::looper('.');
	bool is_runnable_a;
	bool is_runnable_b;
	do
	{
		is_runnable_a = looper_a.run_a_bit();
		is_runnable_b = looper_b.run_a_bit();
		auto const& value_a = looper_a.get_current_value();
		auto const& value_b = looper_b.get_current_value();
		std::cout << value_a << std::endl;
		std::cout << value_b << std::endl;
	}while(is_runnable_a && is_runnable_b);
}
