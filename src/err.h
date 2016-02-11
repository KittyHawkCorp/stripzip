/**
 * @file
 * Error reporting convienence macros.
 * Only the subset required for StripZIP.
 *
 * Copyright (c) 2016, Zee.Aero
 * All rights reserved.
 */

/* Shim for error printing outside of a bigger framework */
#define err_printf(...) printf(__VA_ARGS__)

#define ERR_PREFIX_(file, line, func)                           \
  do {                                                          \
    err_printf("%s:%d: %s:\n", file, line, func);               \
  } while (0)
#define ERR_PREFIX() ERR_PREFIX_(__FILE__, __LINE__, __func__)

/**
 * Print an error message and abort the current function (that is, return
 * \a ret) if the expression is false/zero.
 *
 * @param expr The expression.
 * @param ret The return value if \a expr is false/zero.
 */
#define ERR_RET_IF_NOT(expr, ret)                       \
  do {                                                  \
    typeof(expr) expr_ = (expr);                        \
    typeof(ret) ret_ = (ret);                           \
    if (!expr_)                                         \
    {                                                   \
      ERR_PREFIX();                                     \
      err_printf("    ERR_RET_IF_NOT(%s, %s)\n",        \
             (#expr), #ret);                            \
      return ret_;                                      \
    }                                                   \
  } while (0)

/**
 * Print an error message if the two expressions are not equal.
 *
 * @param expr_1 The first expression.
 * @param expr_2 The second expression.
 *
 * @return True/nonzero if the message was printed (\a expr_1 and \a
 * expr_2 were not equal).
 */
#define ERR_IF_NEQ(expr_1, expr_2)                              \
  ({                                                            \
    typeof(expr_1) expr_1_ = (expr_1);                          \
    typeof(expr_2) expr_2_ = (expr_2);                          \
    bool test = expr_1_ != expr_2_;                             \
    if (test)                                                   \
    {                                                           \
      ERR_PREFIX();                                             \
      err_printf("    ERR_IF_NEQ(%s, %s)\n",                    \
             (#expr_1), #expr_2);                               \
      err_printf("\t%s = %jd\n", #expr_1, (intmax_t)expr_1_);   \
      err_printf("\t%s = %jd\n", #expr_2, (intmax_t)expr_2_);   \
    }                                                           \
    test;                                                       \
  })

/**
 * Print an error message and abort the current function (that is, return
 * \a ret) if the two expressions are not equal.
 *
 * @param expr_1 The first expression.
 * @param expr_2 The second expression.
 * @param ret The return value if two expressions are not equal.
 */
#define ERR_RET_IF_NEQ(expr_1, expr_2, ret)                     \
  do {                                                          \
    typeof(expr_1) expr_1_ = (expr_1);                          \
    typeof(expr_2) expr_2_ = (expr_2);                          \
    typeof(ret) ret_ = (ret);                                   \
    if (expr_1_ != expr_2_)                                     \
    {                                                           \
      ERR_PREFIX();                                             \
      err_printf("    ERR_RET_IF_NEQ(%s, %s, %s)\n",            \
             (#expr_1), #expr_2, #ret);                         \
      err_printf("\t%s = %jd\n", #expr_1, (intmax_t)expr_1_);   \
      err_printf("\t%s = %jd\n", #expr_2, (intmax_t)expr_2_);   \
      return ret_;                                              \
    }                                                           \
  } while (0)

/**
 * Print an error message and abort the current function (that is, return
 * \a ret) if \a expr is negative.  The message contains information
 * from errno.
 *
 * @param expr The expression.
 * @param ret The return value if \a expr is negative.
 */
#define ERR_RET_ON_ERRNO(expr, ret)                                     \
  do {                                                                  \
    typeof(expr) expr_ = (expr);                                        \
    typeof(ret) ret_ = (ret);                                           \
    if (expr_ < 0)                                                      \
    {                                                                   \
      ERR_PREFIX();                                                     \
      err_printf("    ERR_RET_ON_ERRNO(%s, %s): errno = %s (%d)\n",     \
                 (#expr), #ret, strerror(errno), errno);                \
      err_printf("\t%s = %d\n", #expr, expr_);                          \
      return ret_;                                                      \
    }                                                                   \
  } while (0)
