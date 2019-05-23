#pragma once
#include "TypeSet.h"

#include <variant>
#include <optional>

namespace VariantValue {
	using namespace TypeSet;

	template<class T, T val>
	struct Constant { using type = T; constexpr static T value = val; };
	template<int i>
	using I = Constant<int, i>;
	template<class T>
	struct ConstantToValue {
		template<T val>
		constexpr T operator()(Constant<T, val>) const { return val; }
	};

	// Variant Value
	template<class S>
	class Value;
	template<class... Cs>
	class Value<Set<Cs...>> : public std::variant<Cs...> {
		using Base = std::variant<Cs...>;
	public:
		template<class T, class = std::enable_if_t<std::is_same_v<Set<T>, Set<Cs...>>>>
		constexpr Value(T) : Base(T{}) {}
		template<class T, T value>
		constexpr Value(Constant<T, value>) : Base(Constant<T, value>{}) {
			static_assert(Contains_v<Constant<T, value>, Set<Cs...>>, "Constructing Value with invalid Constant");
		}
		template<class... Ts, class = std::enable_if_t<is_super_set(Set<Cs...>(), Set<Ts...>())>>
		Value(Value<Set<Ts...>> v)
			: Base(std::visit([](auto x) constexpr{ return Base(x); }, v))
		{
			static_assert(is_super_set(Set<Cs...>(), Set<Ts...>()));
		}
		constexpr Value(const Value& v) = default;
		constexpr Value(Value&& v) = default;
		constexpr Value& operator=(const Value& v) = default;
		constexpr Value& operator=(Value&& v) = default;

		template< class T, class = std::enable_if_t< ((std::is_same_v<T, typename Cs::type>) && ...) > >
		operator T() const {
			return std::visit(ConstantToValue<T>(), *this);
		}
	};
	// Deduction guides
	template<class T, T value>
	Value(Constant<T, value>)->Value<Set<Constant<T, value>>>;
}

namespace std {
	// std::common_type<> is explicitly allowed to be specialized for user-defined types
	template<class...T1s, class... T2s>
	struct common_type<
		VariantValue::Value<TypeSet::Set<T1s...>>,
		VariantValue::Value<TypeSet::Set<T2s...>>
	>
	{
		using type = typename VariantValue::Value<
			TypeSet::Union_t<TypeSet::Set<T1s...>, TypeSet::Set<T2s...>>
		>;
	};
}

namespace VariantValue {
	static_assert(std::is_same_v<
		Value<Set<I<1>, I<2>>>,
		std::common_type_t< Value<Set<I<1>>>, Value<Set<I<2>>> >
	>, "Custom overload of std::common_type is broken");

	template<class S1, class S2>
	constexpr auto select(bool selectLeft, Value<S1> left, Value<S2> right) {
		using ReturnType = std::common_type_t<Value<S1>, Value<S2>>;
		return selectLeft ? ReturnType(left) : ReturnType(right);
	}
	namespace detail {
		template<class T, class... Funcs>
		struct SelectFunc;
		template<class T, class Func>
		struct SelectFunc<T, Func> {
			using F = Func;
			const F* _pf;
			constexpr SelectFunc(const Func& f) : _pf(&f) {}
		};
		template<class T, class Func, class... Funcs>
		struct SelectFunc<T, Func, Funcs...> {
			constexpr static bool use_func = std::is_invocable_v< Func, Value< Set<T> > >;
			using F = std::conditional_t< use_func, Func, typename SelectFunc<T, Funcs...>::F >;
			const F* _pf;
			SelectFunc(const Func& f1, const Funcs& ... fs) {
				if constexpr (use_func) { _pf = &f1; }
				else { _pf = SelectFunc<T, Funcs...>(fs...)._pf; }
			}
		};
		template<class T, class... Funcs> using SelectFunc_F = typename SelectFunc<T, Funcs...>::F;
	}
	template<class... Ts, class... Funcs>
	auto visit(Value<Set<Ts...>> x, Funcs... fs) {
		using ReturnType = std::common_type_t<std::invoke_result_t< detail::SelectFunc_F<Ts, Funcs...>, Value<Set<Ts>>>...>;
		std::optional<ReturnType> ret = std::nullopt;
		std::initializer_list<int>({ (
			std::holds_alternative<Ts>(x) ? (ret = (*detail::SelectFunc<Ts, Funcs...>(fs...)._pf)(Value<Set<Ts>>(std::get<Ts>(x)))), 0 : 0
		) ... });
		return *ret;
	}

