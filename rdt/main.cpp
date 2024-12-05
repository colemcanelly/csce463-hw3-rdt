// main.cpp : This file contains the 'main' function. Program execution begins and ends there.
/*
* Cole McAnelly
* CSCE 463 - Dist. Network Systems
* Fall 2024
*/

#include "pch.h"

#include "SenderSocket.h"

#include "checksum.h"
#include "Message.h"

#define MAGIC_PORT 22345


/* CLI tests:
* 
* Part 2 (Window = 1)
localhost 15 1 0.1 0.2 0 14
* 
* 
* Part 3
* 1.7
localhost 20 10 0.1 0 0 1000

* 1.8
localhost 28 12000 0.1 0.0001 0 1000

* 1.9
localhost 20 10 0.01 0.1 0 1000

* 1.10
localhost 24 300 0.1 0.001 0 1000

* 1.11
localhost 15 10 0.01 0.5 0 14

* 1.12
localhost 30 3000 0.01 0 0 10000

*/


std::tuple<int, int, net::LinkProperties> parse_args(const char* argv[]) {
	int power = atoi(argv[2]);
	int window = atoi(argv[3]);
	if (window > (1e6 - SenderSocket::MAX_ATTEMPTS)) throw UsageError();
	
	float rtt = atof(argv[4]);
	if (rtt < 0 || rtt > 30.0) throw UsageError();

	float forward_loss = atof(argv[5]);
	if (forward_loss < 0 || forward_loss >= 1.0) throw UsageError();

	float reverse_loss = atof(argv[6]);
	if (reverse_loss < 0 || reverse_loss >= 1.0) throw UsageError();

	float speed = atof(argv[7]);
	if (speed <= 0 || speed > 10e3) throw UsageError();

	net::LinkProperties link(rtt, speed, forward_loss, reverse_loss, window + SenderSocket::MAX_ATTEMPTS);

	return std::make_tuple(power, window, link);
}

static void run(const char* hostname, int power, int window, net::LinkProperties link)
{
	printf("Main:\tsender W = %d, RTT = %.3f sec, loss %g / %g, link %d\n", window, link.rtt, link.pLoss[0], link.pLoss[1], int(link.speed / 1e6));
	auto message = Message::from_exponent(power);


	auto start = Time::now();

	auto rdt = SenderSocket::open(hostname, MAGIC_PORT, link, window);

	printf("Main:\tconnected to %s in %.3f sec, pkt size %llu bytes\n",
		hostname, to_float(time_elapsed<microseconds>(start)), SenderSocket::MAX_PKT_SIZE);

	start = Time::now();


	for (size_t len, off = 0; off < message.size(); off += len)
	{
		len = __min(message.size() - off, SenderSocket::MAX_PKT_SIZE - sizeof(net::SenderDataHeader));
		rdt->send(message.data() + off, len);
	}

	float transfer_duration = to_float(time_elapsed<microseconds>(start));

	auto crc = rdt->close();
	if (crc != CRC32::checksum(message))
		throw std::runtime_error("Checksum does not match!!\tGot: " + std::to_string(crc) + " Expected: " + std::to_string(CRC32::checksum(message)) + "\n");


	printf("Main:\ttransfer finished in %.3f sec, %.2f Kbps, checksum %X\n", transfer_duration, (8 * message.size() / transfer_duration) / 1e3, crc);
	printf("Main:\testRTT %.3f, ideal rate %.2f Kbps\n", rdt->estimated_rtt(), 8 * window * (SenderSocket::MAX_PKT_SIZE - sizeof(net::SenderDataHeader)) / (1e3 * rdt->estimated_rtt()));
}


static void test()
{
	static constexpr size_t NUM_TESTS = 7;
	const char* testnames[NUM_TESTS] = {
		"**Part 2 (Window = 1)",
		"** 1.7",
		"** 1.8",
		"** 1.9",
		"** 1.10",
		"** 1.11",
		"** 1.12"
	};

	const char* tests[NUM_TESTS][8] = {
		{ "", "localhost", "15", "1", "0.1", "0.2", "0", "14" },			// part 2
		{ "", "localhost", "20", "10", "0.1", "0", "0", "1000" },			// 1.7
		{ "", "localhost", "28", "12000", "0.1", "0.0001", "0", "1000" },	// 1.8
		{ "", "localhost", "20", "10", "0.01", "0.1", "0", "1000" },		// 1.9
		{ "", "localhost", "24", "300", "0.1", "0.001", "0", "1000" },		// 1.10
		{ "", "localhost", "15", "10", "0.01", "0.5", "0", "14" },			// 1.11
		{ "", "localhost", "30", "3000", "0.01", "0", "0", "10000" }		// 1.12
	};

	for (int i = 0; i < NUM_TESTS; i++) {
		printf("\n\n%s\n", testnames[i]);
		auto [power, window, link] = parse_args(tests[i]);
		run(tests[i][1], power, window, link);
		std::this_thread::sleep_for(2s);
	}

	exit(0);
}


int main(int argc, const char* argv[]) try {

	//test();

	if (argc != 8) throw UsageError();
	auto [ power, window, link ] = parse_args(argv);

	run(argv[1], power, window, link);

	return 0;
}
catch (const SenderSocket::Error& e) { printf("Main:\t%s\n", e.what()); }
catch (const UsageError& e) { printf("%s\n", e.what()); }
catch (const std::runtime_error& e) { printf("%s\n", e.what()); }