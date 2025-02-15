#include "common.h"

/**
 * @brief 将n字节数据从src复制到dst 待完善
 *
 * @param dst 目的地址
 * @param src 源地址
 * @param n 待复制数据字节数
 * @return void* 目的地址
 */
void *memcpy(void *dst, const void *src, size_t n)
{
	uint8_t *d = (uint8_t *)dst;
	const uint8_t *s = (const uint8_t *)src;
	while (n--)
	{
		*d++ = *s++;
	}
	return dst;
}

/**
 * @brief 用字符c填充buf的前n个字节 待完善
 *
 * @param buf 待初始化地址
 * @param c 用于初始化的字符
 * @param n 地址初始化字节数
 * @return void* 初始化地址
 */
void *memset(void *buf, char c, size_t n)
{
	uint8_t *p = (uint8_t *)buf;
	while (n--)
	{
		*p++ = c;
	}
	return buf;
}

/**
 * @brief 将字符串从src复制到dst 待完善
 *
 * @param dst 目的地址
 * @param src 源地址
 * @return char* 目的地址
 */
char *strcpy(char *dst, const char *src)
{
	char *d = dst;
	while (*src)
	{
		*d++ = *src++;
	}
	*d = '\0';
	return dst;
}

// 比较字符串
/**
 * @brief 字符串比较
 *
 * @param s1 待比较字符串1
 * @param s2 待比较字符串2
 * @return int s1 == s2时返回0，s1 > s2时返回正数，s1 < s2时返回负数
 */
int strcmp(const char *s1, const char *s2)
{
	while (*s1 && *s2)
	{
		if (*s1 != *s2)
		{
			break;
		}
		s1++;
		s2++;
	}
	// 比较时转换为 unsigned char * 是为了符合 POSIX 规范
	return *(unsigned char *)s1 - *(unsigned char *)s2;
}

extern void putchar(char ch);

/**
 * @brief 格式化输出函数(目前仅支持三种格式说明符：%d(十进制)、%x(十六进制)和%s(字符串)
 *
 * @param fmt 格式字符串，由格式说明符和普通字符组成
 * @param ... 待输出数据
 */
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
			case '\0': // %为字符串末尾
				putchar('%');
				goto end;
			case '%':
				putchar('%');
				break;
			case 's': // 打印NULL结尾的字符串
			{
				const char *s = va_arg(vargs, const char *);
				while (*s)
				{
					putchar(*s);
					s++;
				}
				break;
			}
			case 'd': // 打印十进制整型
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
			case 'x': // 打印十六进制整型
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