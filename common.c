#include "common.h"

void putchar(char ch);

void printf(const char *fmt, ...)
{
	va_list vargs;
	va_start(vargs, fmt);

	while (*fmt)
	{
		if (*fmt == '%')
		{
			fmt++;
			switch (*fmt)
			{
			case '\0':	// %为字符串末尾
				putchar('%');
				goto end;
			case '%':
				putchar('%');
				break;
			case 's':	// 打印NULL结尾的字符串
			{
				const char *s = va_arg(vargs, const char *);
				while (*s)
				{
					putchar(*s);
					s++;
				}
				break;
			}
			case 'd':	// 打印十进制整型
			{
				int value = va_arg(vargs, int);
				if (value < 0)
				{
					putchar('-');
					value = -value;
				}
				int divisor = 1;
				while (value / divisor > 9)
				{
					divisor *= 10;
				}
				while (divisor > 0)
				{
					putchar('0' + value / divisor);
					value %= divisor;
					divisor /= 10;
				}
				break;
			}
			case 'x':	// 打印十六进制整型
			{
				int value = va_arg(vargs, int);
				for (int i = 7; i >= 0; i--)
				{
					int nibble = (value >> (i * 4)) & 0xf;
					putchar("0123456789abcdef"[nibble]);
				}
			}

			default:
				break;
			}
		}
		else
		{
			putchar(*fmt);
		}
		fmt++;
	}
end:
	va_end(vargs);
}