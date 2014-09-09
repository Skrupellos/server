#ifndef __UTIL_H
#define __UTIL_H 

#define PRINT_ERR(format, ...) { \
	fprintf(stderr, format "\n", ##__VA_ARGS__); \
	fflush(stderr); \
}

#define PRINT_ERRNO(format, ...) { \
	fprintf(stderr, format " (%s)\n", ##__VA_ARGS__, strerror(errno)); \
	fflush(stderr); \
}


#define RET_ERR(format, ...) { \
	fprintf(stderr, format "\n", ##__VA_ARGS__); \
	fflush(stderr); \
	return EXIT_FAILURE; \
}

#define RET_ERRNO(format, ...) { \
	fprintf(stderr, format " (%s)\n", ##__VA_ARGS__, strerror(errno)); \
	fflush(stderr); \
	return EXIT_FAILURE; \
}


#define EXIT_ERR(format, ...) { \
	fprintf(stderr, format "\n", ##__VA_ARGS__); \
	fflush(stderr); \
	exit(EXIT_FAILURE); \
}

#define EXIT_ERRNO(format, ...) { \
	fprintf(stderr, format " (%s)\n", ##__VA_ARGS__, strerror(errno)); \
	fflush(stderr); \
	exit(EXIT_FAILURE); \
}

#endif
