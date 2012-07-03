
#define FIX_ONE 0x00010000
#define FIX_HALF 0x00008000
#define FIX_POS_INF 0x7fffffff

typedef int Fix;

Fix fixMul(Fix a, Fix b) {
	long long int c = a;
	c *= b;
	return (Fix)(c / FIX_ONE);
}

Fix fixDiv(Fix a, Fix b) {
	long long int c = a;
	c *= FIX_ONE;
	return (Fix)(c / b);
}

Fix fixFloor(Fix a) {
	int v = a;
	v &= 0xffff0000;
	return (Fix)v;
}

Fix fixDenormalize(Fix a, int b) {
	int v = a;

	if (b > 0) {
		v <<= b;
	} else if (b < 0) {
		v >>= (-b);
	}

	return (Fix)v;
}

Fix fixExp(Fix a) {
	static const Fix C1 = 45440; // 0.693359375 * FIX_ONE;
	static const Fix C2 = -13; // -2.12194440e-4 * FIX_ONE;
	static const Fix A1 = 13; // 1.9875691500E-4 * FIX_ONE;
	static const Fix A2 = 91; // 1.3981999507E-3 * FIX_ONE;
	static const Fix A3 = 546; // 8.3334519073E-3 * FIX_ONE;
	static const Fix A4 = 2730; // 4.1665795894E-2 * FIX_ONE;
	static const Fix A5 = 10922; // 1.6666665459E-1 * FIX_ONE;
	static const Fix A6 = 32768; // 5.0000001201E-1 * FIX_ONE;
	static const Fix LOG_MAX = 681391; // 10.397207707933518353842322346778 * FIX_ONE
	static const Fix LOG_MIN = -726817; // -11.090354888959124950675713943331 * FIX_ONE
	static const Fix LOG2_E = 94548; // 1.4426950408889634073599246810019 * FIX_ONE

	Fix x = a;
	Fix z = 0;
	Fix z0 = 0;
	
	int n;

	if(x > LOG_MAX) {
		return FIX_POS_INF;
	}

	if(x < LOG_MIN) {
		return 0;
	}

	/* Express e**x = e**g 2**n
	*   = e**g e**( n loge(2) )
	*   = e**( g + n loge(2) )
	*/
	z = fixFloor(fixMul(LOG2_E, x) + FIX_HALF); /* floor() truncates toward -infinity. */
	x -= fixMul(z, C1);
	x -= fixMul(z, C2);
	n = z / FIX_ONE;

	z = fixMul(x, x);
	z0 = z;
	/* Theoretical peak relative error in [-0.5, +0.5] is 4.2e-9. */
	z = fixMul(A1, x) + A2;
	z = fixMul(z, x) + A3;
	z = fixMul(z, x) + A4;
	z = fixMul(z, x) + A5;
	z = fixMul(z, x) + A6;
	z = fixMul(z, z0);
	z += x + FIX_ONE;

	/* multiply by power of 2 */
	x = fixDenormalize(z, n);

	return x;
}