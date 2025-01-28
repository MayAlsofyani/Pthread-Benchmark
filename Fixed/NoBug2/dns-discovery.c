#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

struct dns_discovery_args {
  FILE *report;
  char *domain;
  int nthreads;
};
struct dns_discovery_args dd_args;
pthread_mutex_t mutexsum;

void error(const char *msg) {
  perror(msg);
  exit(1);
}

FILE *ck_fopen(const char *path, const char *mode) {
  FILE *file = fopen(path, mode);
  if (file == 0) error("fopen");
  return file;
}

void *ck_malloc(size_t size) {
  void *ptr = malloc(size);
  if (ptr == 0) error("malloc");
  return ptr;
}

void chomp(char *str) {
  while (*str) {
    if (*str == '\n' || *str == '\r') {
      *str = 0;
      return;
    }
    str++;
  }
}

void banner() {
  printf("   ___  _  ______    ___  _                              \n"
         "  / _ \\/ |/ / __/___/ _ \\(_)__ _______ _  _____ ______ __\n"
         " / // /    /\\ \\/___/ // / (_-</ __/ _ \\ |/ / -_) __/ // /\n"
         "/____/_/|_/___/   /____/_/___/\\__/\\___/___/\\__/_/  \\_, / \n"
         "                                                  /___/  \n"
         "\tby m0nad\n\n");
  if (dd_args.report) {
    fprintf(dd_args.report, "   ___  _  ______    ___  _                              \n"
                            "  / _ \\/ |/ / __/___/ _ \\(_)__ _______ _  _____ ______ __\n"
                            " / // /    /\\ \\/___/ // / (_-</ __/ _ \\ |/ / -_) __/ // /\n"
                            "/____/_/|_/___/   /____/_/___/\\__/\\___/___/\\__/_/  \\_, / \n"
                            "                                                  /___/  \n"
                            "\tby m0nad\n\n");
  }
}

int usage() {
  printf("usage: ./dns-discovery <domain> [options]\n"
         "options:\n"
         "\t-w <wordlist file> (default : %s)\n"
         "\t-t <threads> (default : 1)\n"
         "\t-r <report file>\n", "wordlist.wl");
  if (dd_args.report) {
    fprintf(dd_args.report, "usage: ./dns-discovery <domain> [options]\n"
                            "options:\n"
                            "\t-w <wordlist file> (default : %s)\n"
                            "\t-t <threads> (default : 1)\n"
                            "\t-r <report file>\n", "wordlist.wl");
  }
  exit(0);
}

FILE *parse_args(int argc, char **argv) {
  FILE *wordlist = 0;
  char c, *ptr_wl = "wordlist.wl";
  if (argc < 2) usage();
  dd_args.domain = argv[1];
  dd_args.nthreads = 1;
  printf("DOMAIN: %s\n", dd_args.domain);
  if (dd_args.report) fprintf(dd_args.report, "DOMAIN: %s\n", dd_args.domain);
  argc--;
  argv++;
  opterr = 0;
  while ((c = getopt(argc, argv, "r:w:t:")) != -1) {
    switch (c) {
      case 'w':
        ptr_wl = optarg;
        break;
      case 't':
        printf("THREADS: %s\n", optarg);
        if (dd_args.report) fprintf(dd_args.report, "THREADS: %s\n", optarg);
        dd_args.nthreads = atoi(optarg);
        break;
      case 'r':
        printf("REPORT: %s\n", optarg);
        if (dd_args.report) fprintf(dd_args.report, "REPORT: %s\n", optarg);
        dd_args.report = ck_fopen(optarg, "a");
        break;
      case '?':
        if (optopt == 'r' || optopt == 'w' || optopt == 't') {
          fprintf(stderr, "Option -%c requires an argument.\n", optopt);
          exit(1);
        }
      default:
        usage();
    }
  }
  printf("WORDLIST: %s\n", ptr_wl);
  if (dd_args.report) fprintf(dd_args.report, "WORDLIST: %s\n", ptr_wl);
  wordlist = ck_fopen(ptr_wl, "r");

  printf("\n");
  if (dd_args.report) fprintf(dd_args.report, "\n");
  return wordlist;
}

void resolve_lookup(const char *hostname) {
  int ipv = 0;
  char addr_str[256];
  void *addr_ptr = 0;
  struct addrinfo *res, *ori_res, hints;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags |= AI_CANONNAME;

  if (getaddrinfo(hostname, 0, &hints, &res) == 0) {
    pthread_mutex_lock(&mutexsum);
    printf("%s\n", hostname);
    if (dd_args.report) fprintf(dd_args.report, "%s\n", hostname);
    for (ori_res = res; res; res = res->ai_next) {
      switch (res->ai_family) {
        case AF_INET:
          ipv = 4;
          addr_ptr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
          break;
        case AF_INET6:
          ipv = 6;
          addr_ptr = &((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
          break;
      }
      inet_ntop(res->ai_family, addr_ptr, addr_str, 256);
      printf("IPv%d address: %s\n", ipv, addr_str);
      if (dd_args.report) fprintf(dd_args.report, "IPv%d address: %s\n", ipv, addr_str);
    }
    printf("\n");
    if (dd_args.report) fprintf(dd_args.report, "\n");
    pthread_mutex_unlock(&mutexsum);
    freeaddrinfo(ori_res);
  }
}

void dns_discovery(FILE *file, const char *domain) {
  char line[256];
  char hostname[512];

  while (fgets(line, sizeof line, file) != 0) {
    chomp(line);
    snprintf(hostname, sizeof hostname, "%s.%s", line, domain);
    resolve_lookup(hostname);
  }
}

void *dns_discovery_thread(void *args) {
  FILE *wordlist = (FILE *) args;
  FILE *local_wordlist = tmpfile();
  char line[256];

  pthread_mutex_lock(&mutexsum);
  rewind(wordlist);
  while (fgets(line, sizeof(line), wordlist)) {
    fputs(line, local_wordlist);
  }
  pthread_mutex_unlock(&mutexsum);
  rewind(local_wordlist);

  dns_discovery(local_wordlist, dd_args.domain);

  fclose(local_wordlist);
  return 0;
}

int main(int argc, char **argv) {
  int i;
  pthread_t *threads;
  FILE *wordlist;

  banner();
  wordlist = parse_args(argc, argv);
  threads = (pthread_t *) ck_malloc(dd_args.nthreads * sizeof(pthread_t));

  pthread_mutex_init(&mutexsum, NULL);

  for (i = 0; i < dd_args.nthreads; i++) {
    if (pthread_create(&threads[i], 0, dns_discovery_thread, (void *) wordlist) != 0)
      error("pthread_create");
  }
  for (i = 0; i < dd_args.nthreads; i++) {
    pthread_join(threads[i], 0);
  }

  if (dd_args.report) fclose(dd_args.report);
  free(threads);
  fclose(wordlist);
  return 0;
}

