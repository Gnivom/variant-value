#pragma once

#include <type_traits>

namespace TypeSet {
	// Set
	template<class... Ts>
	struct Set {};

	// Check is element is in set
	template<class T, class List>
	struct Contains;
	template<class T, class S, class... Ss>
	struct Contains<T, Set<S, Ss...>> {
		constexpr static bool value = std::is_same_v<T, S> || Contains<T, Set<Ss...>>::value;
	};
	template<class T>
	struct Contains<T, Set<>> {
		constexpr static bool value = false;
	};
	template<class T, class S>
	inline constexpr bool Contains_v = Contains<T, S>::value;

	// Union of sets
	template<class S1, class S2>
	struct Union;
	template<class S1, class S2>
	using Union_t = typename Union<S1, S2>::type;
	template<class... T1s, class T2, class... T2s>
	struct Union<Set<T1s...>, Set<T2, T2s...>> {
		using type = std::conditional_t<
			Contains_v<T2, Set<T1s...>>,
			Union_t<Set<T1s...>, Set<T2s...>>,
			Union_t<Set<T1s..., T2>, Set<T2s...>>
		>;
	};
	template<class... Ts>
	struct Union<Set<Ts...>, Set<>> {
		using type = Set<Ts...>;
	};
	static_assert(std::is_same_v<
		Union_t<Set<int>, Set<char>>,
		Set<int, char>
	> , "Set Union is broken");

	// Pair of types
	template<class T1, class T2>
	struct Pair { using First = T1; using Second = T2; };

	// Cartesian product
	template<class Set1, class Set2>
	struct Cartesian;
	template<class Set1, class Set2>
	using Cartesian_t = typename Cartesian<Set1, Set2>::type;
	template<class T1, class... T1s, class... T2s>
	struct Cartesian<Set<T1, T1s...>, Set<T2s...>> {
		using type = Union_t<
			Cartesian_t<Set<T1>, Set<T2s...>>,
			Cartesian_t<Set<T1s...>, Set<T2s...>>
		>;
	};
	template<class T1, class T2, class... T2s>
	struct Cartesian<Set<T1>, Set<T2, T2s...>> {
		using type = Union_t<
			Cartesian_t<Set<T1>, Set<T2>>,
			Cartesian_t<Set<T1>, Set<T2s...>>
		>;
	};
	template<class T1, class T2>
	struct Cartesian<Set<T1>, Set<T2>> {
		using type = Set<Pair<T1, T2>>;
	};
	template<class T>
	struct Cartesian<Set<T>, Set<>> {
		using type = Set<>;
	};
	template<class T>
	struct Cartesian<Set<>, Set<T>> {
		using type = Set<>;
	};
	static_assert(std::is_same_v<
		Cartesian_t<Set<int, bool>, Set<bool, char>>,
		Set<Pair<int, bool>, Pair<int, char>, Pair<bool, bool>, Pair<bool, char>>
	>, "Cartesian product broken");

	// SuperSet
	template<class... Ts>
	constexpr bool is_super_set(Set<Ts...>, Set<>) { return true; }
	template<class... T1s, class T2, class... T2s>
	constexpr bool is_super_set(Set<T1s...> s1, Set<T2, T2s...>) {
		return Contains_v<T2, Set<T1s...>> && is_super_set(s1, Set<T2s...>());
	}
	static_assert(is_super_set(Set<int, void, char, bool>(), Set<void, bool, char>()), "Sets broken");

	// Set Complement
	template<class startSet, class subtractSet> struct Complement;
	template<class startSet, class subtractSet> using Complement_t = typename Complement<startSet, subtractSet>::type;
	template<class... Ts>
	struct Complement<Set<Ts...>, Set<>> { using type = Set<Ts...>; };
	template<class T, class... Ts>
	struct Complement<Set<>, Set<T, Ts...>> { using type = Set<>; };
	template<class T1, class... T1s, class T2>
	struct Complement<Set<T1, T1s...>, Set<T2>> { using type = Union_t<Complement_t<Set<T1s...>, Set<T2>>, std::conditional_t<std::is_same_v<T1, T2>, Set<>, Set<T1>>>; };
	template<class... Ts, class TT1, class TT2, class... TTs>
	struct Complement<Set<Ts...>, Set<TT1, TT2, TTs...>> { using type = Complement_t<Complement_t<Set<Ts...>, Set<TT1>>, Set<TT2, TTs...>>; };
	static_assert(std::is_same_v<
		Complement_t<Set<int, char>, Set<void, int>>,
		Set<char>
	>, "Set Complement broken");

	// Set Intersection
	template<class S1, class S2>
	struct Intersection {
		using C1 = Complement_t<S1, S2>;
		using C2 = Complement_t<S2, S1>;
		using type = Complement_t<Union_t<S1, S2>, Union_t<C1, C2>>;
	};
	template<class list1, class list2> using Intersection_t = typename Intersection<list1, list2>::type;
	static_assert(std::is_same_v<
		Intersection_t<Set<bool, int, char>, Set<char, bool, void>>,
		Set<bool, char>
	>, "Set Intersection broken");


	template<class T, class S>
	struct FindType;
	template<class T, class S, class... Ss>
	struct FindType<T, Set<S, Ss...>> {
		constexpr static int index = std::is_same_v<T, S> ?
			0 : 1+FindType<T, Set<Ss...>>::index;
	};
	template<class T>
	struct FindType<T, Set<>> {
		constexpr static int index = -1;
	};
	static_assert(FindType<char, Set<int, void, char, bool>>::index == 2, "FindType broken");
}
