# define UNREFERENCED_PARAMETER(P) do { (void) P; } while (0)

// TODO take down the program?
#define CHECK(bool_expr) do {                                        \
    if (!(bool_expr)) {                                              \
      printf("Fatal error in file %s, line %d: !(%s)\n",             \
              __FILE__, __LINE__, #bool_expr);                       \
    }                                                                \
  } while (0)
