module roundRobin;

struct Result(T) {
	T a;
	T b;
}

Result!T[] roundRobin(T)(const T[] input_)
in {
	assert(input_.length % 2 == 0, "Can't be a odd amount");
	assert(input_.length > 1, "Need more than 1 element!");
}
body {
	Result!T[] battles;
	T[] input = input_.dup;
	scope (exit)
		input.destroy;

	alias rotate = () {
		import core.stdc.string : memmove;

		const T tmp = input[1];
		memmove(&input[1], &input[2], (input.length - 2) * T.sizeof);
		input[$ - 1] = tmp;
	};

	foreach (size_t round; 0 .. input.length - 1) {
		foreach (size_t battleIdx; 0 .. input.length / 2)
			battles ~= Result!T(input[battleIdx * 2], input[battleIdx * 2 + 1]);
		rotate();
	}
	return battles;
}
