#include <service_app/service_app.hpp>

#include <iostream>

using namespace vix::service_app;

int main()
{
  ServiceApp app;

  app.on_metrics([](const ServiceMetrics &m)
                 {
        std::cout << "Requests: " << m.requests_total << "\n";
        std::cout << "Errors: " << m.errors_total << "\n";
        std::cout << "Uptime: " << m.uptime_ms << " ms\n"; });

  app.run_startup();

  // simulate metrics export
  app.export_metrics();

  app.run_shutdown();

  return 0;
}
