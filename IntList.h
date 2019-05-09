#pragma once

#include <type_traits>
#include <utility>
#include <array>

namespace IntList {
	// List of integers
	template<int... Is> using IL = std::integer_sequence<int, Is...>;
	// Pair of integers
	template<int I1, int I2> struct IntPair { constexpr static int i1 = I1; constexpr static int i2 = I2; };
	// List of pairs
	template<class... Ps> struct PairList {};

	// Contains check at runtime
	template<int... Is> constexpr bool contains(IL<Is...>, int x) { return ((x == Is) || ...); }

	// Concatenate
	template<class list1, class list2> struct Concat;
	template<int... Is1, int... Is2>
	struct Concat<IL<Is1...>, IL<Is2...>> { using type = IL<Is1..., Is2...>; };
	template<class... Ts1, class... Ts2>
	struct Concat<PairList<Ts1...>, PairList<Ts2...>> { using type = PairList<Ts1..., Ts2...>; };
	template<class list1, class list2> using Concat_t = typename Concat<list1, list2>::type;

	// Cartesian Product
	template<class list1, class list2> struct Cartesian;
	template<int... Is>
	struct Cartesian<IL<>, IL<Is...>> { using type = PairList<>; };
	template<int I1, int... Is1, int... Is2>
	struct Cartesian<IL<I1, Is1...>, IL<Is2...>> {
		using type = Concat_t <
			PairList<IntPair<I1, Is2>...>,
			typename Cartesian<IL<Is1...>, IL<Is2...>>::type
		>;
	};
	template<class list1, class list2> using Cartesian_t = typename Cartesian<list1, list2>::type;

	// Set Complement
	template<class startList, class subtractList> struct Complement;
	template<int... Is>
	struct Complement<IL<Is...>, IL<>> { using type = IL<Is...>; };
	template<int J, int... Js>
	struct Complement<IL<>, IL<J, Js...>> { using type = IL<>; };
	template<int I, int... Is, int J>
	struct Complement<IL<I, Is...>, IL<J>> { using type = Concat_t<typename Complement<IL<Is...>, IL<J>>::type, typename std::conditional<I == J, IL<>, IL<I>>::type>; };
	template<int... Is, int J1, int J2, int... Js>
	struct Complement<IL<Is...>, IL<J1, J2, Js...>> { using type = typename Complement<typename Complement<IL<Is...>, IL<J2, Js...>>::type, IL<J1>>::type; };
	template<class startList, class subtractList> using Complement_t = typename Complement<startList, subtractList>::type;

	template<int Start, int End>
	struct Span { using type = Concat_t<IL<Start>, typename Span<Start + 1, End>::type>; };
	template<int Start>
	struct Span<Start, Start> { using type = IL<Start>; };
	template<int Start, int End> using Span_t = typename Span<Start, End>::type;

	// Cleanup = sort and remove duplicates
	template <class intList> struct Cleanup;
	template<int... IIs>
	struct Cleanup<IL<IIs...>> {
	private:
		template<size_t... indices, int... Is>
		constexpr static auto to_int_list(std::index_sequence<indices...>, IL<Is...>) {
			constexpr std::array a = bubble_sort_push_duplicates<Is...>().first;
			return IL<a[indices]...>();
		}
		template<int... Is>
		constexpr static auto bubble_sort_push_duplicates() {
			constexpr int N = sizeof...(Is);
			std::array<int, N> a = { Is... };
			bool sorted = false;
			int removed = 0;
			while (!sorted) {
				sorted = true;
				for (int i = 0; i < N - 1 - removed; ++i) {
					if (a[i] == a[i + 1]) {
						removed += 1;
						sorted = false;
						auto temp = a[i + 1];
						a[i + 1] = a[N - removed];
						a[N - removed] = temp;
					}
					if (a[i] > a[i + 1]) {
						sorted = false;
						auto temp = a[i];
						a[i] = a[i + 1];
						a[i + 1] = temp;
					}
				}
			}
			return std::pair(a, removed);
		}
		constexpr static auto sorted = bubble_sort_push_duplicates<IIs...>();
		constexpr static auto intList = to_int_list(std::make_index_sequence<sorted.first.size() - sorted.second>(), IL<IIs...>());

	public:
		using type = typename std::remove_const<decltype(intList)>::type;
	};
	template<class intList> using Cleanup_t = typename Cleanup<intList>::type;

	// Set union
	template<class list1, class list2>
	using Union_t = Cleanup_t<Concat_t<list1, list2>>;

	// Set Intersection
	template<class list1, class list2>
	struct Intersection {
		using C1 = Complement_t<list1, list2>;
		using C2 = Complement_t<list2, list1>;
		using type = Complement_t<Union_t<list1, list2>, Concat_t<C1, C2>>;
	};
	template<class list1, class list2> using Intersection_t = typename Intersection<list1, list2>::type;

	// Apply functor to each pair
	template<class F, class pairList> struct ApplyToPairList;
	template<class F, int... Is, int... Js>
	struct ApplyToPairList<F, PairList<IntPair<Is, Js>...>> {
		using type = IL<(F::f(Is, Js))...>;
	};
	template<class F, class pairList> using Papply = typename ApplyToPairList<F, pairList>::type;

	template<int... Is> constexpr bool is_super_set(IL<Is...>, IL<>) { return true; }
	template<int... Is, int J, int... Js>
	constexpr bool is_super_set(IL<Is...> is, IL<J, Js...>) {
		return ((Is == J) || ...) && is_super_set(is, IL<Js...>());
	}

	static_assert(std::is_same_v<Union_t<IL<>, IL<>>, IL<>>, "Union broken");
	static_assert(std::is_same_v<Union_t<IL<1, 4, 3, 2>, IL<3, 4, 0, 6>>, IL<0, 1, 2, 3, 4, 6>>, "Union broken");
	static_assert(std::is_same_v<Intersection_t<IL<>, IL<>>, IL<>>, "Intersection broken");
	static_assert(std::is_same_v<Intersection_t<IL<-1, 5, 3, 2>, IL<1, 3, 4, -1>>, IL<-1, 3>>, "Intersection broken");

}