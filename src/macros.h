#ifndef COIRC_MACROS_H
#define COIRC_MACROS_H

#define APPNAME "Speechbubble"
#define UNIXNAME "speechbubble"
#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0

#define BOLD_STR			"\x02"
#define BOLD_CHAR			'\x02'
#define COLOR_STR			"\x03"
#define COLOR_CHAR			'\x03'
#define NORMAL_STR			"\x0F"
#define NORMAL_CHAR			'\x0F'
#define UNDERLINE_STR		"\x15"
#define UNDERLINE_CHAR		'\x15'
#define REVERSE_STR			"\x16"
#define REVERSE_CHAR		'\x16'
#define ITALIC_CHAR			'\x1D'
#define ITALIC_STR			"\x1D"

#ifndef __GNUC__
# define __attribute__(X)
#endif

// Q_DELETE_COPY is not sufficient for me, with it KDevelop still thinks the copy c-tor
// and the copy operator are valid functions and nags me to create definitions for them.
// It understands what a deleted method is though. Plus this is the c++11 way of doing
// it anyway.
#define DELETE_COPY(NAME) \
		NAME (const NAME&) = delete; \
		void operator= (const NAME&) = delete;

#define PROPERTY(ACCESS, TYPE, READ, WRITE, WRITETYPE)			\
private:														\
	TYPE m_##READ;												\
																\
public:															\
	inline TYPE const& READ() const								\
	{															\
		return m_##READ; 										\
	}															\
																\
ACCESS:															\
	void WRITE (TYPE const& a) PROPERTY_##WRITETYPE (READ)		\

#define PROPERTY_STOCK_WRITE(READ)								\
	{															\
		m_##READ = a;											\
	}

#define PROPERTY_CUSTOM_WRITE(READ)								\
	;

#define elif(A) else if (A)

#endif // COIRC_MACROS_H