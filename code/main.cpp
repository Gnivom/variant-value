#include "TypeSet.h"
#include "VariantValue.h"

#include <iostream>

using namespace TypeSet;
using namespace VariantValue;

constexpr Value startMoney = I<20>();

int main() {
	std::cout << "Age?" << std::endl;
	int age; std::cin >> age;

	constexpr Value adultPrice = I<25>();
	constexpr Value childPrice = I<15>();

	Value price = select(age >= 18, adultPrice, childPrice);

	int x = startMoney;

	auto moneyLeft = visit(
		price,
		[](ValueInterval<0, 20> p) { std::cout << "Bought ticket!" << std::endl; return Value(I<5>()); },
		[](auto p) { std::cout << "Price too expensive: $" << std::endl; return startMoney; }
	);

	Value y = adultPrice + moneyLeft;

	std::cout << "Money left: $" << moneyLeft << std::endl;
//	std::cout << typeid(x).name() << std::endl;
}