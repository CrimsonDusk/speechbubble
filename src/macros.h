#ifndef COIRC_MACROS_H
#define COIRC_MACROS_H

#define APPNAME "CobaltIRC"
#define UNIXNAME "cobaltirc"
#define VERSION_MAJOR 0
#define VERSION_MINOR 99
#define VERSION_PATCH 0

#define PROPERTY(ACCESS, T, NAME) \
	private: \
		T m_##NAME; \
	public: \
		inline T const& NAME() const { return m_##NAME; } \
	ACCESS: \
	inline void set_##NAME (T const& a) { m_##NAME = a; }
	
#define NEW_PROPERTY(ACCESS, T, NAME) \
	private: \
		T m_##NAME; \
		public: \
			inline T const& NAME() const { return m_##NAME; } \
			ACCESS: \
			inline void Set##NAME (T const& a) { m_##NAME = a; }

#define defineClass(NAME) typedef NAME SelfType;

// Q_DELETE_COPY is not sufficient for me, with it KDevelop still thinks the copy c-tor
// and the copy operator are valid functions and nags me to create definitions for them.
// It understands what a deleted method is though. Plus this is the c++11 way of doing
// it anyway.
#define deleteCopy(NAME) \
		NAME (const NAME&) = delete; \
		void operator= (const NAME&) = delete;

#ifdef IN_IDE_PARSER
# define elif else if
#else
# define elif(A) else if (A)
#endif

#endif // COIRC_MACROS_H