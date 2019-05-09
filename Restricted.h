#pragma once

#include "IntList.h"

namespace Restricted {
	using namespace IntList;

	struct Axiom {};
	template<class intList> class RestrictedInteger;
	template<int... Is>
	class RestrictedInteger<IL<Is...>> {
		int _i;

	public:
		using intList = IL<Is...>;

		RestrictedInteger(Axiom, int i) : _i(i) {}

		template <int I>
		constexpr RestrictedInteger(std::integral_constant<int, I>) : _i(I) {
			static_assert(((I == Is) || ...), "Invalid value");
		}
		template<int... Js, typename = std::enable_if_t< is_super_set(intList(), IL<Js...>()), void>>
		constexpr RestrictedInteger(const RestrictedInteger<IL<Js...>>& other) : _i(int(other)) {
			static_assert(is_super_set(intList(), IL<Js...>()), "Invalid restriction conversion");
		}
		template<class F, class intList1, class intList2>
		constexpr RestrictedInteger(F, RestrictedInteger<intList1> x, RestrictedInteger<intList2> y) : _i(F::f(int(x), int(y))) {
			using Candidates = Cleanup_t<Papply<F, Cartesian_t<intList1, intList2>>>;
			static_assert(is_super_set(intList(), Candidates()), "In");
		}
		template<int... Js>
		RestrictedInteger& operator=(const RestrictedInteger<IL<Js...>> & other) {
			static_assert(is_super_set(intList(), IL<Js...>()), "Invalid restriction conversion");
			_i = int{ other };
			return *this;
		}

		constexpr operator int() const { return _i; }
	};
}

namespace std {
	// std::common_type<> is explicitly allowed to be specialized for user-defined types
	template<int...Is, int... Js>
	struct common_type<
		Restricted::RestrictedInteger<Restricted::IL<Is...>>,
		Restricted::RestrictedInteger<Restricted::IL<Js...>>
	>
	{
		using type = typename Restricted::RestrictedInteger<
			Restricted::Cleanup_t< Restricted::Concat_t<Restricted::IL<Is...>, Restricted::IL<Js...>> >
		>;
	};
}

namespace Restricted {
#define RESTRICTED_OPERATOR(op) \
	template<class intList1, class intList2> \
	constexpr auto operator op(RestrictedInteger<intList1> X1, RestrictedInteger<intList2> X2) { \
		struct F { constexpr static int f(int x, int y) { return x op y; } }; \
		using Candidates = Cleanup_t<Papply<F, Cartesian_t<intList1, intList2>>>; \
		return RestrictedInteger<Candidates>(F(), X1, X2); \
	}
	RESTRICTED_OPERATOR(+);
	RESTRICTED_OPERATOR(-);
	RESTRICTED_OPERATOR(*);
	RESTRICTED_OPERATOR(/ );
#undef RESTRICTED_OPERATOR

	template<int I> using C = std::integral_constant<int, I>;
	template<int I, int... Is, typename T = C<I>>
	constexpr auto Restrict(T value = T{}) {
		return RestrictedInteger<IL<I, Is...>>(value);
	}
	template<int Start, int End>
	using RestrictSpan = RestrictedInteger<Span_t<Start, End>>;

	template<class leftList, class rightList>
	constexpr auto join(bool selectLeft, RestrictedInteger<leftList> left, RestrictedInteger<rightList> right) {
		using ReturnType = std::common_type_t<RestrictedInteger<leftList>, RestrictedInteger<rightList>>;
		return selectLeft ? ReturnType(left) : ReturnType(right);
	}

	template<int I, class... Funcs>
	struct SelectFunc;
	template<int I, class Func>
	struct SelectFunc<I, Func> { 
		using F = Func;
		const F* _pf;
		constexpr SelectFunc(const Func& f) : _pf(&f) {}
	};
	template<int I, class Func, class... Funcs>
	struct SelectFunc<I, Func, Funcs...> {
		constexpr static bool use_func = std::is_invocable_v< Func, RestrictedInteger< IL<I> > >;
		using F = std::conditional_t< use_func, Func, typename SelectFunc<I, Funcs...>::F >;
		const F* _pf;
		SelectFunc(const Func& f1, const Funcs&... fs) {
			if constexpr (use_func) { _pf = &f1; }
			else { _pf = SelectFunc<I, Funcs...>(fs...)._pf; }
		}
	};
	template<int I, class... Funcs> using SelectFunc_F = typename SelectFunc<I, Funcs...>::F;

	template<int... Is, class... Funcs>
	auto split(RestrictedInteger<IL<Is...>> x, Funcs... fs) {
		constexpr Axiom axiom;
		using ReturnType = std::common_type_t<std::invoke_result_t< SelectFunc_F<Is, Funcs...> , RestrictedInteger<IL<Is>>>...>;
		ReturnType ret(axiom, 0);
		std::initializer_list<int>({ (
			(int{x} == Is) ? (ret = (*SelectFunc<Is, Funcs...>(fs...)._pf)(RestrictedInteger<IL<Is>>(axiom, int{ x }))), 0 : 0
		) ... });
		return ret;
	}
}