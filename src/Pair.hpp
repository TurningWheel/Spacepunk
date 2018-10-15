// Pair.hpp

#pragma once

template <typename A, typename B>
struct UnorderedPair {
	UnorderedPair() {}
	UnorderedPair(A _a, B _b) :
		a(_a),
		b(_b) {}
	A a;
	B b;

	bool operator==(const UnorderedPair& src) {
		if( (a == src.a && b == src.b) ||
			(b == src.a && a == src.b) ) {
			return true;
		} else {
			return false;
		}
	}

	bool operator!=(const UnorderedPair& src) {
		if( (a != src.a || b != src.b) &&
			(b != src.a || a != src.b) ) {
			return true;
		} else {
			return false;
		}
	}

	// @return true if the other pair shares either of our values
	bool shares(const UnorderedPair& src) {
		if( a == src.a || a == src.b ||
			b == src.a || b == src.b ) {
			return true;
		} else {
			return false;
		}
	}
};

template <typename A, typename B>
struct OrderedPair {
	OrderedPair() {}
	OrderedPair(A _a, B _b) :
		a(_a),
		b(_b) {}
	A a;
	B b;

	bool operator==(const OrderedPair& src) {
		if( a == src.a && b == src.b ) {
			return true;
		} else {
			return false;
		}
	}

	bool operator!=(const OrderedPair& src) {
		if( a != src.a || b != src.b ) {
			return true;
		} else {
			return false;
		}
	}
};

template <typename A, typename B>
using Pair = OrderedPair<A, B>;