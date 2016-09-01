/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   createTable_session.h
 * Author: he
 *
 * Created on August 30, 2016, 12:55 PM
 */

#ifndef CREATETABLE_SESSION_H
#define CREATETABLE_SESSION_H

/*
  __FUNCTION__/__func__ is not portable. We do not promise 
  that  our example definition covers each and every compiler.
  If not, it is up to you to find a different definition for 
  your setup.
*/

#if __STDC_VERSION__ < 199901L
#  if __GNUC__ >= 2
#    define EXAMPLE_FUNCTION __FUNCTION__
#  else
#    define EXAMPLE_FUNCTION "(function n/a)"
#  endif
#elif defined(_MSC_VER)
#  if _MSC_VER < 1300
#    define EXAMPLE_FUNCTION "(function n/a)"
#  else
#    define EXAMPLE_FUNCTION __FUNCTION__
#  endif
#elif (defined __func__)
#  define EXAMPLE_FUNCTION __func__
#else
#  define EXAMPLE_FUNCTION "(function n/a)"
#endif

/*
  Again, either you are lucky and this definition 
  works for you or you have to find your own.
*/
#ifndef __LINE__
  #define __LINE__ "(line number n/a)"
#endif

// Connection properties
#define EXAMPLE_DB   "GeoData"
#define EXAMPLE_HOST "localhost"
#define EXAMPLE_USER "root"
#define EXAMPLE_PASS "gravityisfun"

// Sample data
#define EXAMPLE_NUM_TEST_ROWS 1
struct _test_data {
	int id;
	const char* sessionName;
};
static _test_data test_data[EXAMPLE_NUM_TEST_ROWS] = {
	{1, "MFAM"}, 
};



#endif /* CREATETABLE_SESSION_H */

