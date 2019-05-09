#include "Restricted.h"

#include <iostream>

using namespace Restricted;
using namespace std;

constexpr auto startMoney = Restrict<20>();

auto BuyTicket(RestrictSpan<0, startMoney> price) {
	cout << "Bought ticket for $" << price << endl;
	return startMoney - price;
}

int main() {
	cout << "Age?" << endl;
	int age; cin >> age;

	constexpr auto adultPrice = Restrict<25>();
	constexpr auto childPrice = Restrict<15>();

	auto price = join(age >= 18, adultPrice, childPrice);

	auto moneyLeft = split(
		price,
		BuyTicket,
		[](auto p) { cout << "Price too expensive: $" << p << endl; return startMoney;
	});

	cout << "Money left: $" << moneyLeft << endl;
	cout << typeid(moneyLeft).name() << endl;
}