	namespace detail {
		template<class RetType, class T1, class... T2s>
		constexpr inline auto make_pair_inner(Value<Set<T2s...>> x2) {
			std::optional<RetType> ret;
			std::initializer_list<int>({ (
				std::holds_alternative<T2s>(x2) ? (ret = std::optional(Value<Set<Pair<T1, T2s>>>(Pair<T1, T2s>()))), 0 : 0
			) ... });
			return *ret;
		}
	}
	template<class... T1s, class... T2s>
	constexpr auto make_pair(Value<Set<T1s...>> x1, Value<Set<T2s...>> x2) {
		using ReturnType = Value<Cartesian_t<Set<T1s...>, Set<T2s...>>>;
		std::optional<ReturnType> ret = std::nullopt;
		std::initializer_list<int>({ (
			std::holds_alternative<T1s>(x1) ?
				(ret = std::optional(detail::make_pair_inner<ReturnType, T1s, T2s...>(x2))), 0 : 0
		) ... });
		return *ret;
	}

	namespace detail {
		struct Adder {
			template<int I1, int I2>
			constexpr auto operator()(Value<Set<Pair<I<I1>, I<I2>>>>) const -> Value<Set<I<I1+I2>>> { return Value(I<I1 + I2>()); }
		};
		struct Subtracter {
			template<int I1, int I2>
			constexpr auto operator()(Value<Set<Pair<I<I1>, I<I2>>>>) const -> Value<Set<I<I1 - I2>>> { return Value(I<I1 - I2>()); }
		};
		struct Multiplier {
			template<int I1, int I2>
			constexpr auto operator()(Value<Set<Pair<I<I1>, I<I2>>>>) const -> Value<Set<I<I1 * I2>>> { return Value(I<I1 * I2>()); }
		};
		struct Divider {
			template<int I1, int I2>
			constexpr auto operator()(Value<Set<Pair<I<I1>, I<I2>>>>) const -> Value<Set<I<I1 / I2>>> { return Value(I<I1 / I2>()); }
		};
	}
	template<class V1, class V2>
	constexpr auto operator+(V1 v1, V2 v2) { return visit(make_pair(v1, v2), detail::Adder()); }
	template<class V1, class V2>
	constexpr auto operator-(V1 v1, V2 v2) { return visit(make_pair(v1, v2), detail::Subtracter()); }
	template<class V1, class V2>
	constexpr auto operator*(V1 v1, V2 v2) { return visit(make_pair(v1, v2), detail::Multiplier()); }
	template<class V1, class V2>
	constexpr auto operator/(V1 v1, V2 v2) { return visit(make_pair(v1, v2), detail::Divider()); }

	namespace detail {
		template<int Start, int PastEnd>
		struct IntegerInterval {
			static_assert(Start<PastEnd, "Invalid interval");
			using type = Union_t<typename IntegerInterval<Start, PastEnd - 1>::type, Set<I<PastEnd - 1>>>;
		};
		template<int Start>
		struct IntegerInterval<Start, Start> { using type = Set<>; };
		template<int Start, int End>
		using IntegerInterval_t = typename IntegerInterval<Start, End>::type;
	}
	template<int Start, int End>
	using ValueInterval = Value<detail::IntegerInterval_t<Start, End+1>>;
}