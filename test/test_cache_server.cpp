#include <glib.h>
#include "cache_server.hpp"

void test_cache_server()
{
  unsigned long server_id = 1;
  unsigned long cache_size = 1024;
  std::string cache_algo = "lru";
  std::string server_name = "test_server";

  CacheAllocation::CacheServer cache_server(server_id, cache_size, cache_algo, server_name);

  g_assert_cmpuint(cache_server.server_id, ==, server_id);
  g_assert_cmpuint(cache_server.cache_size, ==, cache_size);
  g_assert_cmpstr(cache_server.server_name.c_str(), ==, server_name.c_str());
  g_assert_nonnull(cache_server.cache);
}

int main(int argc, char *argv[])
{
  g_test_init(&argc, &argv, NULL);

  g_test_add_func("/cache_server/test_cache_server", test_cache_server);

  return g_test_run();
}