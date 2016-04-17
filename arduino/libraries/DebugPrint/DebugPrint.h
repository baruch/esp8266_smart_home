#ifndef DEBUG_PRINT_H
#define DEBUG_PRINT_H

#include <Arduino.h>

class DebugPrint {
public:
	template <typename T1>
		void log(T1 t1)
	{
		Serial.print(millis());
		Serial.print(' ');
		Serial.println(t1);
	};

	template <typename T1, typename T2>
		void log(T1 t1, T2 t2)
	{
		Serial.print(millis());
		Serial.print(' ');
		Serial.print(t1);
		Serial.println(t2);
	};

	template <typename T1, typename T2, typename T3>
		void log(T1 t1, T2 t2, T3 t3)
	{
		Serial.print(millis());
		Serial.print(' ');
		Serial.print(t1);
		Serial.print(t2);
		Serial.println(t3);
	};

	template <typename T1, typename T2, typename T3, typename T4>
		void log(T1 t1, T2 t2, T3 t3, T4 t4)
	{
		Serial.print(millis());
		Serial.print(' ');
		Serial.print(t1);
		Serial.print(t2);
		Serial.print(t3);
		Serial.println(t4);
	};

	template <typename T1, typename T2, typename T3, typename T4, typename T5>
		void log(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5)
	{
		Serial.print(millis());
		Serial.print(' ');
		Serial.print(t1);
		Serial.print(t2);
		Serial.print(t3);
		Serial.print(t4);
		Serial.println(t5);
	};

	template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
		void log(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6)
	{
		Serial.print(millis());
		Serial.print(' ');
		Serial.print(t1);
		Serial.print(t2);
		Serial.print(t3);
		Serial.print(t4);
		Serial.print(t5);
		Serial.println(t6);
	};

	template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
		void log(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7)
	{
		Serial.print(millis());
		Serial.print(' ');
		Serial.print(t1);
		Serial.print(t2);
		Serial.print(t3);
		Serial.print(t4);
		Serial.print(t5);
		Serial.print(t6);
		Serial.println(t7);
	};

	template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
		void log(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8)
	{
		Serial.print(millis());
		Serial.print(' ');
		Serial.print(t1);
		Serial.print(t2);
		Serial.print(t3);
		Serial.print(t4);
		Serial.print(t5);
		Serial.print(t6);
		Serial.print(t7);
		Serial.println(t8);
	};
};

extern DebugPrint debug;

#endif
