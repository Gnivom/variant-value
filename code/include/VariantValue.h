#pragma once
#include "TypeSet.h"

#include <array>
#include <functional>

namespace VariantValue {
	using namespace TypeSet;

	template<class T, T val>
	struct Constant { using type = T; constexpr static T value = val; };
	template<int i>
	using I = Constant<int, i>;
	using False = Constant<bool, false>;
	using True = Constant<bool, true>;
	template<class T>
	struct ConstantToValue {
		template<T val>
		constexpr T operator()(Constant<T, val>) const { return val; }
	};

	struct ValueIndex { int _index; ValueIndex(int i) : _index(i) {} };

	// Variant Value
	template<class S>
	class Value;
	template<class... Cs>
	class Value<Set<Cs...>> {
		int _index; // Which type is currently held? Zero-indexed based on Cs...
	public:
		constexpr Value(ValueIndex i) : _index(i._index) {}
		template<class T, T value>
		constexpr Value(Constant<T, value>) : _index(FindType<Constant<T, value>, Set<Cs...>>::index) {
			static_assert(Contains_v<Constant<T, value>, Set<Cs...>>, "Constructing Value with invalid Constant");
		}
		template<class T, class = std::enable_if_t<is_super_set(Set<Cs...>(), Set<T>())>>
		constexpr Value(T) : _index(FindType<T, Set<Cs...>>::index) {}
		template<class... Ts, class = std::enable_if_t<is_super_set(Set<Cs...>(), Set<Ts...>()) >>
		constexpr Value(Value<Set<Ts...>> v) : _index(-1) {
			constexpr std::array<int, sizeof...(Ts)> a = { (FindType<Ts, Set<Cs...>>::index)... };
			_index = a[v.GetIndex()];
		}
		constexpr Value(const Value& v) = default;
		constexpr Value(Value&& v) = default;
		constexpr Value& operator=(const Value& v) = default;
		constexpr Value& operator=(Value&& v) = default;

		constexpr int GetIndex() const { return _index; }
		template< class T, class = std::enable_if_t< ((std::is_same_v<T, typename Cs::type>) && ...) > >
		constexpr operator T() const {
			return visit(*this, ConstantToValue<T>());
		}
	};
	// Deduction guides
	template<class T, T value>
	Value(Constant<T, value>)->Value<Set<Constant<T, value>>>;
}

namespace std {
	// std::common_type<> is explicitly allowed to be specialized for user-defined types
	template<class... C1s, class... C2s>
	struct common_type<
		VariantValue::Value<TypeSet::Set<C1s...>>,
		VariantValue::Value<TypeSet::Set<C2s...>>
	>
	{
		using type = VariantValue::Value<
			TypeSet::Union_t<TypeSet::Set<C1s...>, TypeSet::Set<C2s...>>
		>;
	};
	// Promote Constant to Value
	template<class T, T val, class C>
	struct common_type<VariantValue::Constant<T, val>, C> :
		common_type<VariantValue::Value<TypeSet::Set< VariantValue::Constant<T, val> >>, C> {};
	template<class... C1s, class T, T val>
	struct common_type<
		VariantValue::Value<TypeSet::Set<C1s...>>,
		VariantValue::Constant<T, val>
	> : common_type<
		VariantValue::Value<TypeSet::Set<C1s...>>,
		VariantValue::Value<TypeSet::Set<VariantValue::Constant<T, val>>>
	> {};
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
		template<class T, class... Funcs>
		using SelectFunc_F = typename SelectFunc<T, Funcs...>::F;
		template<class T, class Func>
		struct SelectFunc<T, Func> {
			using F = Func;
			const F* _pf;
			constexpr SelectFunc(const Func& f) : _pf(&f) {}
		};
		template<class T, class Func, class... Funcs>
		struct SelectFunc<T, Func, Funcs...> {
			constexpr static bool use_func = std::is_invocable_v< Func, Value< Set<T> > >;
			using F = std::conditional_t< use_func, Func, SelectFunc_F<T, Funcs...> >;
			const F* _pf;
			constexpr SelectFunc(const Func& f1, const Funcs& ... fs)
				: _pf(nullptr)
			{
				if constexpr (use_func) { _pf = &f1; }
				else { _pf = SelectFunc<T, Funcs...>(fs...)._pf; }
			}
		};
	}
	template<class... Ts, class... Funcs>
	constexpr auto visit(Value<Set<Ts...>> x, Funcs... fs) {
		using ReturnType = std::common_type_t<std::invoke_result_t< detail::SelectFunc_F<Ts, Funcs...>, Ts>...>;
		typedef ReturnType(*DerivedFunc)(Funcs...);
		constexpr std::array<DerivedFunc, sizeof...(Ts)> a = {
			( [](Funcs... _fs) -> ReturnType {
				return (*detail::SelectFunc<Ts, Funcs...>(_fs...)._pf)(Ts{});
			})...
		};
		return a[x.GetIndex()](fs...);
	}

	template<class... T1s, class... T2s>
	constexpr auto make_pair(Value<Set<T1s...>> x1, Value<Set<T2s...>> x2) {
		using ReturnType = Value<Cartesian_t<Set<T1s...>, Set<T2s...>>>;
		return ReturnType(ValueIndex(x1.GetIndex() * sizeof...(T2s) + x2.GetIndex()));
	}

	namespace detail {
		struct Adder {
			template<int I1, int I2>
			constexpr auto operator()(Pair<I<I1>, I<I2>>) const -> I<I1+I2> { return I<I1 + I2>(); }
		};
		struct Subtracter {
			template<int I1, int I2>
			constexpr auto operator()(Pair<I<I1>, I<I2>>) const -> I<I1 - I2> { return I<I1 - I2>(); }
		};
		struct Multiplier {
			template<int I1, int I2>
			constexpr auto operator()(Pair<I<I1>, I<I2>>) const -> I<I1 * I2> { return I<I1 * I2>(); }
		};
		struct Divider {
			template<int I1, int I2>
			constexpr auto operator()(Pair<I<I1>, I<I2>>) const -> I<I1 / I2> { return I<I1 / I2>(); }
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
	using IntInterval = Value<detail::IntegerInterval_t<Start, End+1>>;
}