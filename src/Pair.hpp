//! @file Pair.hpp

#pragma once

template <typename A, typename B>
struct UnorderedPair {
	UnorderedPair() = default;
	UnorderedPair(A _a, B _b) :
		a(_a),
		b(_b) {}
	UnorderedPair(const UnorderedPair&) = default;
	UnorderedPair(UnorderedPair&&) = default;
	~UnorderedPair() = default;

	UnorderedPair& operator=(const UnorderedPair&) = default;
	UnorderedPair& operator=(UnorderedPair&&) = default;

	bool operator==(const UnorderedPair& src) {
		if ((a == src.a && b == src.b) ||
			(b == src.a && a == src.b)) {
			return true;
		} else {
			return false;
		}
	}

	bool operator!=(const UnorderedPair& src) {
		if ((a != src.a || b != src.b) &&
			(b != src.a || a != src.b)) {
			return true;
		} else {
			return false;
		}
	}

	//! @return true if the other pair shares either of our values
	//! @param src pair to compare with
	bool shares(const UnorderedPair& src) {
		if (a == src.a || a == src.b ||
			b == src.a || b == src.b) {
			return true;
		} else {
			return false;
		}
	}

	A a;
	B b;
};

template <typename A, typename B>
struct OrderedPair {
	OrderedPair() = default;
	OrderedPair(A _a, B _b) :
		a(_a),
		b(_b) {}
	OrderedPair(const OrderedPair&) = default;
	OrderedPair(OrderedPair&&) = default;
	~OrderedPair() = default;

	OrderedPair& operator=(const OrderedPair&) = default;
	OrderedPair& operator=(OrderedPair&&) = default;

	bool operator==(const OrderedPair& src) {
		if (a == src.a && b == src.b) {
			return true;
		} else {
			return false;
		}
	}

	bool operator!=(const OrderedPair& src) {
		if (a != src.a || b != src.b) {
			return true;
		} else {
			return false;
		}
	}

	A a;
	B b;
};

template <typename A, typename B>
using Pair = OrderedPair<A, B>;