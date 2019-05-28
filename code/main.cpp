#include "TypeSet.h"
#include "VariantValue.h"

#include <iostream>

using namespace TypeSet;
using namespace VariantValue;

constexpr Value startMoney = I<20>();

int main() {
	std::cout << "Age?" << std::endl;
	int age; std::cin >> age;

	constexpr Value normalPrice = I<25>();
	constexpr Value seniorPrice = I<12>();

	Value price = select(age >= 18, select(age >= 65, seniorPrice, normalPrice), Value(False()));

	auto moneyLeft = visit(
		price,
		[](IntInterval<0, startMoney> p) { std::cout << "Bought ticket!" << std::endl; return startMoney - p; },
		[](Value<Set<False>>) { std::cout << "Too young" << std::endl; return startMoney; },
		[](auto) { std::cout << "Price too expensive" << std::endl; return startMoney; }
	);

	std::cout << "Money left: $" << moneyLeft << std::endl;
}