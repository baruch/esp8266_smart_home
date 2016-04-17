#ifndef DEBUG_PRINT_H
#define DEBUG_PRINT_H

#include <Print.h>
#include <WiFiClient.h>

class DebugPrint : public Print {
public:
	void begin(void);

	template <typename T1>
		void log(T1 t1)
	{
		print(millis());
		print(' ');
		println(t1);
	};

	template <typename T1, typename T2>
		void log(T1 t1, T2 t2)
	{
		print(millis());
		print(' ');
		print(t1);
		println(t2);
	};

	template <typename T1, typename T2, typename T3>
		void log(T1 t1, T2 t2, T3 t3)
	{
		print(millis());
		print(' ');
		print(t1);
		print(t2);
		println(t3);
	};

	template <typename T1, typename T2, typename T3, typename T4>
		void log(T1 t1, T2 t2, T3 t3, T4 t4)
	{
		print(millis());
		print(' ');
		print(t1);
		print(t2);
		print(t3);
		println(t4);
	};

	template <typename T1, typename T2, typename T3, typename T4, typename T5>
		void log(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5)
	{
		print(millis());
		print(' ');
		print(t1);
		print(t2);
		print(t3);
		print(t4);
		println(t5);
	};

	template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
		void log(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6)
	{
		print(millis());
		print(' ');
		print(t1);
		print(t2);
		print(t3);
		print(t4);
		print(t5);
		println(t6);
	};

	template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
		void log(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7)
	{
		print(millis());
		print(' ');
		print(t1);
		print(t2);
		print(t3);
		print(t4);
		print(t5);
		print(t6);
		println(t7);
	};

	template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
		void log(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8)
	{
		print(millis());
		print(' ');
		print(t1);
		print(t2);
		print(t3);
		print(t4);
		print(t5);
		print(t6);
		print(t7);
		println(t8);
	};

	virtual size_t write(uint8_t ch);
	void set_log_server(const char *server);

private:
	uint8_t buf[2048];
	uint16_t buf_start;
	uint16_t buf_end;

	char server_name[32];
	WiFiClient client;

	bool reconnect(void);
};

extern DebugPrint debug;

#endif
