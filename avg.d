import std.stdio;
import std.range;
import std.conv;
import std.algorithm;

void main(string[] args) {
	float[] gl = File(args[1]).byLine.map!(to!float).array;
	float[] vk = File(args[2]).byLine.map!(to!float).array;

	float gain = iota(0, gl.length).map!(i => (100*gl[i])/vk[i]).sum() / gl.length;
	float avgMSGL = gl.sum() / gl.length;
	float avgFPSGL = gl.map!(x => 1000/x).sum() / gl.length;
	float avgMSVK = vk.sum() / vk.length;
	float avgFPSVK = vk.map!(x => 1000/x).sum() / vk.length;

	writeln("gain: ", gain);
	writeln("avgMSGL: ", avgMSGL);
	writeln("avgFPSGL: ", avgFPSGL);
	writeln("avgMSVK: ", avgMSVK);
	writeln("avgFPSVK: ", avgFPSVK);
}