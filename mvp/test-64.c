// extern int doo(int x);

long long foo(long long x) {
	int z = x * 2;
	return z;
}

long long bar(int x, long long y) {
	long long z = foo(x + y);
	return z;
}
