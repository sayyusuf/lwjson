
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef DEBUG_MODE
# define PRINT		fprintf
# define NOP_PARAM
#else
# define PRINT		(void)
# define NOP_PARAM	(void)
#endif


#define FT_CHECK(status, ret, ...){			\
	if (!(status))					\
	{						\
		PRINT(NOP_PARAM stderr, __VA_ARGS__);	\
		return (ret);				\
	}						\
}

#define FT_ASSERT(status, ret, ...){			\
	if (!(status))					\
	{						\
		fprintf(stderr, __VA_ARGS__);		\
		exit(ret);				\
	}						\
}


/**
 * @param fmt ".dumyarray[2][1].employe.name"
 * @param fmt "[0][2].employ.name"
 * */

#define EFMT	1
#define ESTR	2
#define ENFND	3
#define EBUFF	4

static int
jumpto(const char **str, const char c);


static int
space_iter(const char **str)
{
	if (!str | !*str)
		return (-1);
	while (**str && isspace(**str))
		++(*str);
	if (!**str)
		return (-1);
	return (0);
}

static size_t
key_len(const char *str)
{
	size_t	len;

	len = 0;
	while (str[len] && !strchr(":.[{\"", str[len]) && !isspace(str[len]))
		++len;
	return (len);
}

static int
jumpto_dquote(const char **str)
{
	++(*str);
	while (**str && **str != '"')
		++(*str);
	if (!**str)
		return (-1);
	++(*str);
	return (0);
}

static int
jumpto_bracket(const char **str, const char c)
{
	++(*str);
	while (**str && **str != c)
	{
		if ('"' == **str || '{' == **str || '[' == **str)
		{
			if (0 > jumpto(str, **str))
				return (-1);
		}
		else
			++(*str);
	}
	if (!**str)
		return (-1);
	++(*str);
	return (0);
}

static int
jumpto_number(const char **str)
{
	while (**str && (isdigit(**str) || '.' == **str))
		++(*str);
	if (!**str)
		return (-1);
	return (0);
}

static int
jumpto(const char **str, const char c)
{
	switch(c)
	{
		case '"':
			return (jumpto_dquote(str));
		case '{':
			return (jumpto_bracket(str, '}'));
		case '[':
			return (jumpto_bracket(str, ']'));	
		case 't':
			if (!strncmp("true", *str, strlen("true")))
				return ((*str += strlen("true")), 0);
			break ;
		case 'f':
			if (!strncmp("false", *str, strlen("fasle")))
				return ((*str += strlen("fasle")), 0);
			break ;
		case 'n':
			if (!strncmp("null", *str, strlen("null")))
				return ((*str += strlen("null")), 0);
			break ;
		default:
			if (**str && (isdigit(**str) || '.' == **str))
				return (jumpto_number(str));
			else
				return (-1);
	}
	return (-1);
}

static int
parse_array(const char *fmt, const char **begin, const char **end)
{
	int		k;
	int		i;
	const char	*str;
	const char	*tmp;

	k = atoi(++fmt);
	i = 0;
	str = *begin;
	++str;
	while (i < k)
	{
		FT_CHECK( -1 < space_iter(&str) && str < *end, -ESTR, "\n");
		if (!*str || 0 > jumpto(&str, *str) || str >= *end)
			return (-ESTR);
		FT_CHECK( -1 < space_iter(&str) && str < *end, -ESTR, "\n");
		if (']' == *str)
			return (-ENFND);
		if (',' != *str)
			return (-ESTR);
		++str;
		++i;
	}
	FT_CHECK( -1 < space_iter(&str) && str < *end, -ESTR, "\n");
	tmp = str;
	if (!*str || 0 > jumpto(&str, *str) || str >= *end)
		return (-ESTR);
	*begin = tmp;
	*end = str;
	return (0);
}


static int
parse_obj(const char *fmt, const char **begin, const char **end)
{	
	size_t		fmt_len;
	size_t		str_len;
	int		res;
	const char	*str;
	const char	*tmp;

	str = *begin;
	++str;
	fmt_len = key_len(++fmt);
	if (!fmt_len)
		return (-EFMT);
	while (*str)
	{
		FT_CHECK( -1 < space_iter(&str) && str < *end, -ESTR, "\n");
		FT_CHECK( '\"' == *str , -ESTR, "\n");
		str_len = key_len(++str);
		if (!str_len)
			return (-ESTR);
		res = fmt_len == str_len && !strncmp(str, fmt, fmt_len > str_len? fmt_len: str_len);
		str +=  str_len;
		while (*str && ':' != *str)
			++str;
		++str;
		FT_CHECK( -1 < space_iter(&str) && str < *end, -ESTR, "\n");
		tmp = str;
		if (!*str || 0 > jumpto(&str, *str) || str >= *end)
			return (-ESTR);
		if (res)
		{
			*begin = tmp;
			*end = str;
			return (0);
		}
		FT_CHECK( -1 < space_iter(&str) && str < *end, -ESTR, "\n");
		if ('}' == *str)
			return (-ENFND);
		if (',' != *str)
			return (-ESTR);
		++str;
	}
	return (-ENFND);
}

static int
exec_type(const char *begin, const char *end, va_list *args)
{
	char	*buff;
	int	n;

	buff = va_arg(*args, char *);
	n = va_arg(*args, int);
	if (*begin == '"')
	{
		++begin;
		--end;
	}
	if (n < end - begin)
		return (-EBUFF);
	strncpy(buff, begin, end - begin);
	buff[end - begin] = 0;
	return (0);

}

static int
parse_any(const char *str, const char *fmt, va_list *args)
{
	int		len;
	int		status;
	const char	*begin;
	const char	*end;

	FT_CHECK( -1 < space_iter(&str), -ESTR, "\n");
	len = strlen(str);
	begin = str;
	end = str + len;
	while (*fmt)
	{
		switch(*fmt)
		{
			case '.':
				status = parse_obj(fmt, &begin, &end);
				break ;
			case '[':
				status = parse_array(fmt, &begin, &end);
				break ;
			default:
				return (-EFMT);
		}
		if (0 > status)
			return (status);
		fmt += key_len(fmt + 1) + 1;
	}
	return (exec_type(begin, end, args));
}


int
lwjson_parse(const char *str, const char *fmt, ...)
{
	va_list	args;
	int	status;
	if (!str || !fmt)
		return (-1);
	va_start(args, fmt);
	status = parse_any(str, fmt, &args);
	va_end(args);
	return (status);
}

#include <stdio.h>
#include <assert.h>
#include <fcntl.h>

int main(int ac, char *av[])
{
	char 	strbuf[10000];
	char	buff[10000];
	int	stat;
	int	fd;
	int	ret;

	(void)ac;
	fd = open(av[1], O_RDONLY);
	FT_ASSERT(0 < fd, -1,"fd < 0\n");
	ret = read(fd, strbuf, 10000);
	assert(0 <= ret);
	strbuf[ret] = 0;
	printf("%s\n", av[2]);
	stat =  lwjson_parse(strbuf, av[2], buff, 10000);
	printf("%s\n", buff);
	return (stat);


}
