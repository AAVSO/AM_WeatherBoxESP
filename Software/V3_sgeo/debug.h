#ifndef _JAMES_SYNGE_DEBUG_H_
#define _JAMES_SYNGE_DEBUG_H_

#ifdef DO_DEBUG
#define DBG(a) Serial.print(a)
#define DBG2(a,b) Serial.print(a,b)
#define DBGLN(a) Serial.println(a)
#define DBGLN2(a,b) Serial.println(a,b)
#define ASSERT(exp) if (exp) ; else { DBGLN("ASSERT failed: " #exp); }
#else
#define DBG(a)
#define DBG2(a,b)
#define DBGLN(a)
#define DBGLN2(a,b)
#define ASSERT(exp)
#endif

#endif  // _JAMES_SYNGE_DEBUG_H_
