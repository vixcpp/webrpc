#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

static int run(const std::string &cmd)
{
  std::cout << "\n$ " << cmd << "\n";
  const int rc = std::system(cmd.c_str());
  if (rc != 0)
    std::cout << "-> failed with code " << rc << "\n";
  return rc;
}

int main()
{
  // this executable lives in build/examples/,
  // so we run sibling binaries with "./<name>"
  const std::vector<std::string> cmds = {
      "./webrpc_example_basic_request",
      "./webrpc_example_router_dispatch",
      "./webrpc_example_notification",
      "./webrpc_example_batch_requests",
  };

  int failures = 0;
  for (const auto &c : cmds)
  {
    const int rc = run(c);
    if (rc != 0)
      ++failures;
  }

  if (failures == 0)
  {
    std::cout << "\n[webrpc] examples OK\n";
    return 0;
  }

  std::cout << "\n[webrpc] examples FAILED (" << failures << ")\n";
  return 1;
}
