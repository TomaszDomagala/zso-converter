char buf[13];
int size = 13;


int check() {
	for (long long i = 0; i < size; i++)
		if (buf[i] != i)
			return -1;
	return 0;
}
