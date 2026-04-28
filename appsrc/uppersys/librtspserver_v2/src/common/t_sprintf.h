#ifdef ON_ANDROID 
#include <stdarg.h>

#ifdef __cplusplus    //__cplusplus是cpp中自定义的一个宏
extern "C" {          //告诉编译器，这部分代码按C语言的格式进行编译，而不是C++的
#endif

	int t_printf(const char *format, ...);

	int t_sprintf(char *out, const char *format, ...);
	int t_snprintf( char *buf, unsigned int count, const char *format, ... ); 

#ifdef __cplusplus
}
#endif
#endif
