extern int doo(int x);

int foo(int x) {
	int z = x * 2;
	return z;
}

int bar(int x) {
	int z = foo(x);
	int zz = doo(z);
	return zz;
}
