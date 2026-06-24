#include <coroutine> /* std::coroutine_handle std::suspend_always */
#include <exception> /* std::terminate */
#include <utility> /* std::move std::forward */

namespace mk
{
	namespace lib
	{
		template<typename t>
		class generator
		{
		public:
			class promise_type
			{
			public:
				promise_type() noexcept :
					m_current_value()
				{
				}
				promise_type(mk::lib::generator<t>::promise_type const&) noexcept = delete;
				promise_type(mk::lib::generator<t>::promise_type&& other) noexcept :
					m_current_value(std::move(other.m_current_value))
				{
				}
				mk::lib::generator<t>::promise_type& operator=(mk::lib::generator<t>::promise_type const&) noexcept = delete;
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
				[[nodiscard]] std::suspend_always initial_suspend(void) const noexcept
				{
					return std::suspend_always{};
				}
				[[nodiscard]] std::suspend_always final_suspend(void) const noexcept
				{
					return std::suspend_always{};
				}
				void unhandled_exception(void) const noexcept
				{
					std::terminate();
				}
				template<typename u>
				[[nodiscard]] std::suspend_always yield_value(u&& value) noexcept
				{
					m_current_value = std::forward<u>(value);
					return std::suspend_always{};
				}
				void return_void(void) noexcept
				{
					m_current_value = t{};
				}
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
